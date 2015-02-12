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

#include "egShaderState.h"
#include "egModules.h"
#include "egCom.h"
#include "egTokenizer.h"
#include <cstring>

#include "utDebug.h"


namespace Horde3D {

using namespace std;


ShaderStateResource::ShaderStateResource( std::string const& name, int flags ) :
   Resource( ResourceTypes::ShaderState, name, flags )
{
   initDefault();	
}


ShaderStateResource::~ShaderStateResource()
{
   release();
}


Resource *ShaderStateResource::clone()
{
   ShaderStateResource *res = new ShaderStateResource( "", _flags );

   *res = *this;
   return res;
}


void ShaderStateResource::initDefault()
{
   blendMode = BlendModes::Replace;
   depthFunc = TestModes::LessEqual;
   cullMode = CullModes::Back;
   depthTest = true;
   writeDepth = true;
   alphaToCoverage = false;
   stencilOpModes = StencilOpModes::Off;
   stencilFunc = TestModes::Always;
   stencilRef = 0;
   writeColor = true;
   writeAlpha = true; 
   writeMask = 0xf;
}


void ShaderStateResource::release()
{
}

uint32 toWriteMask(const char* writeMaskStr)
{
   uint32 writeMask = 0;
   while (*writeMaskStr != 0x0)
   {
      if (*writeMaskStr == 'R') {
         writeMask |= 1;
      } else if (*writeMaskStr == 'G') {
         writeMask |= 2;
      } else if (*writeMaskStr == 'B') {
         writeMask |= 4;
      } else if (*writeMaskStr == 'A') {
         writeMask |= 8;
      }
      writeMaskStr++;
   }
   return writeMask;
}

bool ShaderStateResource::raiseError( std::string const& msg, int line )
{
   // Reset
   release();
   initDefault();

   if (line < 0) {
      Modules::log().writeError("ShaderState resource '%s': %s", _name.c_str(), msg.c_str());
   } else {
      Modules::log().writeError("State resource '%s': %s (line %i)", _name.c_str(), msg.c_str(), line);
   }

   return false;
}


bool ShaderStateResource::load(const char *data, int size)
{
   if (!Resource::load( data, size )) {
   return false;
   }

   const char *identifier = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
   const char *intnum = "+-0123456789";
   const char *floatnum = "+-0123456789.eE";
	
   bool unitFree[12] = {true, true, true, true, true, true, true, true, true, true, true, true}; 
   Tokenizer tok(data);

   while(true)
   {
      if(!tok.hasToken()) {
         break;
      } else if (tok.checkToken("ZWriteEnable")) {
         if (!tok.checkToken( "=" )) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("true")) {
            writeDepth = true;
         } else if (tok.checkToken("false")) {
            writeDepth = false;
         } else {
            return raiseError("FX: invalid bool value", tok.getLine());
         }
      } else if (tok.checkToken("ZEnable")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("true")) {
            depthTest = true;
         } else if (tok.checkToken("false")) {
            depthTest = false;
         } else {
            return raiseError("FX: invalid bool value", tok.getLine());
         }
      } else if (tok.checkToken("ZFunc")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("LessEqual")) {
            depthFunc = TestModes::LessEqual;
         } else if (tok.checkToken("Always")) {
            depthFunc = TestModes::Always;
         } else if (tok.checkToken("Equal")) {
            depthFunc = TestModes::Equal;
         } else if (tok.checkToken("Less")) {
            depthFunc = TestModes::Less;
         } else if (tok.checkToken("Greater")) {
            depthFunc = TestModes::Greater;
         } else if (tok.checkToken("GreaterEqual")) {
            depthFunc = TestModes::GreaterEqual;
         } else {
            return raiseError("FX: invalid enum value", tok.getLine());
         }
      } else if (tok.checkToken("StencilFunc")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("LessEqual")) {
            stencilFunc = TestModes::LessEqual;
         } else if (tok.checkToken("Always")) {
            stencilFunc = TestModes::Always;
         } else if (tok.checkToken("Equal")) {
            stencilFunc = TestModes::Equal;
         } else if (tok.checkToken("Less")) {
            stencilFunc = TestModes::Less;
         } else if (tok.checkToken("Greater")) {
            stencilFunc = TestModes::Greater;
         } else if (tok.checkToken("GreaterEqual")) {
            stencilFunc = TestModes::GreaterEqual;
         } else if (tok.checkToken("NotEqual")) {
            stencilFunc = TestModes::NotEqual;
         } else {
            return raiseError("FX: invalid enum value", tok.getLine());
         }
      } else if (tok.checkToken("BlendMode")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("Replace")) {
            blendMode = BlendModes::Replace;
         } else if (tok.checkToken("Blend")) {
            blendMode = BlendModes::Blend;
         } else if (tok.checkToken("Add")) {
            blendMode = BlendModes::Add;
         } else if (tok.checkToken("AddBlended")) {
            blendMode = BlendModes::AddBlended;
         } else if (tok.checkToken("ReplaceByAlpha")) {
            blendMode = BlendModes::ReplaceByAlpha;
         } else if (tok.checkToken("Mult")) {
            blendMode = BlendModes::Mult;
         } else if (tok.checkToken("Sub")) {
            blendMode = BlendModes::Sub;
         } else {
            return raiseError("FX: invalid enum value", tok.getLine());
         }
      } else if (tok.checkToken("CullMode")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("Back")) {
            cullMode = CullModes::Back;
         } else if (tok.checkToken("Front")) {
            cullMode = CullModes::Front;
         } else if (tok.checkToken("None")) {
            cullMode = CullModes::None;
         } else {
            return raiseError("FX: invalid enum value", tok.getLine());
         }
      } else if (tok.checkToken("StencilOp")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("Keep_Dec_Dec")) {
            stencilOpModes = StencilOpModes::Keep_Dec_Dec;
         } else if (tok.checkToken("Keep_Inc_Inc")) {
            stencilOpModes = StencilOpModes::Keep_Inc_Inc;
         } else if (tok.checkToken("Keep_Keep_Inc")) {
            stencilOpModes = StencilOpModes::Keep_Keep_Inc;
         } else if (tok.checkToken("Keep_Keep_Dec")) {
            stencilOpModes = StencilOpModes::Keep_Keep_Dec;
         } else if (tok.checkToken("Replace_Replace_Replace")) {
            stencilOpModes = StencilOpModes::Replace_Replace_Replace;
         } else {
            return raiseError("FX: invalid enum value", tok.getLine());
         }
      } else if (tok.checkToken("StencilRef")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         stencilRef = atoi(tok.getToken(intnum));
      } else if (tok.checkToken("AlphaToCoverage")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         if (tok.checkToken("true") || tok.checkToken("1")) {
            alphaToCoverage = true;
         } else if (tok.checkToken("false") || tok.checkToken("1")) {
            alphaToCoverage = false;
         } else {
            return raiseError("FX: invalid bool value", tok.getLine());
         }
      } else if (tok.checkToken("ColorWriteMask")) {
         if (!tok.checkToken("=")) {
            return raiseError("FX: expected '='", tok.getLine());
         }
         const char* mask = tok.getToken("RGBA");
         writeMask = toWriteMask(mask);
      } else {
         return raiseError("FX: unexpected token", tok.getLine());
      }
		
      if (!tok.checkToken(";")) {
         return raiseError("FX: expected ';'", tok.getLine());
      }
   }
   return true;
}
}  // namespace
