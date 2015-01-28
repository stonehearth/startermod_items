// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
// --------------------------------------
// Copyright (C) 2006-2011 Nicolas Schulz
//
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/legal/epl-v10.html
//
// *************************************************************************************************

#include "egMaterial.h"
#include "egTexture.h"
#include "egModules.h"
#include "egCom.h"
#include "libjson.h"
#include "lib/json/node.h"
#include <cstring>

#include "utDebug.h"


namespace Horde3D {

using namespace std;
using namespace ::radiant::json;


MaterialResource::MaterialResource( std::string const& name, int flags ) :
	Resource( ResourceTypes::Material, name, flags )
{
	initDefault();	
}


MaterialResource::~MaterialResource()
{
	release();
}


Resource *MaterialResource::clone()
{
	MaterialResource *res = new MaterialResource( "", _flags );

	*res = *this;

   // When cloning a resource, reset its animation time to zero.
   for (auto& s : res->_samplers) {
      s.currentAnimationTime = 0;
   }
	
	return res;
}


void MaterialResource::initDefault()
{
   _shaders.clear();
}


void MaterialResource::release()
{
   _shaders.clear();
	for(uint32 i = 0; i < _samplers.size(); ++i) {
      _samplers[i].texRes = 0x0;
   }

	_samplers.clear();
	_uniforms.clear();
   _context_to_shader_map.clear();
   _input_defaults.clear();
   _input_to_input_map.clear();
   _context_to_shader_state.clear();
}


bool MaterialResource::raiseError( std::string const& msg, int line )
{
	// Reset
	release();
	initDefault();

	if( line < 0 )
		Modules::log().writeError( "Material resource '%s': %s", _name.c_str(), msg.c_str() );
	else
		Modules::log().writeError( "Material resource '%s' in line %i: %s", _name.c_str(), line, msg.c_str() );
	
	return false;
}


void loadTexture(MatSampler& sampler, std::string const& path) {
   ResHandle texMap;
   texMap = Modules::resMan().addResource(ResourceTypes::Texture, path, sampler.flags, false);
   sampler.texRes = (TextureResource *)Modules::resMan().resolveResHandle(texMap);
}

void loadAnimatedTexture(MatSampler& sampler, std::string const& path, int numFrames, float frameRate) {
   sampler.animationRate = frameRate;
   sampler.animatedTextures.clear();

   // Animated maps will be of the form "animXYZ.foo", so look for them in that order.
   std::stringstream ss(path);
   std::string baseName;
   std::string extension;
   std::getline(ss, baseName, '.');
   std::getline(ss, extension, '.');
   char buff[256];

   for (int i = 0; i < numFrames; i++) {
      sprintf(buff, "%s%02d.%s", baseName.c_str(), sampler.animatedTextures.size(), extension.c_str());
      ResHandle r = Modules::resMan().addResource(ResourceTypes::Texture, buff, sampler.flags, false);
      sampler.animatedTextures.push_back((TextureResource *)Modules::resMan().resolveResHandle(r));
   }

   sampler.texRes = sampler.animatedTextures[0];
}

bool MaterialResource::load( const char *data, int size )
{
   if(!Resource::load( data, size )) {
      return false;
   }
	
   std::string jsonData(data, size);
   Node root(libjson::parse(jsonData));

   for (auto const& snode : root.get_node("samplers")) {
      if (!snode.has("name")) {
         return raiseError("Missing Sampler attribute 'name'");
      }
      if (!snode.has("map")) {
         return raiseError("Missing Sampler attribute 'map'");
      }

      MatSampler sampler;
      sampler.name = snode.get("name", "");
      sampler.currentAnimationTime = 0;
      sampler.flags = 0;

      if (!Modules::config().loadTextures) {
         sampler.flags |= ResourceFlags::NoQuery;
      }
		
      if (!snode.get("allowCompression", true)) {
         sampler.flags |= ResourceFlags::NoTexCompression;
      }

      if (!snode.get("mipmaps", true)) {
         sampler.flags |= ResourceFlags::NoTexMipmaps;
      }

      if (snode.get("sRGB", false)) {
         sampler.flags |= ResourceFlags::TexSRGB;
      }

      if (snode.get("numAnimationFrames", 0) == 0) {
         loadTexture(sampler, snode.get("map", ""));
      } else {
         loadAnimatedTexture(sampler, snode.get("map", ""), snode.get("numAnimationFrames", 0), snode.get("frameRate", 24.0f));
      }
      _samplers.push_back(sampler);
   }

   // Vector uniforms
   for (auto const& inode : root.get_node("inputs")) {
      if (!inode.has("name")) {
         return raiseError("Missing input field 'name'");
      }

      MatUniform uniform;

      uniform.name = inode.get("name", "");
      if (inode.has("default")) {
         int i = 0;
         for (auto const& v : inode.get_node("default")) {
            uniform.values[i++] = v.as<float>();
         }
      }
      _uniforms[uniform.name] = uniform;
   }

   if (root.has("shaders")) {
      for (auto const& shader : root.get_node("shaders")) {
         uint32 sthandle = Modules::resMan().addResource(ResourceTypes::ShaderState, shader.get("state", ""), 0, false);
         _context_to_shader_state[shader.name()] = (ShaderStateResource *)Modules::resMan().resolveResHandle(sthandle);

         uint32 shandle = Modules::resMan().addResource(ResourceTypes::Shader, shader.get("shader", ""), 0, false);
         _shaders.push_back((ShaderResource *)Modules::resMan().resolveResHandle(shandle));
         _context_to_shader_map[shader.name()] = _shaders.back();
         for (auto const& sinput : shader.get_node("inputs")) {
            if (sinput.has("bind_to_material_input")) {
               _input_to_input_map[shader.name()][sinput.get("name", "")] = sinput.get("bind_to_material_input", "");
            }
            if (sinput.has("default")) {
               MatUniform def;
               int i = 0;
               for (auto const& v : sinput.get_node("default")) {
                  def.values[i++] = v.as<float>();
               }
               _input_defaults[shader.name()][sinput.get("name", "")] = def;
            }
         }
      }
   }
	
	return true;
}

// Gets a _shader_ uniform.  If that shader binds to a material uniform, use that.  Otherwise,
// get the default value for that shader uniform.  Otherwise, return null.
MatUniform* MaterialResource::getUniform(std::string const& context, std::string const& name)
{
   // First, look to see if we have a binding set up.
   auto const& h = _input_to_input_map.find(context);
   if (h != _input_to_input_map.end()) {
      auto const& i = h->second.find(name);
      if (i != h->second.end()) {
         return &_uniforms[i->second];
      }
   }

   // No binding exists for that shader; now, lets see if we have a default.
   auto const& j = _input_defaults.find(context);
   if (j != _input_defaults.end()) {
      auto const& k = j->second.find(name);
      if (k != j->second.end()) {
         return &k->second;
      }
   }

   // Nothing left!
   return nullptr;
}


bool MaterialResource::setSampler(std::string const& samplerName, std::string const& resPath, int numFrames, float frameRate)
{
   for (auto& sampler : _samplers) {
      if (sampler.name == samplerName) {
         if (numFrames == 0) {
            loadTexture(sampler, resPath);
         } else {
            loadAnimatedTexture(sampler, resPath, numFrames, frameRate);
         }
      }
   }
	return true;
}


bool MaterialResource::setUniform( std::string const& name, float a, float b, float c, float d )
{
   auto i = _uniforms.find(name);
   if (i == _uniforms.end()) {
      return false;
   }
   MatUniform &m = i->second;
   m.values[0] = a;
   m.values[1] = b;
   m.values[2] = c;
   m.values[3] = d;
	return true;
}


bool MaterialResource::setArrayUniform( std::string const& name, float* data, int dataCount)
{
   auto i = _uniforms.find(name);
   if (i == _uniforms.end()) {
      return false;
   }

   MatUniform &m = i->second;
   m.arrayValues.clear();

   for (int j = 0; j < dataCount; j++)
   {
      m.arrayValues.push_back(data[j]);
   }
	return true;
}


int MaterialResource::getElemCount( int elem )
{
	switch( elem )
	{
	case MaterialResData::MaterialElem:
		return 1;
	case MaterialResData::SamplerElem:
		return (int)_samplers.size();
	default:
		return Resource::getElemCount( elem );
	}
}


int MaterialResource::getElemParamI( int elem, int elemIdx, int param )
{
	switch( elem )
	{
	case MaterialResData::SamplerElem:
		if( (unsigned)elemIdx < _samplers.size() )
		{
			switch( param )
			{
			case MaterialResData::SampTexResI:
				return _samplers[elemIdx].texRes->getHandle();
			}
		}
		break;
	}

	return Resource::getElemParamI( elem, elemIdx, param );
}


void MaterialResource::setElemParamI( int elem, int elemIdx, int param, int value )
{
	switch( elem )
	{
	case MaterialResData::SamplerElem:
		if( (unsigned)elemIdx < _samplers.size() )
		{
			switch( param )
			{
			case MaterialResData::SampTexResI:
				Resource *res = Modules::resMan().resolveResHandle( value );
				if( res != 0x0 && res->getType() == ResourceTypes::Texture )
					_samplers[elemIdx].texRes = (TextureResource *)res;
				else
					Modules::setError( "Invalid handle in h3dSetResParamI for H3DMatRes::SampTexResI" );
				return;
			}
		}
		break;
	}

	Resource::setElemParamI( elem, elemIdx, param, value );
}


float MaterialResource::getElemParamF( int elem, int elemIdx, int param, int compIdx )
{
	switch( elem )
	{
   case MaterialResData::SamplerElem:
      switch(param) {
      case MaterialResData::AnimatedTexTime:
         return _samplers[elemIdx].currentAnimationTime;
         break;
      }
      break;
	}
	
	return Resource::getElemParamF( elem, elemIdx, param, compIdx );
}


void MaterialResource::setElemParamF( int elem, int elemIdx, int param, int compIdx, float value )
{
	switch( elem )
	{
   case MaterialResData::SamplerElem:
      switch (param) {
      case MaterialResData::AnimatedTexTime:
         updateSamplerAnimation(elemIdx, value);
         return;
         break;
      }
      break;
	}
	
	Resource::setElemParamF( elem, elemIdx, param, compIdx, value );
}

void MaterialResource::updateSamplerAnimation(int samplerNum, float animTime) 
{
   if (samplerNum >= (int)_samplers.size()) {
      return;
   }
   MatSampler &sampler = _samplers[samplerNum];
   if (sampler.animatedTextures.size() == 0) {
      return;
   }

   float totalLen = sampler.animatedTextures.size() * (1.0f / sampler.animationRate);
   sampler.currentAnimationTime = fmod(animTime, totalLen);

   int curFrame = (unsigned int)(sampler.currentAnimationTime * sampler.animationRate);
   curFrame = std::min(curFrame, (int)sampler.animatedTextures.size() - 1);
   setElemParamI(MaterialResData::SamplerElem, samplerNum, MaterialResData::SampTexResI, sampler.animatedTextures[curFrame]->getHandle());
}

const char *MaterialResource::getElemParamStr( int elem, int elemIdx, int param )
{
	switch( elem )
	{
	case MaterialResData::SamplerElem:
		if( (unsigned)elemIdx < _samplers.size() )
		{
			switch( param )
			{
			case MaterialResData::SampNameStr:
				return _samplers[elemIdx].name.c_str();
			}
		}
		break;
	}
	
	return Resource::getElemParamStr( elem, elemIdx, param );
}


void MaterialResource::setElemParamStr( int elem, int elemIdx, int param, const char *value )
{
	Resource::setElemParamStr( elem, elemIdx, param, value );
}

}  // namespace
