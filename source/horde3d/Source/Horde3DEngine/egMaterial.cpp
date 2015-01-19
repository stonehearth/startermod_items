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
#include "utXML.h"
#include <cstring>

#include "utDebug.h"


namespace Horde3D {

using namespace std;


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
	_shaderRes = 0x0;
   _parentMaterial = 0x0;
}


void MaterialResource::release()
{
	_shaderRes = 0x0;
   _parentMaterial = 0x0;
	for( uint32 i = 0; i < _samplers.size(); ++i ) _samplers[i].texRes = 0x0;

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
	if( !Resource::load( data, size ) ) return false;
	
	XMLDoc doc;
	doc.parseBuffer( data, size );
	if( doc.hasError() )
		return raiseError( "XML parsing error" );

	XMLNode rootNode = doc.getRootNode();
	if( strcmp( rootNode.getName(), "Material" ) != 0 )
		return raiseError( "Not a material resource file" );

   const char* parentName = rootNode.getAttribute("parent");
   if (strcmp(parentName, "")) {
      uint32 mat = Modules::resMan().addResource(ResourceTypes::Material, parentName, 0, false);
      _parentMaterial = (MaterialResource*)Modules::resMan().resolveResHandle(mat);
   }

    // Shader
	XMLNode node1 = rootNode.getFirstChild( "Shader" );
	if( !node1.isEmpty() )
	{
		if( node1.getAttribute( "source" ) == 0x0 ) return raiseError( "Missing Shader attribute 'source'" );
			
		uint32 shader = Modules::resMan().addResource(
				ResourceTypes::Shader, node1.getAttribute( "source" ), 0, false );
		_shaderRes = (ShaderResource *)Modules::resMan().resolveResHandle( shader );
	}

	// Texture samplers
	node1 = rootNode.getFirstChild( "Sampler" );
	while( !node1.isEmpty() )
	{
		if( node1.getAttribute( "name" ) == 0x0 ) return raiseError( "Missing Sampler attribute 'name'" );
		if( node1.getAttribute( "map" ) == 0x0 ) return raiseError( "Missing Sampler attribute 'map'" );

		MatSampler sampler;
		sampler.name = node1.getAttribute( "name" );
      sampler.currentAnimationTime = 0;

		ResHandle texMap;
		uint32 flags = 0;
		if( !Modules::config().loadTextures ) flags |= ResourceFlags::NoQuery;
		
		if( _stricmp( node1.getAttribute( "allowCompression", "true" ), "false" ) == 0 ||
			_stricmp( node1.getAttribute( "allowCompression", "1" ), "0" ) == 0 )
			flags |= ResourceFlags::NoTexCompression;

		if( _stricmp( node1.getAttribute( "mipmaps", "true" ), "false" ) == 0 ||
			_stricmp( node1.getAttribute( "mipmaps", "1" ), "0" ) == 0 )
			flags |= ResourceFlags::NoTexMipmaps;

		if( _stricmp( node1.getAttribute( "sRGB", "false" ), "true" ) == 0 ||
			_stricmp( node1.getAttribute( "sRGB", "0" ), "1" ) == 0 )
			flags |= ResourceFlags::TexSRGB;

      if (_stricmp(node1.getAttribute("numAnimationFrames", "0"), "0") == 0) {
		   texMap = Modules::resMan().addResource(
			   ResourceTypes::Texture, node1.getAttribute( "map" ), flags, false );

		   sampler.texRes = (TextureResource *)Modules::resMan().resolveResHandle( texMap );
      } else {
         sampler.animationRate = (float)atoi(node1.getAttribute("frameRate", "24"));
         int numFrames = atoi(node1.getAttribute("numAnimationFrames", "0"));
         // Animated maps will be of the form "animXYZ.foo", so look for them in that order.
         std::stringstream ss(node1.getAttribute("map"));
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

		
		_samplers.push_back( sampler );
		
		node1 = node1.getNextSibling( "Sampler" );
	}
		
	// Vector uniforms
	node1 = rootNode.getFirstChild( "Uniform" );
	while( !node1.isEmpty() )
	{
		if( node1.getAttribute( "name" ) == 0x0 ) return raiseError( "Missing Uniform attribute 'name'" );

		MatUniform uniform;
		uniform.name = node1.getAttribute( "name" );

		uniform.values[0] = (float)atof( node1.getAttribute( "a", "0" ) );
		uniform.values[1] = (float)atof( node1.getAttribute( "b", "0" ) );
		uniform.values[2] = (float)atof( node1.getAttribute( "c", "0" ) );
		uniform.values[3] = (float)atof( node1.getAttribute( "d", "0" ) );

		_uniforms.push_back( uniform );

		node1 = node1.getNextSibling( "Uniform" );
	}
	
	return true;
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
	case MaterialResData::MaterialElem:
		switch( param )
		{
		case MaterialResData::MatShaderI:
			return _shaderRes != 0x0 ? _shaderRes->getHandle() : 0;
		}
		break;
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
	case MaterialResData::MaterialElem:
		switch( param )
		{
		case MaterialResData::MatShaderI:
			if( value == 0 )
			{	
				_shaderRes = 0x0;
				return;
			}
			else
			{
				Resource *res = Modules::resMan().resolveResHandle( value );
				if( res != 0x0 && res->getType() == ResourceTypes::Shader )
					_shaderRes = (ShaderResource *)res;
				else
					Modules::setError( "Invalid handle in h3dSetResParamI for H3DMatRes::MatShaderI" );
				return;
			}
			break;
		}
		break;
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
