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

#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "egRendererBase.h"
#include "egModules.h"
#include "egCom.h"
#include "utOpenGL.h"
#include <gl\GLU.h>

#include "utDebug.h"
#include "lib/perfmon/perfmon.h"

namespace Horde3D {

#ifdef H3D_VALIDATE_DRAWCALLS
#	define CHECK_GL_ERROR checkGLError();
#else
#	define CHECK_GL_ERROR
#endif

// Because glGetError is surprisingly expensive.
static bool _enable_gl_validation = false;
void validateGLCall(const char* errorStr)
{
   if (_enable_gl_validation) {
      uint32 error = glGetError();
      if (error != GL_NO_ERROR) {
         Modules::log().writeError(errorStr, error);
         ASSERT(false);
      }
   }
}


// =================================================================================================
// RenderDevice
// =================================================================================================

RenderDevice::RenderDevice()
{
	_numVertexLayouts = 0;
	
	_vpX = 0; _vpY = 0; _vpWidth = 320; _vpHeight = 240;
	_scX = 0; _scY = 0; _scWidth = 320; _scHeight = 240;
	_prevShaderId = _curShaderId = 0;
	_curRendBuf = 0; _outputBufferIndex = 0;
	_textureMem = 0; _bufferMem = 0;
	_curVertLayout = _newVertLayout = 0;
	_curIndexBuf = _newIndexBuf = 0;
	_indexFormat = (uint32)IDXFMT_16;
	_pendingMask = 0;
   _shadowFactor = 0.0f;
   _shadowUnits = 0.0f;
}


RenderDevice::~RenderDevice()
{
}


void RenderDevice::initStates()
{
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
}

CardType RenderDevice::getCardType(const char* cardString)
{
   if (cardString == nullptr)
   {
      return CardType::UNKNOWN;
   }

   if (strncmp("ATI", gRDI->getCaps().vendor, 3) == 0 ||
       strncmp("AMD", gRDI->getCaps().vendor, 3) == 0)
   {
      return CardType::ATI;
   }

   if (strncmp("Intel", gRDI->getCaps().vendor, 5) == 0)
   {
      return CardType::INTEL;
   }

   if (strncmp("NVIDIA", gRDI->getCaps().vendor, 6) == 0)
   {
      return CardType::NVIDIA;
   }
   return CardType::UNKNOWN;
}

void _stdcall glDebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, 
      int length, const char* message, void* userParam)
{
   Modules::log().writeWarning(message);
}


bool RenderDevice::init(int glMajor, int glMinor, bool msaaWindowSupported, bool enable_gl_logging)
{
	bool failed = false;
   _msaaFramebuffer = msaaWindowSupported;
   _enable_gl_validation = enable_gl_logging;

	char *vendor = (char *)glGetString( GL_VENDOR );
	char *renderer = (char *)glGetString( GL_RENDERER );
	char *version = (char *)glGetString( GL_VERSION );

	Modules::log().writeInfo( "Initializing GL2 backend using OpenGL driver '%s' by '%s' on '%s'",
	                          version, vendor, renderer );
	
	// Init extensions
	if( !initOpenGLExtensions() )
	{	
		Modules::log().writeError( "Could not find all required OpenGL function entry points" );
		failed = true;
	}

   // Check that the requested version of OpenGL 2.0 available
   if( glExt::majorVersion * 10 + glExt::minorVersion < (glMajor * 10 + glMinor) )
   {
      Modules::log().writeError( "OpenGL %d.%d not available", glMajor, glMinor );
      failed = true;
   }
	
	// Check that required extensions are supported
	if( !glExt::EXT_framebuffer_object )
	{
		Modules::log().writeError( "Extension EXT_framebuffer_object not supported" );
		failed = true;
	}
	if( !glExt::EXT_texture_filter_anisotropic )
	{
		Modules::log().writeError( "Extension EXT_texture_filter_anisotropic not supported" );
		failed = true;
	}
	if( !glExt::EXT_texture_compression_s3tc )
	{
		Modules::log().writeError( "Extension EXT_texture_compression_s3tc not supported" );
		failed = true;
	}
	if( !glExt::EXT_texture_sRGB )
	{
		Modules::log().writeError( "Extension EXT_texture_sRGB not supported" );
		failed = true;
	}
	
	if( failed )
	{
		Modules::log().writeError( "Failed to init renderer backend, debug info following" );
		char *exts = (char *)glGetString( GL_EXTENSIONS );
		Modules::log().writeInfo( "Supported extensions: '%s'", exts );

		return false;
	}
	
   if (enable_gl_logging && glDebugMessageCallbackARB)
   {
      glDebugMessageCallbackARB(glDebugCallback, NULL);
   }

	// Get capabilities
	_caps.texFloat = glExt::ARB_texture_float ? 1 : 0;
	_caps.texNPOT = glExt::ARB_texture_non_power_of_two ? 1 : 0;
	_caps.rtMultisampling = glExt::EXT_framebuffer_multisample ? 1 : 0;
   _caps.glVersion = glExt::majorVersion * 10 + glExt::minorVersion;
   _caps.hasInstancing = _caps.glVersion >= 33;
   _caps.renderer = renderer;
   _caps.vendor = vendor;
   _caps.cardType = getCardType(vendor);
   _caps.hasPinnedMemory = glExt::AMD_pinned_memory;
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_caps.maxTextureSize);

   // Don't trust Intel's EXT_framebuffer support on old drivers.
   if (_caps.cardType == CardType::INTEL && _caps.glVersion <= 30) {
      _caps.rtMultisampling = 0;
   }

	// Find supported depth format (some old ATI cards only support 16 bit depth for FBOs)
	_depthFormat = GL_DEPTH_COMPONENT24;
	uint32 testBuf = createRenderBuffer( 32, 32, TextureFormats::BGRA8, true, 1, 0 ); 
	if( testBuf == 0 )
	{	
		_depthFormat = GL_DEPTH_COMPONENT16;
		Modules::log().writeWarning( "Render target depth precision limited to 16 bit" );
	}
	else
		destroyRenderBuffer( testBuf );
	
	initStates();
	resetStates();

	return true;
}


// =================================================================================================
// Vertex layouts
// =================================================================================================

uint32 RenderDevice::registerVertexLayout( uint32 numAttribs, VertexLayoutAttrib *attribs )
{
	if( _numVertexLayouts == MaxNumVertexLayouts )
		return 0;
	
	_vertexLayouts[_numVertexLayouts].numAttribs = numAttribs;

	for( uint32 i = 0; i < numAttribs; ++i ) {
		_vertexLayouts[_numVertexLayouts].attribs[i] = attribs[i];
      _vertexLayouts[_numVertexLayouts].divisors[i].divisor = 0;
   }

	return ++_numVertexLayouts;
}

uint32 RenderDevice::registerVertexLayout( uint32 numAttribs, VertexLayoutAttrib *attribs, VertexDivisorAttrib *divisors )
{
	if( _numVertexLayouts == MaxNumVertexLayouts )
		return 0;
	
	_vertexLayouts[_numVertexLayouts].numAttribs = numAttribs;

	for( uint32 i = 0; i < numAttribs; ++i ) {
		_vertexLayouts[_numVertexLayouts].attribs[i] = attribs[i];
      _vertexLayouts[_numVertexLayouts].divisors[i] = divisors[i];
   }

	return ++_numVertexLayouts;
}

// =================================================================================================
// Buffers
// =================================================================================================

uint32 RenderDevice::createVertexBuffer( uint32 size, uint32 usage, const void *data )
{
   RDIBuffer buf = { 0xff };

	buf.type = GL_ARRAY_BUFFER;
	buf.size = size;
   buf.usage = usage;
	glGenBuffers( 1, &buf.glObj );
   ASSERT(buf.glObj != -1);
   ASSERT(buf.glObj != 0);

	glBindBuffer( buf.type, buf.glObj );
	glBufferData( buf.type, size, data, usage );
	glBindBuffer( buf.type, 0 );
	
   validateGLCall("Error creating vertex buffer: %d");

   _bufferMem += size;
	return _buffers.add( buf );
}


uint32 RenderDevice::createIndexBuffer( uint32 size, const void *data )
{
   RDIBuffer buf = { 0xff };

	buf.type = GL_ELEMENT_ARRAY_BUFFER;
	buf.size = size;
   buf.usage = GL_STATIC_DRAW;
	glGenBuffers( 1, &buf.glObj );
   ASSERT(buf.glObj != -1);
   ASSERT(buf.glObj != 0);

   glBindBuffer( buf.type, buf.glObj );
	glBufferData( buf.type, size, data, buf.usage );
	glBindBuffer( buf.type, 0 );
	
   validateGLCall("Error creating index buffer: %d");

   _bufferMem += size;
	return _buffers.add( buf );
}


uint32 RenderDevice::createPixelBuffer( uint32 type, uint32 size, const void *data )
{
   RDIBuffer buf = { 0xff };
   buf.type = type;
   buf.size = size;
   buf.usage = GL_DYNAMIC_DRAW;
   glGenBuffers(1, &buf.glObj);

   ASSERT(buf.glObj != -1);
   ASSERT(buf.glObj != 0);

   glBindBuffer(buf.type, buf.glObj);
   glBufferData(buf.type, size, data, buf.usage);
   glBindBuffer(buf.type, 0);

   validateGLCall("Error creating pixel buffer: %d");

   _bufferMem += size;
   return _buffers.add(buf);
}


void RenderDevice::destroyBuffer( uint32 bufObj )
{
	if( bufObj == 0 ) return;
	
	RDIBuffer& buf = _buffers.getRef( bufObj );
	glDeleteBuffers( 1, &buf.glObj );

   validateGLCall("Error destroying buffer: %d");

	_bufferMem -= buf.size;
	_buffers.remove( bufObj );
}


void RenderDevice::updateBufferData( uint32 bufObj, uint32 offset, uint32 size, void *data )
{
	const RDIBuffer &buf = _buffers.getRef( bufObj );
	ASSERT( offset + size <= buf.size );
	
	glBindBuffer( buf.type, buf.glObj );
	
	if( offset == 0 &&  size == buf.size )
	{
		// Replacing the whole buffer can help the driver to avoid pipeline stalls
		glBufferData( buf.type, size, NULL, buf.usage );
		glBufferData( buf.type, size, data, buf.usage );
	} else {
      if (getCaps().cardType == NVIDIA) {
         // nVidia cards are a touch faster using an explicit orphan.
         glBufferData( buf.type, buf.size, NULL, buf.usage );
         glBufferSubData( buf.type, offset, size, data );
      } else {
         // AMD and Intel cards are happier with just the buffer call.
         glBufferData( buf.type, size, data, buf.usage );
      }
   }
	glBindBuffer( buf.type, 0 );
}


void* RenderDevice::mapBuffer(uint32 bufObj, bool discard)
{
   const RDIBuffer &buf = _buffers.getRef( bufObj );
   glBindBuffer(buf.type, buf.glObj);

   if (discard)
   {
      glBufferData(buf.type, buf.size, NULL, buf.usage);
   }
   void* result = glMapBuffer(buf.type, GL_WRITE_ONLY);
   glBindBuffer(buf.type, 0);

   validateGLCall("Error mapping buffer: %d");

   return result;
}


void RenderDevice::unmapBuffer(uint32 bufObj)
{
   const RDIBuffer &buf = _buffers.getRef( bufObj );
   glBindBuffer(buf.type, buf.glObj);
   glUnmapBuffer(buf.type);
   glBindBuffer(buf.type, 0);

   validateGLCall("Error unmapping buffer: %d");
}


// =================================================================================================
// Textures
// =================================================================================================

uint32 RenderDevice::calcTextureSize( TextureFormats::List format, int width, int height, int depth )
{
	switch( format )
	{
	case TextureFormats::BGRA8:
		return width * height * depth * 4;
	case TextureFormats::DXT1:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 8;
	case TextureFormats::DXT3:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 16;
	case TextureFormats::DXT5:
		return std::max( width / 4, 1 ) * std::max( height / 4, 1 ) * depth * 16;
	case TextureFormats::RGBA16F:
		return width * height * depth * 8;
	case TextureFormats::RGBA32F:
		return width * height * depth * 16;
   case TextureFormats::A8:
      return width * height * depth;
	default:
		return 0;
	}
}


uint32 RenderDevice::createTexture( TextureTypes::List type, int width, int height, int depth,
                                    TextureFormats::List format,
                                    bool hasMips, bool genMips, bool compress, bool sRGB )
{
	if( !_caps.texNPOT )
	{
		// Check if texture is NPOT
		if( (width & (width-1)) != 0 || (height & (height-1)) != 0 )
			Modules::log().writeWarning( "Texture has non-power-of-two dimensions although NPOT is not supported by GPU" );
	}
	
	RDITexture tex;
	tex.type = type;
	tex.format = format;
	tex.width = width;
	tex.height = height;
	tex.depth = (type == TextureTypes::Tex3D ? depth : 1);
	tex.sRGB = sRGB && Modules::config().sRGBLinearization;
	tex.genMips = genMips;
	tex.hasMips = hasMips;
	
	switch( format )
	{
	case TextureFormats::BGRA8:
		tex.glFmt = tex.sRGB ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8;
		break;
	case TextureFormats::DXT1:
		tex.glFmt = tex.sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case TextureFormats::DXT3:
		tex.glFmt = tex.sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case TextureFormats::DXT5:
		tex.glFmt = tex.sRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	case TextureFormats::RGBA16F:
		tex.glFmt = GL_RGBA16F_ARB;
		break;
	case TextureFormats::RGBA32F:
		tex.glFmt = GL_RGBA32F_ARB;
		break;
	case TextureFormats::DEPTH:
		tex.glFmt = _depthFormat;
		break;
	case TextureFormats::A8:
		tex.glFmt = GL_INTENSITY8;
		break;
	default:
		ASSERT( 0 );
		break;
	};
	
	glGenTextures( 1, &tex.glObj );
	glActiveTexture( GL_TEXTURE15 );
	glBindTexture( tex.type, tex.glObj );
	
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	glTexParameterfv( tex.type, GL_TEXTURE_BORDER_COLOR, borderColor );

	tex.samplerState = 0;
	applySamplerState( tex );
	
	glBindTexture( tex.type, 0 );
	if( _texSlots[15].texObj )
		glBindTexture( _textures.getRef( _texSlots[15].texObj ).type, _textures.getRef( _texSlots[15].texObj ).glObj );

   validateGLCall("Error creating texture: %d");

   // Calculate memory requirements
	tex.memSize = calcTextureSize( format, width, height, depth );
	if( hasMips || genMips ) tex.memSize += ftoi_r( tex.memSize * 1.0f / 3.0f );
	if( type == TextureTypes::TexCube ) tex.memSize *= 6;
	_textureMem += tex.memSize;
	
	return _textures.add( tex );
}

void RenderDevice::copyTextureDataFromPbo( uint32 texObj, uint32 pboObj, int xOffset, int yOffset, int width, int height )
{
   const RDITexture &tex = _textures.getRef( texObj );
   const RDIBuffer &buf = _buffers.getRef(pboObj);
   int inputFormat = GL_BGRA, inputType = GL_UNSIGNED_BYTE;

   switch( tex.format )
   {
      case TextureFormats::RGBA16F:
         inputFormat = GL_RGBA;
         inputType = GL_FLOAT;
         break;
      case TextureFormats::RGBA32F:
         inputFormat = GL_RGBA;
         inputType = GL_FLOAT;
         break;
      case TextureFormats::DEPTH:
         inputFormat = GL_DEPTH_COMPONENT;
         inputType = GL_FLOAT;
         break;
      case TextureFormats::A8:
        inputFormat = GL_LUMINANCE;
         break;
   };

   ASSERT(width <= tex.width);
   ASSERT(height <= tex.height);

   glBindTexture(GL_TEXTURE_2D, tex.glObj);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf.glObj);

   glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset, width, height, inputFormat, inputType, 0);   
   
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   
   validateGLCall("Error copying texture data from pbo: %d");
}


bool RenderDevice::useGluMipmapFallback()
{
   if (getCaps().glVersion < 33) {
      if (getCaps().cardType == CardType::INTEL)
      {
         return true;
      }
   }
   return false;
}


void RenderDevice::uploadTextureData( uint32 texObj, int slice, int mipLevel, const void *pixels )
{
	const RDITexture &tex = _textures.getRef( texObj );
	TextureFormats::List format = tex.format;

   glActiveTexture( GL_TEXTURE15 );
	glBindTexture( tex.type, tex.glObj );

	int inputFormat = GL_BGRA, inputType = GL_UNSIGNED_BYTE;
	bool compressed = (format == TextureFormats::DXT1) || (format == TextureFormats::DXT3) ||
	                  (format == TextureFormats::DXT5);
	
	switch( format )
	{
	case TextureFormats::RGBA16F:
		inputFormat = GL_RGBA;
		inputType = GL_FLOAT;
		break;
	case TextureFormats::RGBA32F:
		inputFormat = GL_RGBA;
		inputType = GL_FLOAT;
		break;
	case TextureFormats::DEPTH:
		inputFormat = GL_DEPTH_COMPONENT;
		inputType = GL_FLOAT;
      break;
   case TextureFormats::A8:
      inputFormat = GL_LUMINANCE;
      break;
	};
	
	// Calculate size of next mipmap using "floor" convention
	int width = std::max( tex.width >> mipLevel, 1 ), height = std::max( tex.height >> mipLevel, 1 );
	
	if( tex.type == TextureTypes::Tex2D || tex.type == TextureTypes::TexCube )
	{
		int target = (tex.type == TextureTypes::Tex2D) ?
			GL_TEXTURE_2D : (GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice);
		
		if( compressed )
			glCompressedTexImage2D( target, mipLevel, tex.glFmt, width, height, 0,
			                        calcTextureSize( format, width, height, 1 ), pixels );	
		else
			glTexImage2D( target, mipLevel, tex.glFmt, width, height, 0, inputFormat, inputType, pixels );
	}
	else if( tex.type == TextureTypes::Tex3D )
	{
		int depth = std::max( tex.depth >> mipLevel, 1 );
		
		if( compressed )
			glCompressedTexImage3D( GL_TEXTURE_3D, mipLevel, tex.glFmt, width, height, depth, 0,
			                        calcTextureSize( format, width, height, depth ), pixels );	
		else
			glTexImage3D( GL_TEXTURE_3D, mipLevel, tex.glFmt, width, height, depth, 0,
			              inputFormat, inputType, pixels );
	}

	if( tex.genMips && (tex.type != GL_TEXTURE_CUBE_MAP || slice == 5) )
	{
      if (useGluMipmapFallback())
      {
         if (inputFormat == GL_BGRA && inputType == GL_UNSIGNED_BYTE)
         {
            Modules::log().writeInfo("Taking glu path for mipmap generation.");
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA , width, height, inputFormat, GL_UNSIGNED_BYTE, pixels);
         } else {
            Modules::log().writeWarning("Attempting risky mipmap generation (%d, %d, %x, %x)", width, height, inputFormat, inputType);
		      glEnable( tex.type );  // Workaround for ATI driver bug
		      glGenerateMipmapEXT( tex.type );
		      glDisable( tex.type );
         }
      } else {
		   glEnable( tex.type );  // Workaround for ATI driver bug
		   glGenerateMipmapEXT( tex.type );
		   glDisable( tex.type );
      }
		// Note: for cube maps mips are only generated when the side with the highest index is uploaded
	}

	glBindTexture( tex.type, 0 );
	if( _texSlots[15].texObj )
		glBindTexture( _textures.getRef( _texSlots[15].texObj ).type, _textures.getRef( _texSlots[15].texObj ).glObj );

   validateGLCall("Error uploading texture data: %d");
}


void RenderDevice::destroyTexture( uint32 texObj )
{
	if( texObj == 0 ) return;
	
	const RDITexture &tex = _textures.getRef( texObj );
	glDeleteTextures( 1, &tex.glObj );

	_textureMem -= tex.memSize;
	_textures.remove( texObj );
}


void RenderDevice::updateTextureData( uint32 texObj, int slice, int mipLevel, const void *pixels )
{
	uploadTextureData( texObj, slice, mipLevel, pixels );
}


bool RenderDevice::getTextureData( uint32 texObj, int slice, int mipLevel, void *buffer )
{
	const RDITexture &tex = _textures.getRef( texObj );
	
	int target = tex.type == TextureTypes::TexCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
	if( target == GL_TEXTURE_CUBE_MAP ) target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + slice;
	
	int fmt, type, compressed = 0;
	glActiveTexture( GL_TEXTURE15 );
	glBindTexture( tex.type, tex.glObj );

	switch( tex.format )
	{
	case TextureFormats::BGRA8:
		fmt = GL_BGRA;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormats::DXT1:
	case TextureFormats::DXT3:
	case TextureFormats::DXT5:
		compressed = 1;
		break;
	case TextureFormats::RGBA16F:
	case TextureFormats::RGBA32F:
		fmt = GL_RGBA;
		type = GL_FLOAT;
		break;
	default:
		return false;
	};

	if( compressed )
		glGetCompressedTexImage( target, mipLevel, buffer );
	else
		glGetTexImage( target, mipLevel, fmt, type, buffer );

	glBindTexture( tex.type, 0 );
	if( _texSlots[15].texObj )
		glBindTexture( _textures.getRef( _texSlots[15].texObj ).type, _textures.getRef( _texSlots[15].texObj ).glObj );

	return true;
}


// =================================================================================================
// Shaders
// =================================================================================================

uint32 RenderDevice::createShaderProgram( const char* filename, const char *vertexShaderSrc, const char *fragmentShaderSrc )
{
	int status;

	// Vertex shader
	uint32 vs = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vs, 1, &vertexShaderSrc, 0x0 );
	glCompileShader( vs );
	glGetShaderiv( vs, GL_COMPILE_STATUS, &status );
	if( !status )
	{
      ReportShaderError(vs, VERTEX_SHADER, filename, vertexShaderSrc, fragmentShaderSrc);
		glDeleteShader( vs );
		return 0;
	}

	// Fragment shader
	uint32 fs = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fs, 1, &fragmentShaderSrc, 0x0 );
	glCompileShader( fs );
	glGetShaderiv( fs, GL_COMPILE_STATUS, &status );
	if( !status )
	{	
      ReportShaderError(fs, FRAGMENT_SHADER, filename, vertexShaderSrc, fragmentShaderSrc);
		glDeleteShader( vs );
		glDeleteShader( fs );
		return 0;
	}

	// Shader program
	uint32 program = glCreateProgram();
	glAttachShader( program, vs );
	glAttachShader( program, fs );
	glDeleteShader( vs );
	glDeleteShader( fs );

	return program;
}


bool RenderDevice::linkShaderProgram( uint32 programObj, const char* filename, const char *vertexShaderSrc, const char *fragmentShaderSrc)
{
	int infologLength = 0;
	int charsWritten = 0;
	char *infoLog = 0x0;
	int status;

	glLinkProgram( programObj );
	glGetProgramiv( programObj, GL_INFO_LOG_LENGTH, &infologLength );
	if( infologLength > 1 )
	{
      ReportShaderError(programObj, PROGRAM, filename, vertexShaderSrc, fragmentShaderSrc);
	}
	
	glGetProgramiv( programObj, GL_LINK_STATUS, &status );
	if( !status ) return false;

	return true;
}


uint32 RenderDevice::createShader( const char* filename, const char *vertexShaderSrc, const char *fragmentShaderSrc )
{
	// Compile and link shader
	uint32 programObj = createShaderProgram( filename, vertexShaderSrc, fragmentShaderSrc );
	if( programObj == 0 ) return 0;
	if( !linkShaderProgram( programObj, filename, vertexShaderSrc, fragmentShaderSrc ) ) return 0;
	
	uint32 shaderId = _shaders.add( RDIShader() );
	RDIShader &shader = _shaders.getRef( shaderId );
	shader.oglProgramObj = programObj;
	
	int attribCount;
	glGetProgramiv( programObj, GL_ACTIVE_ATTRIBUTES, &attribCount );
	
	for( uint32 i = 0; i < _numVertexLayouts; ++i )
	{
		RDIVertexLayout &vl = _vertexLayouts[i];
		bool allAttribsFound = true;
		
		for( uint32 j = 0; j < 16; ++j )
			shader.inputLayouts[i].attribIndices[j] = -1;
		
		for( int j = 0; j < attribCount; ++j )
		{
			char name[32];
			uint32 size, type;
			glGetActiveAttrib( programObj, j, 32, 0x0, (int *)&size, &type, name );

			bool attribFound = false;
			for( uint32 k = 0; k < vl.numAttribs; ++k )
			{
				if( vl.attribs[k].semanticName.compare(name) == 0 )
				{
					shader.inputLayouts[i].attribIndices[k] = glGetAttribLocation( programObj, name );
					attribFound = true;
				}
			}

			if( !attribFound )
			{
				allAttribsFound = false;
				break;
			}
		}

		shader.inputLayouts[i].valid = allAttribsFound;
	}

	return shaderId;
}


void RenderDevice::destroyShader( uint32 shaderId )
{
	if( shaderId == 0 ) return;

	RDIShader &shader = _shaders.getRef( shaderId );
	glDeleteProgram( shader.oglProgramObj );
	_shaders.remove( shaderId );
}


void RenderDevice::bindShader( uint32 shaderId )
{
	if( shaderId != 0 )
	{
		RDIShader &shader = _shaders.getRef( shaderId );
		glUseProgram( shader.oglProgramObj );
	}
	else
	{
		glUseProgram( 0 );
	}
	
	_curShaderId = shaderId;
	_pendingMask |= PM_VERTLAYOUT;
} 


int RenderDevice::getShaderConstLoc( uint32 shaderId, const char *name )
{
	RDIShader &shader = _shaders.getRef( shaderId );
	return glGetUniformLocation( shader.oglProgramObj, name );
}


int RenderDevice::getShaderSamplerLoc( uint32 shaderId, const char *name )
{
	RDIShader &shader = _shaders.getRef( shaderId );
	return glGetUniformLocation( shader.oglProgramObj, name );
}


void RenderDevice::setShaderConst( int loc, RDIShaderConstType type, void *values, uint32 count )
{
	switch( type )
	{
	case CONST_FLOAT:
		glUniform1fv( loc, count, (float *)values );
		break;
	case CONST_FLOAT2:
		glUniform2fv( loc, count, (float *)values );
		break;
	case CONST_FLOAT3:
		glUniform3fv( loc, count, (float *)values );
		break;
	case CONST_FLOAT4:
		glUniform4fv( loc, count, (float *)values );
		break;
	case CONST_FLOAT44:
		glUniformMatrix4fv( loc, count, false, (float *)values );
		break;
	case CONST_FLOAT33:
		glUniformMatrix3fv( loc, count, false, (float *)values );
		break;
	}
}


void RenderDevice::setShaderSampler( int loc, uint32 texUnit )
{
	glUniform1i( loc, (int)texUnit );
}


// =================================================================================================
// Renderbuffers
// =================================================================================================

uint32 RenderDevice::createRenderBuffer( uint32 width, uint32 height, TextureFormats::List format,
                                         bool depth, uint32 numColBufs, uint32 samples, uint32 numMips )
{
	if( (format == TextureFormats::RGBA16F || format == TextureFormats::RGBA32F) && !_caps.texFloat )
	{
		return 0;
	}

	if( numColBufs > RDIRenderBuffer::MaxColorAttachmentCount ) return 0;

	uint32 maxSamples = 0;
	if( _caps.rtMultisampling )
	{
		GLint value;
		glGetIntegerv( GL_MAX_SAMPLES_EXT, &value );
		maxSamples = (uint32)value;
	}
	if( samples > maxSamples )
	{
		samples = maxSamples;
		Modules::log().writeWarning( "GPU does not support desired multisampling quality for render target" );
	}

	RDIRenderBuffer rb;
	rb.width = width;
	rb.height = height;
	rb.samples = samples;

	// Create framebuffers
	glGenFramebuffersEXT( 1, &rb.fbo );
	if( samples > 0 ) 
   {
      glGenFramebuffersEXT( 1, &rb.fboMS );
   }

	if( numColBufs > 0 )
	{
		// Attach color buffers
		for( uint32 j = 0; j < numColBufs; ++j )
		{
			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
			// Create a color texture

			uint32 texObj = createTexture( TextureTypes::Tex2D, rb.width, rb.height, 1, format, numMips > 0 ? true : false, numMips > 0 ? true : false, false, false );
			ASSERT( texObj != 0 );
			uploadTextureData( texObj, 0, 0, 0x0 );
			rb.colTexs[j] = texObj;
			RDITexture &tex = _textures.getRef( texObj );
			// Attach the texture
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j, GL_TEXTURE_2D, tex.glObj, 0 );

			if( samples > 0 )
			{
				glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
				// Create a multisampled renderbuffer
				glGenRenderbuffersEXT( 1, &rb.colBufs[j] );
				glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, rb.colBufs[j] );
				glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, rb.samples, tex.glFmt, rb.width, rb.height );
				// Attach the renderbuffer
				glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + j,
				                              GL_RENDERBUFFER_EXT, rb.colBufs[j] );
			}
		}

		uint32 buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, 
         GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT, GL_COLOR_ATTACHMENT4_EXT, 
         GL_COLOR_ATTACHMENT5_EXT, GL_COLOR_ATTACHMENT6_EXT, GL_COLOR_ATTACHMENT7_EXT,
         GL_COLOR_ATTACHMENT8_EXT, GL_COLOR_ATTACHMENT9_EXT, GL_COLOR_ATTACHMENT10_EXT,
         GL_COLOR_ATTACHMENT11_EXT, GL_COLOR_ATTACHMENT12_EXT, GL_COLOR_ATTACHMENT13_EXT,
         GL_COLOR_ATTACHMENT14_EXT, GL_COLOR_ATTACHMENT15_EXT};
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
		glDrawBuffers( numColBufs, buffers );
		
		if( samples > 0 )
		{
			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
			glDrawBuffers( numColBufs, buffers );
		}
	}

	// Attach depth buffer
	if( depth )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
      if (numColBufs == 0) {
		   glDrawBuffer( GL_NONE );
		   glReadBuffer( GL_NONE );
      }
		// Create a depth texture
		uint32 texObj = createTexture( TextureTypes::Tex2D, rb.width, rb.height, 1, TextureFormats::DEPTH, false, false, false, false );
		ASSERT( texObj != 0 );

      RDITexture &tex = _textures.getRef(texObj);
      glBindTexture(GL_TEXTURE_2D, tex.glObj);
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
      glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
      glBindTexture(GL_TEXTURE_2D, 0);
		
      uploadTextureData( texObj, 0, 0, 0x0 );
		rb.depthTex = texObj;
		// Attach the texture
		glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, tex.glObj, 0 );

		if( samples > 0 )
		{
			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
         if (numColBufs == 0) {
		      glDrawBuffer( GL_NONE );
		      glReadBuffer( GL_NONE );
         }
			// Create a multisampled renderbuffer
			glGenRenderbuffersEXT( 1, &rb.depthBuf );
			glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, rb.depthBuf );
			glRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, rb.samples, _depthFormat, rb.width, rb.height );
			// Attach the renderbuffer
			glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
			                              GL_RENDERBUFFER_EXT, rb.depthBuf );
		}
	}

	uint32 rbObj = _rendBufs.add( rb );
	
	// Check if FBO is complete
	bool valid = true;
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
	uint32 status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	if( status != GL_FRAMEBUFFER_COMPLETE_EXT ) 
   {
      Modules::log().writeError("Unable to create fbo: %d, %d", status, rb.fbo);
      valid = false;
   }
	
	if( samples > 0 )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS );
		status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		if( status != GL_FRAMEBUFFER_COMPLETE_EXT ) 
      {
         Modules::log().writeError("Unable to create fbo (multisample).");
         valid = false;
      }
	}

	if( !valid )
	{
		destroyRenderBuffer( rbObj );
		return 0;
	}
	
	return rbObj;
}


void RenderDevice::destroyRenderBuffer( uint32 rbObj )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
	
	if( rb.depthTex != 0 ) destroyTexture( rb.depthTex );
	if( rb.depthBuf != 0 ) glDeleteRenderbuffersEXT( 1, &rb.depthBuf );
	rb.depthTex = rb.depthBuf = 0;
		
	for( uint32 i = 0; i < RDIRenderBuffer::MaxColorAttachmentCount; ++i )
	{
		if( rb.colTexs[i] != 0 ) destroyTexture( rb.colTexs[i] );
		if( rb.colBufs[i] != 0 ) glDeleteRenderbuffersEXT( 1, &rb.colBufs[i] );
		rb.colTexs[i] = rb.colBufs[i] = 0;
	}

	if( rb.fbo != 0 ) 
   {
      glDeleteFramebuffersEXT( 1, &rb.fbo );
   }
	if( rb.fboMS != 0 )
   {
      glDeleteFramebuffersEXT( 1, &rb.fboMS );
   }
	rb.fbo = rb.fboMS = 0;

	_rendBufs.remove( rbObj );
}


uint32 RenderDevice::getRenderBufferTex( uint32 rbObj, uint32 bufIndex )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
	if( bufIndex < RDIRenderBuffer::MaxColorAttachmentCount ) return rb.colTexs[bufIndex];
	else if( bufIndex == 32 ) return rb.depthTex;
	else return 0;
}


void RenderDevice::resolveRenderBuffer( uint32 rbObj )
{
	RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
	
	if( rb.fboMS == 0 )
   {
      return;
   }

	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, rb.fboMS );
	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, rb.fbo );

	bool depthResolved = false;
	for( uint32 i = 0; i < RDIRenderBuffer::MaxColorAttachmentCount; ++i )
	{
		if( rb.colBufs[i] != 0 )
		{
			glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
			glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
			
			int mask = GL_COLOR_BUFFER_BIT;
			if( !depthResolved && rb.depthBuf != 0 )
			{
				mask |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
				depthResolved = true;
			}
			glBlitFramebufferEXT( 0, 0, rb.width, rb.height, 0, 0, rb.width, rb.height, mask, GL_NEAREST );
		}
	}

	if( !depthResolved && rb.depthBuf != 0 )
	{
		glReadBuffer( GL_NONE );
		glDrawBuffer( GL_NONE );
		glBlitFramebufferEXT( 0, 0, rb.width, rb.height, 0, 0, rb.width, rb.height,
							  GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST );
	}

	glBindFramebufferEXT( GL_READ_FRAMEBUFFER_EXT, 0 );
	glBindFramebufferEXT( GL_DRAW_FRAMEBUFFER_EXT, 0 );

   validateGLCall("Error resolving render buffer: %d");
}


void RenderDevice::setRenderBuffer( uint32 rbObj )
{
	// Resolve render buffer if necessary
	if(_curRendBuf != 0)
   {
      resolveRenderBuffer(_curRendBuf);
   }
	
	// Set new render buffer
	_curRendBuf = rbObj;
	
	if( rbObj == 0 )
	{
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		glDrawBuffer( _outputBufferIndex == 1 ? GL_BACK_RIGHT : GL_BACK_LEFT );
		_fbWidth = _vpWidth + _vpX;
		_fbHeight = _vpHeight + _vpY;

      if (_msaaFramebuffer && _caps.rtMultisampling) {
   		glEnable( GL_MULTISAMPLE );
      } else {
   		glDisable( GL_MULTISAMPLE );
      }
	}
	else
	{
		// Unbind all textures to make sure that no FBO attachment is bound any more
		for( uint32 i = 0; i < 16; ++i ) 
      {
         setTexture( i, 0, 0 );
      }
		commitStates( PM_TEXTURES );
		
		RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fboMS != 0 ? rb.fboMS : rb.fbo );
      int status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

      if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
      {
         Modules::log().writeError("Bind Frambuffer error: %d, %d, %x, %d", rb.fbo, rb.fboMS, status, glGetError());
      }
		ASSERT(status == GL_FRAMEBUFFER_COMPLETE_EXT);
		_fbWidth = rb.width;
		_fbHeight = rb.height;

		if( rb.fboMS != 0 )
      {
         glEnable( GL_MULTISAMPLE );
      } else {
         glDisable( GL_MULTISAMPLE );
      }
	}
}


bool RenderDevice::getRenderBufferData( uint32 rbObj, int bufIndex, int *width, int *height,
                                        int *compCount, void *dataBuffer, int bufferSize )
{
	int x, y, w, h;
	int format = GL_RGBA;
	int type = GL_FLOAT;

	glPixelStorei( GL_PACK_ALIGNMENT, 4 );
	
	if( rbObj == 0 )
	{
		if( bufIndex != 32 && bufIndex != 0 ) return false;
		if( width != 0x0 ) *width = _vpWidth;
		if( height != 0x0 ) *height = _vpHeight;
		
		x = _vpX; y = _vpY; w = _vpWidth; h = _vpHeight;

		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		if( bufIndex != 32 ) glReadBuffer( GL_BACK_LEFT );
		//format = GL_BGRA;
		//type = GL_UNSIGNED_BYTE;
	}
	else
	{
		resolveRenderBuffer( rbObj );
		RDIRenderBuffer &rb = _rendBufs.getRef( rbObj );
		
		if( bufIndex == 32 && rb.depthTex == 0 ) return false;
		if( bufIndex != 32 )
		{
			if( (unsigned)bufIndex >= RDIRenderBuffer::MaxColorAttachmentCount || rb.colTexs[bufIndex] == 0 )
				return false;
		}
		if( width != 0x0 ) *width = rb.width;
		if( height != 0x0 ) *height = rb.height;

		x = 0; y = 0; w = rb.width; h = rb.height;
		
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, rb.fbo );
		if( bufIndex != 32 ) glReadBuffer( GL_COLOR_ATTACHMENT0_EXT + bufIndex );
	}

	if( bufIndex == 32 )
	{	
		format = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
	}
	
	int comps = (bufIndex == 32 ? 1 : 4);
	if( compCount != 0x0 ) *compCount = comps;
	
	if( dataBuffer == 0x0 ) return true;
	if( bufferSize < w * h * comps * (type == GL_FLOAT ? 4 : 1) ) return false;
	
	glFinish();
	glReadPixels( x, y, w, h, format, type, dataBuffer );
	
	return true;
}


// =================================================================================================
// Queries
// =================================================================================================

uint32 RenderDevice::createOcclusionQuery()
{
	uint32 queryObj;
	glGenQueries( 1, &queryObj );
	return queryObj;
}


void RenderDevice::destroyQuery( uint32 queryObj )
{
	if( queryObj == 0 ) return;
	
	glDeleteQueries( 1, &queryObj );
}


void RenderDevice::beginQuery( uint32 queryObj )
{
	glBeginQuery( GL_SAMPLES_PASSED, queryObj );
}


void RenderDevice::endQuery( uint32 /*queryObj*/ )
{
	glEndQuery( GL_SAMPLES_PASSED );
}


uint32 RenderDevice::getQueryResult( uint32 queryObj )
{
	uint32 samples = 0;
	glGetQueryObjectuiv( queryObj, GL_QUERY_RESULT, &samples );
	return samples;
}


// =================================================================================================
// Internal state management
// =================================================================================================

void RenderDevice::checkGLError()
{
	uint32 error = glGetError();
	ASSERT( error != GL_INVALID_ENUM );
	ASSERT( error != GL_INVALID_VALUE );
	ASSERT( error != GL_INVALID_OPERATION );
	ASSERT( error != GL_OUT_OF_MEMORY );
	ASSERT( error != GL_STACK_OVERFLOW && error != GL_STACK_UNDERFLOW );
}


bool RenderDevice::applyVertexLayout()
{
   radiant::perfmon::TimelineCounterGuard avl("applyVertexLayout");
	if( _newVertLayout == 0 || _curShaderId == 0 ) return false;

	RDIVertexLayout &vl = _vertexLayouts[_newVertLayout - 1];
	RDIShader &shader = _shaders.getRef( _curShaderId );
	RDIInputLayout &inputLayout = shader.inputLayouts[_newVertLayout - 1];
	
	if( !inputLayout.valid )
		return false;

	// Set vertex attrib pointers
	uint32 newVertexAttribMask = 0;
	for( uint32 i = 0; i < vl.numAttribs; ++i )
	{
		int8 attribIndex = inputLayout.attribIndices[i];
		if( attribIndex >= 0 )
		{
			VertexLayoutAttrib &attrib = vl.attribs[i];
			const RDIVertBufSlot &vbSlot = _vertBufSlots[attrib.vbSlot];
			
			ASSERT( _buffers.getRef( _vertBufSlots[attrib.vbSlot].vbObj ).glObj != 0 &&
			        _buffers.getRef( _vertBufSlots[attrib.vbSlot].vbObj ).type == GL_ARRAY_BUFFER );
			
			glBindBuffer( GL_ARRAY_BUFFER, _buffers.getRef( _vertBufSlots[attrib.vbSlot].vbObj ).glObj );

         if (_caps.hasInstancing) {
            int numPositions = attrib.size / 4;
            if (numPositions == 0) {
               numPositions = 1;
            }
            // If we have more than one position to fill, assume we're filling an entire vec4.
            // (The current vertex layout only gives size, so we can't tell a 4x3 from a 3x4).
            int realSize = numPositions > 1 ? 4 : attrib.size;
            for ( int curPos = 0; curPos < numPositions; curPos++) {
			      glVertexAttribPointer( attribIndex + curPos, realSize, GL_FLOAT, GL_FALSE,
			                             vbSlot.stride, 
                                      (char *)0 + vbSlot.offset + attrib.offset + (curPos * 4 * sizeof(float)));

               glVertexAttribDivisor(attribIndex + curPos, vl.divisors[i].divisor);

               newVertexAttribMask |= 1 << (attribIndex + curPos);
            }
         } else {
            glVertexAttribPointer( attribIndex, attrib.size, GL_FLOAT, GL_FALSE,
			                           vbSlot.stride, 
                                    (char *)0 + vbSlot.offset + attrib.offset);

            newVertexAttribMask |= 1 << attribIndex;
         }
		}
	}
	
	for( uint32 i = 0; i < 16; ++i )
	{
		uint32 curBit = 1 << i;
		if( (newVertexAttribMask & curBit) != (_activeVertexAttribsMask & curBit) )
		{
			if( newVertexAttribMask & curBit ) 
         {
            glEnableVertexAttribArray( i );
         } else {
            glDisableVertexAttribArray( i );
         }
		}
	}
	_activeVertexAttribsMask = newVertexAttribMask;

	return true;
}


void RenderDevice::applySamplerState( RDITexture &tex )
{
	uint32 state = tex.samplerState;
	uint32 target = tex.type;
	
	const uint32 magFilters[] = { GL_LINEAR, GL_LINEAR, GL_NEAREST, 0, GL_NEAREST  };
	const uint32 minFiltersMips[] = { GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST_MIPMAP_NEAREST, 0, GL_LINEAR_MIPMAP_LINEAR };
	const uint32 maxAniso[] = { 1, 2, 4, 0, 8, 0, 0, 0, 16 };
	const uint32 wrapModes[] = { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_CLAMP_TO_BORDER };

	if( tex.hasMips )
		glTexParameteri( target, GL_TEXTURE_MIN_FILTER, minFiltersMips[(state & SS_FILTER_MASK) >> SS_FILTER_START] );
	else
		glTexParameteri( target, GL_TEXTURE_MIN_FILTER, magFilters[(state & SS_FILTER_MASK) >> SS_FILTER_START] );

	glTexParameteri( target, GL_TEXTURE_MAG_FILTER, magFilters[(state & SS_FILTER_MASK) >> SS_FILTER_START] );
	glTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso[(state & SS_ANISO_MASK) >> SS_ANISO_START] );
	glTexParameteri( target, GL_TEXTURE_WRAP_S, wrapModes[(state & SS_ADDRU_MASK) >> SS_ADDRU_START] );
	glTexParameteri( target, GL_TEXTURE_WRAP_T, wrapModes[(state & SS_ADDRV_MASK) >> SS_ADDRV_START] );
	glTexParameteri( target, GL_TEXTURE_WRAP_R, wrapModes[(state & SS_ADDRW_MASK) >> SS_ADDRW_START] );
	
	if( !(state & SS_COMP_LEQUAL) )
	{
		glTexParameteri( target, GL_TEXTURE_COMPARE_MODE, GL_NONE );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );
	}

}


bool RenderDevice::commitStates( uint32 filter )
{
   radiant::perfmon::TimelineCounterGuard cs("commitStates");
	if( _pendingMask & filter )
	{
		uint32 mask = _pendingMask & filter;
	
		// Set viewport
		if( mask & PM_VIEWPORT )
		{
			glViewport( _vpX, _vpY, _vpWidth, _vpHeight );
			_pendingMask &= ~PM_VIEWPORT;
		}

		// Set scissor rect
		if( mask & PM_SCISSOR )
		{
			glScissor( _scX, _scY, _scWidth, _scHeight );
			_pendingMask &= ~PM_SCISSOR;
		}
		
		// Bind index buffer
		if( mask & PM_INDEXBUF )
		{
			if( _newIndexBuf != _curIndexBuf )
			{
				if( _newIndexBuf != 0 )
					glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _buffers.getRef( _newIndexBuf ).glObj );
				else
					glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
				
				_curIndexBuf = _newIndexBuf;
				_pendingMask &= ~PM_INDEXBUF;
			}
		}
		
		// Bind vertex buffers
		if( mask & PM_VERTLAYOUT )
		{
			//if( _newVertLayout != _curVertLayout || _curShader != _prevShader )
			{
				if( !applyVertexLayout() )
					return false;
				_curVertLayout = _newVertLayout;
				_prevShaderId = _curShaderId;
				_pendingMask &= ~PM_VERTLAYOUT;
			}
		}

		// Bind textures and set sampler state
		if( mask & PM_TEXTURES )
		{
			for( uint32 i = 0; i < 16; ++i )
			{
				glActiveTexture( GL_TEXTURE0 + i );

				if( _texSlots[i].texObj != 0 )
				{
					RDITexture &tex = _textures.getRef( _texSlots[i].texObj );
					glBindTexture( tex.type, tex.glObj );

					// Apply sampler state
					if( tex.samplerState != _texSlots[i].samplerState )
					{
						tex.samplerState = _texSlots[i].samplerState;
						applySamplerState( tex );
					}
				}
				else
				{
					glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
					glBindTexture( GL_TEXTURE_3D, 0 );
					glBindTexture( GL_TEXTURE_2D, 0 );
				}
			}
			
			_pendingMask &= ~PM_TEXTURES;
		}

		CHECK_GL_ERROR
	}

	return true;
}


void RenderDevice::resetStates()
{
	static int maxVertexAttribs = 0;
	if( maxVertexAttribs == 0 )
		glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs );
	
	_curIndexBuf = 1; _newIndexBuf = 0;
	_curVertLayout = 1; _newVertLayout = 0;

	for( uint32 i = 0; i < 16; ++i )
		setTexture( i, 0, 0 );

	_activeVertexAttribsMask = 0;

	for( uint32 i = 0; i < (uint32)maxVertexAttribs; ++i )
		glDisableVertexAttribArray( i );

	_pendingMask = 0xFFFFFFFF;
	commitStates();
}


// =================================================================================================
// Draw calls and clears
// =================================================================================================

void RenderDevice::clear( uint32 flags, float *colorRGBA, float depth )
{
	uint32 mask = 0;
	
	if( flags & CLR_DEPTH )
	{
		mask |= GL_DEPTH_BUFFER_BIT;
		glClearDepth( depth );
	}
	if( flags & CLR_COLOR )
	{
		mask |= GL_COLOR_BUFFER_BIT;
		if( colorRGBA ) glClearColor( colorRGBA[0], colorRGBA[1], colorRGBA[2], colorRGBA[3] );
		else glClearColor( 0, 0, 0, 0 );
	}
	
	if( mask )
	{	
		commitStates( PM_VIEWPORT | PM_SCISSOR );
		glClear( mask );
	}

	CHECK_GL_ERROR
}


void RenderDevice::draw( RDIPrimType primType, uint32 firstVert, uint32 numVerts )
{
	if( commitStates() )
	{
		glDrawArrays( (uint32)primType, firstVert, numVerts );
	}

	CHECK_GL_ERROR
}


void RenderDevice::drawIndexed( RDIPrimType primType, uint32 firstIndex, uint32 numIndices,
                                uint32 firstVert, uint32 numVerts )
{
	if( commitStates() )
	{
		firstIndex *= (_indexFormat == IDXFMT_16) ? sizeof( short ) : sizeof( int );
		
      radiant::perfmon::TimelineCounterGuard di("drawIndexed");

		glDrawRangeElements( (uint32)primType, firstVert, firstVert + numVerts,
		                     numIndices, _indexFormat, (char *)0 + firstIndex );
	}

	CHECK_GL_ERROR
}

void RenderDevice::drawInstanced( RDIPrimType primType, uint32 count, uint32 firstIndex, GLsizei primcount)
{
	if( commitStates() )
	{
		firstIndex *= (_indexFormat == IDXFMT_16) ? sizeof( short ) : sizeof( int );

      glDrawElementsInstanced((uint32)primType, count, _indexFormat, (char *)0 + firstIndex, primcount);
	}

	CHECK_GL_ERROR
}

void RenderDevice::ReportShaderError(uint32 shader_id, ShaderType type, const char* filename, const char* vs, const char* fs)
{
	int info_len = 0;
   bool program_shader = type == PROGRAM;
   const char *src = NULL;
   std::ostringstream program_src_stream;
   std::string program_src;

   if (type == PROGRAM) {
      program_src_stream << "Vertex Info" << std::endl << vs << std::endl;
      program_src_stream << "Fragment Info" << std::endl << fs << std::endl;
      program_src = program_src_stream.str();
      src = program_src.c_str();
   } else if (type == VERTEX_SHADER) { 
      src = vs;
   } else if (type == FRAGMENT_SHADER) {
      src = fs;
   }

	// Get info
	(program_shader ? glGetProgramiv : glGetShaderiv)(shader_id, GL_INFO_LOG_LENGTH, &info_len);
	if (info_len == 0) {
      ::radiant::om::ErrorBrowser::Record record;

      std::ostringstream s;
      s << "unknown error compling shader";
      record.SetCategory(record.SEVERE);
      record.SetSummary(s.str());
      if (filename) {
         record.SetFilename(filename);
      }
      Modules::log().ReportError(record);
      return;
   }
   int c = 0;
   char *info = NULL;

   info = new char[info_len + 1];
   if (info) {
		(program_shader ? glGetProgramInfoLog : glGetShaderInfoLog)(shader_id, info_len, &c, info);

      if (strncmp(info, "No errors.", 10) != 0)
      {
         Modules::log().writeError("raw shader compile error: %s", info);

         std::vector<std::string> lines;
         boost::split(lines, info, boost::is_any_of("\n\r"));
         lines.erase(std::remove(lines.begin(), lines.end(), ""), lines.end());

         for (std::string const& line : lines) {
            ::radiant::om::ErrorBrowser::Record record;

            std::regex exp("(\\d+)\\((\\d+)\\)(.*)");
            std::smatch match;
            if (line == "Vertex info") {
               type = VERTEX_SHADER;
               src = vs;
            } else if (line == "Fragment info") {
               type = FRAGMENT_SHADER;
               src = fs;
            } else if (std::regex_match(line, match, exp)) {
               int char_offset = atoi(match[1].str().c_str());
               int line_number = atoi(match[2].str().c_str());
               std::string summary = match[3].str();

               record.SetSummary(summary);

               if (strstr(line.c_str(), "error")) {
                  record.SetCategory(record.SEVERE);
               } else {
                  record.SetCategory(record.WARNING);
               }

               if (filename) {
                  record.SetFilename(filename);
               }
               if (src) {
                  record.SetFileContent(src);
                  if (type != PROGRAM) {
                     record.SetCharOffset(char_offset);
                     record.SetLineNumber(line_number);
                  }
               }
               Modules::log().ReportError(record);
            }
         }
      }
      delete [] info;
   }
}

const bool RenderDevice::getShadowOffsets(float* factor, float* units) {
   if (factor)
   {
      *factor = _shadowFactor; 
   }
   if (units)
   {
      *units = _shadowUnits;
   }
   return _shadowFactor != 0.0f || _shadowUnits != 0.0f;
}


}  // namespace
