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

#ifndef _egShaderState_H_
#define _egShaderState_H_

#include "egPrerequisites.h"
#include "egResource.h"


namespace Horde3D {

// =================================================================================================

struct BlendModes
{
	enum List
	{
		Replace,
		Blend,
		Add,
		AddBlended,
		Mult
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

class ShaderStateResource;
typedef SmartResPtr< ShaderStateResource > PShaderStateResource;

class ShaderStateResource : public Resource
{
public:
	static Resource *factoryFunc( std::string const& name, int flags )
		{ return new ShaderStateResource( name, flags ); }
	
	ShaderStateResource( std::string const& name, int flags );
	~ShaderStateResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );

private:
	bool raiseError( std::string const& msg, int line = -1 );

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
   uint32                            writeMask;

   friend class Renderer;
	friend class ResourceManager;
};

}
#endif // _egMaterial_H_
