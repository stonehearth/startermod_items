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

#ifndef _egMaterial_H_
#define _egMaterial_H_

#include "egPrerequisites.h"
#include "egResource.h"
#include "egShader.h"
#include "egShaderState.h"
#include "egTexture.h"


namespace Horde3D {

// =================================================================================================
// Material Resource
// =================================================================================================

struct MaterialResData
{
	enum List
	{
		MaterialElem = 400,
		SamplerElem,
		MatClassStr,
		MatShaderI,
		SampNameStr,
		SampTexResI,
		UnifNameStr,
		UnifValueF4,
      AnimatedTexTime
	};
};

// =================================================================================================

struct MatSampler
{
	std::string       name;
	PTextureResource  texRes;
   
   std::vector<PTextureResource> animatedTextures;
   float             currentAnimationTime;
   float             animationRate;  // fps
};


struct MatUniform
{
	std::string  name;
	float        values[4];
   std::vector<float> arrayValues;


	MatUniform()
	{
		values[0] = 0; values[1] = 0; values[2] = 0; values[3] = 0;
	}
};

// =================================================================================================

class MaterialResource;
typedef SmartResPtr< MaterialResource > PMaterialResource;

class MaterialResource : public Resource
{
public:
	static Resource *factoryFunc( std::string const& name, int flags )
		{ return new MaterialResource( name, flags ); }
	
	MaterialResource( std::string const& name, int flags );
	~MaterialResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );
	bool setUniform( std::string const& name, float a, float b, float c, float d );
   bool setArrayUniform( std::string const& name, float* data, int dataCount);

	int getElemCount( int elem );
	int getElemParamI( int elem, int elemIdx, int param );
	void setElemParamI( int elem, int elemIdx, int param, int value );
	float getElemParamF( int elem, int elemIdx, int param, int compIdx );
	void setElemParamF( int elem, int elemIdx, int param, int compIdx, float value );
	const char *getElemParamStr( int elem, int elemIdx, int param );
	void setElemParamStr( int elem, int elemIdx, int param, const char *value );

   std::vector<PShaderResource> const& getShaders() const { return _shaders; }
   std::vector<MatSampler>& getSamplers() { return _samplers; }
   MatUniform* getUniform(std::string const& context, std::string const& name);
   PShaderStateResource getShaderState(std::string const& context) { return _context_to_shader_state[context]; }
   PShaderResource getShader(std::string const& context) { return _context_to_shader_map[context]; }

private:
	bool raiseError( std::string const& msg, int line = -1 );
   void updateSamplerAnimation(int samplerNum, float animTime);

private:
	std::vector<PShaderResource> _shaders;
	std::vector<MatSampler>      _samplers;
   std::unordered_map<std::string, MatUniform>   _uniforms;
   std::unordered_map<std::string, std::unordered_map<std::string, std::string>> _input_to_input_map;
   std::unordered_map<std::string, PShaderResource> _context_to_shader_map;
   std::unordered_map<std::string, std::unordered_map<std::string, MatUniform>> _input_defaults;
   std::unordered_map<std::string, PShaderStateResource> _context_to_shader_state;

	friend class ResourceManager;
	friend class MeshNode;
};

}
#endif // _egMaterial_H_
