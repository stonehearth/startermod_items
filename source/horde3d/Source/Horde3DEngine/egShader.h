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

#ifndef _egShader_H_
#define _egShader_H_

#include "egPrerequisites.h"
#include "egResource.h"
#include "egTexture.h"
#include <set>


namespace Horde3D {

// =================================================================================================
// Code Resource
// =================================================================================================

class CodeResource;
typedef SmartResPtr< CodeResource > PCodeResource;

class CodeResource : public Resource
{
public:
	static Resource *factoryFunc( std::string const& name, int flags )
		{ return new CodeResource( name, flags ); }
	
	CodeResource( std::string const& name, int flags );
	~CodeResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );

	bool hasDependency( CodeResource *codeRes );
	bool tryLinking();
	std::string assembleCode();

	bool isLoaded() { return _loaded; }
	std::string const& getCode() { return _code; }
   std::string getVersion() { return _verLine; }

private:
	bool raiseError( std::string const& msg );
	void updateShaders();

private:
   std::string                                        _verLine;
	std::string                                        _code;
	std::vector< std::pair< PCodeResource, size_t > >  _includes;	// Pair: Included res and location in _code

	friend class Renderer;
};


// =================================================================================================
// Shader Resource
// =================================================================================================

struct ShaderResData
{
	enum List
	{
		ContextElem = 600,
		SamplerElem,
		UniformElem,
		ContNameStr,
		SampNameStr,
		UnifNameStr,
		UnifSizeI,
		UnifDefValueF4
	};
};




struct ShaderCombination
{
	uint32              shaderObj;
	uint32              lastUpdateStamp;

	// Engine uniforms
	int                 uni_currentTime;
	int                 uni_frameBufSize, uni_viewPortSize, uni_viewPortPos;
   int                 uni_lodLevels;
	int                 uni_halfTanFoV, uni_nearPlane, uni_farPlane;
	int                 uni_viewMat, uni_viewMatInv, uni_projMat, uni_viewProjMat, uni_viewProjMatInv, uni_viewerPos;
   int                 uni_camProjMat, uni_camViewProjMat, uni_camViewProjMatInv, uni_camViewMat, uni_camViewMatInv, uni_camViewerPos;
   int                 uni_projectorMat;
   int                 uni_worldMat, uni_worldNormalMat, uni_nodeId;
	int                 uni_skinMatRows;
	int                 uni_lightPos, uni_lightRadii, uni_lightDir, uni_lightColor, uni_lightAmbientColor;
	int                 uni_shadowSplitDists, uni_shadowMats, uni_shadowMapSize, uni_shadowBias;
	int                 uni_parPosArray, uni_parSizeAndRotArray, uni_parColorArray;
   int                 uni_cubeBatchTransformArray, uni_cubeBatchColorArray;
	int                 uni_olayColor;
   int                 uni_bones, uni_modelScale;

	std::vector< int >  customSamplers;
	std::vector< int >  customUniforms;
   std::set<std::string> engineFlags;


	ShaderCombination() :
		shaderObj( 0 ), lastUpdateStamp( 0 )
	{
	}
};

// =================================================================================================

struct ShaderSampler
{
	std::string            id;
	TextureTypes::List     type;
	PTextureResource       defTex;
	int                    texUnit;
	uint32                 sampState;


	ShaderSampler() :
		texUnit( -1 ), sampState( 0 )
	{
	}
};

struct ShaderUniform
{
	std::string    id;
	float          defValues[4];
	unsigned char  size;

   unsigned char       arraySize;
   std::vector<float>  arrayValues;
};


class ShaderResource : public Resource
{
public:
	static Resource *factoryFunc( std::string const& name, int flags )
		{ return new ShaderResource( name, flags ); }

	static void setPreambles( std::string const& vertPreamble, std::string const& fragPreamble )
		{ _vertPreamble = vertPreamble; _fragPreamble = fragPreamble; }

	ShaderResource( std::string const& name, int flags );
	~ShaderResource();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );
	void compileContexts();

	int getElemCount( int elem );
	int getElemParamI( int elem, int elemIdx, int param );
	float getElemParamF( int elem, int elemIdx, int param, int compIdx );
	void setElemParamF( int elem, int elemIdx, int param, int compIdx, float value );
	const char *getElemParamStr( int elem, int elemIdx, int param );

	CodeResource *getCode( uint32 index ) { return &_codeSections[index]; }

private:
	bool raiseError( std::string const& msg, int line = -1 );
	bool parseFXSection( char *data );
	void compileCombination(ShaderCombination &combination);
	
private:
	static std::string            _vertPreamble, _fragPreamble;
	static std::string            _tmpCode0, _tmpCode1;
	

   // Shaders
   std::vector<ShaderCombination>    shaderCombinations;
   int                               vertCodeIdx, fragCodeIdx;
   bool                              compiled;

	std::vector< ShaderSampler >  _samplers;
	std::vector< ShaderUniform >  _uniforms;
	std::vector< CodeResource >   _codeSections;

	friend class Renderer;
   friend class CodeResource;
};

typedef SmartResPtr< ShaderResource > PShaderResource;

}
#endif //_egShader_H_
