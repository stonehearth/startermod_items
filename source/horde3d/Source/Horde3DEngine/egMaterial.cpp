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

      ResHandle texMap;
      uint32 flags = 0;
      if (!Modules::config().loadTextures) {
         flags |= ResourceFlags::NoQuery;
      }
		
      if (!snode.get("allowCompression", true)) {
         flags |= ResourceFlags::NoTexCompression;
      }

      if (!snode.get("mipmaps", true)) {
         flags |= ResourceFlags::NoTexMipmaps;
      }

      if (snode.get("sRGB", false)) {
         flags |= ResourceFlags::TexSRGB;
      }

      if (snode.get("numAnimationFrames", 0) == 0) {
         texMap = Modules::resMan().addResource(ResourceTypes::Texture, snode.get("map", ""), flags, false);
         sampler.texRes = (TextureResource *)Modules::resMan().resolveResHandle(texMap);
      } else {
         sampler.animationRate = snode.get("frameRate", 24.0f);
         int numFrames = snode.get("numAnimationFrames", 0);

         // Animated maps will be of the form "animXYZ.foo", so look for them in that order.
         std::stringstream ss(snode.get("map", ""));
         std::string baseName;
         std::string extension;
         std::getline(ss, baseName, '.');
         std::getline(ss, extension, '.');
         char buff[256];

         for (int i = 0; i < numFrames; i++) {
            sprintf(buff, "%s%02d.%s", baseName.c_str(), sampler.animatedTextures.size(), extension.c_str());
            ResHandle r = Modules::resMan().addResource(ResourceTypes::Texture, buff, flags, false);
            sampler.animatedTextures.push_back((TextureResource *)Modules::resMan().resolveResHandle(r));
         }

         sampler.texRes = sampler.animatedTextures[0];
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
         uint32 shandle = Modules::resMan().addResource(ResourceTypes::Shader, shader.get("shader", ""), 0, false);
         _shaders.push_back((ShaderResource *)Modules::resMan().resolveResHandle(shandle));
         _context_to_shader_map[shader.as<std::string>().c_str()] = _shaders.back();

         for (auto const& sinput : shader.get_node("inputs")) {
            if (sinput.has("bind_to_material_input")) {
               _input_to_input_map[shandle][sinput.get("name", "")] = sinput.get("bind_to_material_input", "");
            }
            if (sinput.has("default")) {
               MatUniform def;
               int i = 0;
               for (auto const& v : sinput.get_node("default")) {
                  def.values[i++] = v.as<float>();
               }
               _input_defaults[sinput.get("name", "")] = def;
            }
         }
      }
   }
	
	return true;
}

MatUniform* MaterialResource::getUniform(uint32 shaderHandle, std::string const& name)
{
   // First, look to see if we have a binding set up.
   auto const& h = _input_to_input_map.find(shaderHandle);
   if (h == _input_to_input_map.end()) {
      return nullptr;
   }

   auto const& i = h->second.find(name);
   if (i != h->second.end()) {
      return &_uniforms[i->second];
   }

   // No binding exists for that shader; now, lets see if we have a default.
   auto const& j = _input_defaults.find(name);
   if (j != _input_defaults.end()) {
      return &j->second;
   }

   // Nothing left!
   return nullptr;
}


bool MaterialResource::setUniform( std::string const& name, float a, float b, float c, float d )
{
	for( uint32 i = 0; i < _uniforms.size(); ++i )
	{
		if( _uniforms[i].name == name )
		{
			_uniforms[i].values[0] = a;
			_uniforms[i].values[1] = b;
			_uniforms[i].values[2] = c;
			_uniforms[i].values[3] = d;
			return true;
		}
	}

	return false;
}


bool MaterialResource::setArrayUniform( std::string const& name, float* data, int dataCount)
{
	for( uint32 i = 0; i < _uniforms.size(); ++i )
	{
		if( _uniforms[i].name == name )
		{
         _uniforms[i].arrayValues.clear();

         for (int j = 0; j < dataCount; j++)
         {
            _uniforms[i].arrayValues.push_back(data[j]);
         }
			return true;
		}
	}

	return false;
}


int MaterialResource::getElemCount( int elem )
{
	switch( elem )
	{
	case MaterialResData::MaterialElem:
		return 1;
	case MaterialResData::SamplerElem:
		return (int)_samplers.size();
	case MaterialResData::UniformElem:
		return (int)_uniforms.size();
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
	case MaterialResData::UniformElem:
		if( (unsigned)elemIdx < _uniforms.size() )
		{
			switch( param )
			{
			case MaterialResData::UnifValueF4:
				if( (unsigned)compIdx < 4 ) return _uniforms[elemIdx].values[compIdx];
				break;
			}
		}
		break;
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
	case MaterialResData::UniformElem:
		if( (unsigned)elemIdx < _uniforms.size() )
		{	
			switch( param )
			{
			case MaterialResData::UnifValueF4:
				if( (unsigned)compIdx < 4 )
				{	
					_uniforms[elemIdx].values[compIdx] = value;
					return;
				}
				break;
			}
		}
		break;
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

   unsigned int curFrame = (unsigned int)(sampler.currentAnimationTime * sampler.animationRate);
   curFrame = std::min(curFrame, sampler.animatedTextures.size() - 1);
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
	case MaterialResData::UniformElem:
		if( (unsigned)elemIdx < _uniforms.size() )
		{
			switch( param )
			{
			case MaterialResData::UnifNameStr:
				return _uniforms[elemIdx].name.c_str();
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
