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
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new CodeResource( name, flags ); }
	
	CodeResource( const std::string &name, int flags );
	~CodeResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );

	bool hasDependency( CodeResource *codeRes );
	bool tryLinking();
	std::string assembleCode();

	bool isLoaded() { return _loaded; }
	const std::string &getCode() { return _code; }

private:
	bool raiseError( const std::string &msg );
	void updateShaders();

private:
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

// =================================================================================================

struct BlendModes
{
	enum List
	{
		Replace,
		Blend,
		Add,
		AddBlended,
		Mult,
      Whateva
	};
};

struct TestModes
{
	enum List
	{
		Always,
		Equal,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
      NotEqual
	};
};

struct CullModes
{
	enum List
	{
		Back,
		Front,
		None
	};
};

struct StencilOpModes
{
   enum List
   {
      Off,
      Keep_Dec_Dec,
      Keep_Inc_Inc,
      Keep_Keep_Inc,
      Keep_Keep_Dec,
      Replace_Replace_Replace
   };
};


struct ShaderCombination
{
	uint32              shaderObj;
	uint32              lastUpdateStamp;

	// Engine uniforms
	int                 uni_currentTime;
	int                 uni_frameBufSize;
	int                 uni_halfTanFoV, uni_nearPlane, uni_farPlane;
	int                 uni_viewMat, uni_viewMatInv, uni_projMat, uni_viewProjMat, uni_viewProjMatInv, uni_viewerPos;
   int                 uni_camProjMat, uni_camViewProjMat, uni_camViewProjMatInv, uni_camViewMat, uni_camViewMatInv, uni_camViewerPos;
   int                 uni_projectorMat;
   int                 uni_worldMat, uni_worldNormalMat, uni_nodeId;
	int                 uni_skinMatRows;
	int                 uni_lightPos, uni_lightDir, uni_lightColor, uni_lightAmbientColor;
	int                 uni_shadowSplitDists, uni_shadowMats, uni_shadowMapSize, uni_shadowBias;
	int                 uni_parPosArray, uni_parSizeAndRotArray, uni_parColorArray;
   int                 uni_cubeBatchTransformArray, uni_cubeBatchColorArray;
	int                 uni_olayColor;

	std::vector< int >  customSamplers;
	std::vector< int >  customUniforms;
   std::set<std::string> engineFlags;


	ShaderCombination() :
		shaderObj( 0 ), lastUpdateStamp( 0 )
	{
	}
};


struct ShaderContext
{
	std::string                       id;
	
	// RenderConfig
	BlendModes::List                  blendMode;
	TestModes::List                   depthFunc;
	CullModes::List                   cullMode;
   StencilOpModes::List              stencilOpModes;
   TestModes::List                   stencilFunc;
   int                               stencilRef;
	bool                              depthTest;
	bool                              writeDepth;
   bool                              writeColor;
   bool                              writeAlpha;
	bool                              alphaToCoverage;
	
	// Shaders
	std::vector<ShaderCombination>    shaderCombinations;
	int                               vertCodeIdx, fragCodeIdx;
	bool                              compiled;


	ShaderContext() :
		blendMode( BlendModes::Replace ), depthFunc( TestModes::LessEqual ),
		cullMode( CullModes::Back ), depthTest( true ), writeDepth( true ), alphaToCoverage( false ),
      vertCodeIdx( -1 ), fragCodeIdx( -1 ), compiled( false ), stencilOpModes(StencilOpModes::Off), 
      stencilFunc(TestModes::Always), stencilRef(0), writeColor(true), writeAlpha(true)
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
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new ShaderResource( name, flags ); }

	static void setPreambles( const std::string &vertPreamble, const std::string &fragPreamble )
		{ _vertPreamble = vertPreamble; _fragPreamble = fragPreamble; }

	ShaderResource( const std::string &name, int flags );
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

	ShaderContext *findContext( const std::string &name )
	{
		for( uint32 i = 0; i < _contexts.size(); ++i )
			if( _contexts[i].id == name ) return &_contexts[i];
		
		return 0x0;
	}

	std::vector< ShaderContext > &getContexts() { return _contexts; }
	CodeResource *getCode( uint32 index ) { return &_codeSections[index]; }

private:
	bool raiseError( const std::string &msg, int line = -1 );
	bool parseFXSection( char *data );
	void compileCombination(ShaderContext &context, ShaderCombination &combination);
	
private:
	static std::string            _vertPreamble, _fragPreamble;
	static std::string            _tmpCode0, _tmpCode1;
	
	std::vector< ShaderContext >  _contexts;
	std::vector< ShaderSampler >  _samplers;
	std::vector< ShaderUniform >  _uniforms;
	std::vector< CodeResource >   _codeSections;

	friend class Renderer;
};

typedef SmartResPtr< ShaderResource > PShaderResource;

}
#endif //_egShader_H_
