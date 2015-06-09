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

#define _CRT_SECURE_NO_DEPRECATE
#include "Horde3D.h"
#include "egModules.h"
#include "egVoxelGeometry.h"
#include "egRenderer.h"
#include "utPlatform.h"
#include "utMath.h"
#include <math.h>
#ifdef PLATFORM_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#	   define WIN32_LEAN_AND_MEAN 1
#  endif
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#endif
#ifndef PLATFORM_MAC
#	include <GL/gl.h>
#endif
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>

#include "radiant.h"
#include "core/config.h"
#include <boost/algorithm/string/predicate.hpp>
#include <png.h>
#include <png.h>

using namespace Horde3D;
using namespace std;

#ifdef __MINGW32__
#undef PLATFORM_WIN
#endif

#define R_LOG(level)    LOG(horde.renderer, level)

static bool h3dutWritePNGImage(FILE *fp, const unsigned char *pixels, int width, int height);

namespace Horde3DUtils {

struct InfoBox
{
	H3DRes  fontMatRes;
	float   x, y_row0;
	float   width;
	int     row;
} infoBox;

ofstream            outf;
map< int, std::string >  resourcePaths;

#ifdef PLATFORM_WIN
HDC    hDC = 0;
HGLRC  hRC = 0;
#endif


std::string cleanPath( std::string path )
{
	// Remove spaces at the beginning
	int cnt = 0;
	for( int i = 0; i < (int)path.length(); ++i )
	{
		if( path[i] != ' ' ) break;
		else ++cnt;
	}
	if( cnt > 0 ) path.erase( 0, cnt );

	// Remove slashes, backslashes and spaces at the end
	cnt = 0;
	for( int i = (int)path.length() - 1; i >= 0; --i )
	{
		if( path[i] != '/' && path[i] != '\\' && path[i] != ' ' ) break;
		else ++cnt;
	}

	if( cnt > 0 ) path.erase( path.length() - cnt, cnt );

	return path;
}

}  // namespace


// =================================================================================================
// Exported API functions
// =================================================================================================

using namespace Horde3DUtils;

// TODO: Make OpenGL functions platform independent

DLLEXP bool h3dutInitOpenGL( int hdc )
{
#ifdef PLATFORM_WIN
	hDC = (HDC)(__int64)hdc;
	
	// Init OpenGL rendering context
	int pixelFormat;

	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof( PIXELFORMATDESCRIPTOR ),            // Size of this pixel format descriptor
		1,                                          // Version number
		PFD_DRAW_TO_WINDOW |                        // Format must support window
		PFD_SUPPORT_OPENGL |                        // Format must support OpenGL
		PFD_DOUBLEBUFFER,                           // Must support double buffering
		PFD_TYPE_RGBA,                              // Request a RGBA format
		32,                                         // Select our color depth
		0, 0, 0, 0, 0, 0,                           // Color bits ignored
		8,                                          // 8Bit alpha buffer
		0,                                          // Shift bit ignored
		0,                                          // No accumulation buffer
		0, 0, 0, 0,                                 // Accumulation bits ignored
		32,                                         // 32Bit z-buffer (depth buffer)  
		8,                                          // 8Bit stencil buffer
		0,                                          // No auxiliary buffer
		PFD_MAIN_PLANE,                             // Main drawing layer
		0,                                          // Reserved
		0, 0, 0                                     // Layer masks ignored
	};

	if( !(pixelFormat = ChoosePixelFormat( hDC, &pfd )) ) 
	{
		return false;
	}

	if( !SetPixelFormat( hDC, pixelFormat, &pfd ) ) 
	{
		return false;
	}

	if( !(hRC = wglCreateContext( hDC )) ) 
	{
		return false;
	}

	if( !wglMakeCurrent( hDC, hRC ) ) 
	{
		wglDeleteContext( hRC );
		return false;
	}

	return true;
	
#else
	return false;
#endif
}


DLLEXP void h3dutReleaseOpenGL()
{
#ifdef PLATFORM_WIN
	if( hDC == 0 || hRC == 0 ) return;

	if( !wglMakeCurrent( 0x0, 0x0 ) ) 
	{
		return;
	}
	if( !wglDeleteContext( hRC ) ) 
	{
		return;
	}
	hRC = 0;
#endif
}


DLLEXP void h3dutSwapBuffers()
{
#ifdef PLATFORM_WIN
	if( hDC == 0 || hRC == 0 ) return;

	SwapBuffers( hDC );
#endif
}


DLLEXP const char *h3dutGetResourcePath( int type )
{
	return resourcePaths[type].c_str();
}


DLLEXP void h3dutSetResourcePath( int type, const char *path )
{
	std::string s = path != 0x0 ? path : "";

	resourcePaths[type] = cleanPath( s );
}


DLLEXP bool h3dutLoadResourcesFromDisk( const char *contentDir )
{
	bool result = true;
	std::string dir;
	vector< std::string > dirs;

	// Split path string
	char *c = (char *)contentDir;
	do
	{
		if( *c != '|' && *c != '\0' )
			dir += *c;
		else
		{
			dir = cleanPath( dir );
			if( dir != "" ) dir += '/';
			dirs.push_back( dir );
			dir = "";
		}
	} while( *c++ != '\0' );
	
	// Get the first resource that needs to be loaded
	int res = h3dQueryUnloadedResource( 0 );
	
	char *dataBuf = 0;
	int bufSize = 0;

	while( res != 0 )
	{
		ifstream inf;
		
		// Loop over search paths and try to open files
		for( unsigned int i = 0; i < dirs.size(); ++i )
		{
			std::string fileName = dirs[i] + resourcePaths[h3dGetResType( res )] + "/" + h3dGetResName( res );
			inf.clear();
			inf.open( fileName.c_str(), ios::binary );
			if( inf.good() ) break;
		}

		// Open resource file
		if( inf.good() ) // Resource file found
		{
			// Find size of resource file
			inf.seekg( 0, ios::end );
			int fileSize = (int)inf.tellg();
			if( bufSize < fileSize  )
			{
				delete[] dataBuf;				
				dataBuf = new char[fileSize];
				if( !dataBuf )
				{
					bufSize = 0;
					continue;
				}
				bufSize = fileSize;
			}
			if( fileSize == 0 )	continue;
			// Copy resource file to memory
			inf.seekg( 0 );
			inf.read( dataBuf, fileSize );
			inf.close();
			// Send resource data to engine
			result &= h3dLoadResource( res, dataBuf, fileSize );
		}
		else // Resource file not found
		{
			// Tell engine to use the dafault resource by using NULL as data pointer
			h3dLoadResource( res, 0x0, 0 );
			result = false;
		}
		// Get next unloaded resource
		res = h3dQueryUnloadedResource( 0 );
	}
	delete[] dataBuf;

	return result;
}

DLLEXP void h3dutShowText( const char *text, float x, float y, float size, float colR,
                           float colG, float colB, H3DRes fontMaterialRes )
{
	if( text == 0x0 ) return;
	
	float ovFontVerts[64 * 16];
	float *p = ovFontVerts;
	float pos = 0;
	
	do
	{
		unsigned char ch = (unsigned char)*text++;

		float u0 = 0.0625f * (ch % 16);
		float v0 = 1.0f - 0.0625f * (ch / 16);

		*p++ = x + size * 0.5f * pos;         *p++ = y;         *p++ = u0;            *p++ = v0;
		*p++ = x + size * 0.5f * pos,         *p++ = y + size;  *p++ = u0;            *p++ = v0 - 0.0625f;
		*p++ = x + size * 0.5f * pos + size;  *p++ = y + size;  *p++ = u0 + 0.0625f;  *p++ = v0 - 0.0625f;
		*p++ = x + size * 0.5f * pos + size;  *p++ = y;         *p++ = u0 + 0.0625f;  *p++ = v0;
		
		pos += 1.f;
	} while( *text && pos < 64 );

	h3dShowOverlays( ovFontVerts, (int)pos * 4, colR, colG, colB, 1.f, fontMaterialRes, 0 );
}


void beginInfoBox( float x, float y, float width, int numRows, const char *title,
                   H3DRes fontMaterialRes, H3DRes boxMaterialRes )
{
	float fontSize = 0.03f;
	float barHeight = fontSize + 0.01f;
	float bodyHeight = numRows * 0.035f + 0.005f;
	
	infoBox.fontMatRes = fontMaterialRes;
	infoBox.x = x;
	infoBox.y_row0 = y + barHeight + 0.005f;
	infoBox.width = width;
	infoBox.row = 0;
	
	// Title bar
	float ovTitleVerts[] = { x, y, 0, 1, x, y + barHeight, 0, 0,
	                         x + width, y + barHeight, 1, 0, x + width, y, 1, 1 };
	h3dShowOverlays( ovTitleVerts, 4,  0.15f, 0.23f, 0.31f, 0.8f, boxMaterialRes, 0 );

	// Title text
	h3dutShowText( title, x + 0.005f, y + 0.005f, fontSize, 0.7f, 0.85f, 0.95f, fontMaterialRes );

	// Body
	float yy = y + barHeight;
	float ovBodyVerts[] = { x, yy, 0, 1, x, yy + bodyHeight, 0, 0,
	                        x + width, yy + bodyHeight, 1, 0, x + width, yy, 1, 1 };
	h3dShowOverlays( ovBodyVerts, 4, 0.12f, 0.12f, 0.12f, 0.5f, boxMaterialRes, 0 );
}


void addInfoBoxRow( const char *column1, const char *column2 )
{
	float fontSize = 0.028f;
	float fontWidth = fontSize * 0.5f;
	float x = infoBox.x;
	float y = infoBox.y_row0 + infoBox.row++ * 0.035f;

	// First column
	h3dutShowText( column1, x + 0.005f, y, fontSize, 1, 1, 1, infoBox.fontMatRes );

	// Second column
	x = infoBox.x + infoBox.width - ((strlen( column2 ) - 1) * fontWidth + fontSize);
	h3dutShowText( column2, x - 0.005f, y, fontSize, 1, 1, 1, infoBox.fontMatRes );
}


DLLEXP void h3dCollectDebugFrame()
{
   Horde3D::Modules::renderer().collectOneDebugFrame();
}


DLLEXP int h3dutCreateRenderTarget( int width, int height, H3DFormats::List format,
                                    bool depth, int numColBufs, int samples, int numMips)
{
   TextureFormats::List tf;
   if (format == H3DFormats::TEX_BGRA8) {
      tf = TextureFormats::BGRA8;
   } else if (format == H3DFormats::TEX_RGBA32F) {
      tf = TextureFormats::RGBA32F;
   } else {
      ASSERT(0);
   }
   return gRDI->createRenderBuffer(width, height, tf, depth, numColBufs, samples, numMips);
}

DLLEXP void h3dutShowFrameStats( H3DRes fontMaterialRes, H3DRes boxMaterialRes, int mode )
{
	static stringstream text;
	static float curFPS = 30;
	static float timer = 100;
	static float fps = 30;
	static float frameTime = 0;
	static float animTime = 0;
	static float geoUpdateTime = 0;
	static float particleSimTime = 0;

	// Calculate FPS
	float curFrameTime = h3dGetStat(H3DStats::AverageFrameTime, false);
	curFPS = 1000.0f / curFrameTime;
	
	timer += curFrameTime / 1000.0f;
	if( timer > 0.7f )
	{	
		fps = curFPS;
		frameTime = curFrameTime;
		animTime = h3dGetStat( H3DStats::AnimationTime, false );
		geoUpdateTime = h3dGetStat( H3DStats::GeoUpdateTime, false );
		particleSimTime = h3dGetStat( H3DStats::ParticleSimTime, false );
		timer = 0;
	}
	
	if( mode > 0 )
	{
		// InfoBox
		beginInfoBox( 0.03f, 0.03f, 0.32f, 5, "Frame Stats", fontMaterialRes, boxMaterialRes );
		
		// FPS
		text.str( "" );
		text << fixed << setprecision( 2 ) << fps;
		addInfoBoxRow( "FPS", text.str().c_str() );
		
		// Triangle count
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::TriCount, false );
		addInfoBoxRow( "Tris", text.str().c_str() );
		
		// Number of batches
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::BatchCount, false );
		addInfoBoxRow( "Batches", text.str().c_str() );
		
		// Number of lighting passes
		text.str( "" );
		text << (int)h3dGetStat( H3DStats::LightPassCount, false );
		addInfoBoxRow( "Lights", text.str().c_str() );

      text.str( "" );
      text << (int)h3dGetStat( H3DStats::ShadowPassCount, false );
      addInfoBoxRow( "Shadow Passes", text.str().c_str() );
	}

	if( mode > 1 )
	{
		// Video memory
		beginInfoBox( 0.03f, 0.30f, 0.32f, 3, "VMem", fontMaterialRes, boxMaterialRes );
		
		// Available
		text.str( "" );
      text << h3dGetStat( H3DStats::AvailableGpuMemory, false ) << "mb";
		addInfoBoxRow( "Available", text.str().c_str() );

      // Textures
		text.str( "" );
		text << h3dGetStat( H3DStats::TextureVMem, false ) << "mb";
		addInfoBoxRow( "Textures", text.str().c_str() );
		
		// Geometry
		text.str( "" );
		text << h3dGetStat( H3DStats::GeometryVMem, false ) << "mb";
		addInfoBoxRow( "Geometry", text.str().c_str() );
		
		// CPU time
		beginInfoBox( 0.03f, 0.44f, 0.32f, 4, "CPU Time", fontMaterialRes, boxMaterialRes );
		
		// Frame time
		text.str( "" );
		text << frameTime << "ms";
		addInfoBoxRow( "Frame Total", text.str().c_str() );
		
		// Animation
		text.str( "" );
		text << animTime << "ms";
		addInfoBoxRow( "Animation", text.str().c_str() );

		// Geometry updates
		text.str( "" );
		text << geoUpdateTime << "ms";
		addInfoBoxRow( "Geo Updates", text.str().c_str() );

		// Particle simulation
		text.str( "" );
		text << particleSimTime << "ms";
		addInfoBoxRow( "Particles", text.str().c_str() );
	}
}


DLLEXP void h3dutFreeMem( char **ptr )
{
	if( ptr == 0x0 ) return;
	
	delete[] *ptr; *ptr = 0x0;
}

DLLEXP H3DRes h3dutCreateVoxelGeometryRes( const char *name, struct VoxelGeometryVertex const* vertexData, int vertexOffsets[], unsigned int const* indexData, int indexOffsets[], int numLodLevels)
{
	if( numLodLevels <= 0 || vertexOffsets[1] == 0 || indexOffsets[1] == 0 ) return 0;

	H3DRes res = h3dAddResource(H3DResTypes::VoxelGeometry, name, 0);
   VoxelGeometryResource *geoRes = (VoxelGeometryResource* )Horde3D::Modules::resMan().resolveResHandle( res );
   geoRes->loadData((Horde3D::VoxelVertexData*)vertexData, vertexOffsets, indexData, indexOffsets, numLodLevels);

   return res;
}


DLLEXP H3DRes h3dutCreateGeometryRes( 
	const char *name, 
	int numVertices, int numTriangleIndices,
	float *posData, 
	uint32 *indexData, 
	short *normalData,
	short *tangentData,
	short *bitangentData,
	float *texData1, float *texData2 )
{
	if( numVertices == 0 || numTriangleIndices == 0 ) return 0;

	H3DRes res = h3dAddResource( H3DResTypes::Geometry, name, 0 );
				
	uint32 size = 
		// General data
		4 +					// Horde Flag 
		sizeof( uint32 ) +	// Version 
		sizeof( uint32 ) +	// Joint Count ( will be always set to set to zero because this method does not create joints )
		sizeof( uint32 ) +	// number of streams 
		sizeof( uint32 ) +	// streamsize
		// vertex stream data
		sizeof( uint32 ) +	// stream id 
		sizeof( uint32 ) +	// stream element size 
		numVertices * sizeof( float ) * 3 + // vertices data
	
		// morph targets 
		sizeof( uint32 );	// number of morph targets ( will be always set to zero because this method does not create morph targets )

	if( normalData )
	{
		size += 
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3; // normal data
	}

	if( tangentData && bitangentData )
	{
		size += 
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3 + // tangent data
			// normal stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
			numVertices * sizeof( uint16 ) * 3; // bitangent data
	}

	int numTexSets = 0;
	if( texData1 ) ++numTexSets;
	if( texData2 ) ++numTexSets;

	for( int i = 0; i < numTexSets; ++i )
	{
		size += 
			// texture stream data
			sizeof( uint32 ) +	// stream id 
			sizeof( uint32 ) +	// stream element size 
		numVertices * sizeof( float ) * 2; // texture data
	}

	size += 
		// index stream data
		sizeof( uint32 ) +	// index count 
		numTriangleIndices * sizeof( uint32 ); // index data


	// Create resource data block
	char *data = new char[size];

	char *pData = data;
	// Write Horde flag
	pData[0] = 'H'; pData[1] = '3'; pData[2] = 'D'; pData[3] = 'G'; pData += 4;
	// Set version to 5 
	*( (uint32 *)pData ) = 5; pData += sizeof( uint32 );
	// Set joint count (zero for this method)
	*( (uint32 *)pData ) = 0; pData += sizeof( uint32 );
	// Set number of streams
	*( (uint32 *)pData ) = 1 + numTexSets + ( normalData ? 1 : 0 ) + ((tangentData && bitangentData) ? 2 : 0); pData += sizeof( uint32 );
	// Write number of elements in each stream
	*( (uint32 *)pData ) = numVertices; pData += sizeof( uint32 );

	// Beginning of stream data

	// Vertex Stream ID
	*( (uint32 *)pData ) = 0; pData += sizeof( uint32 );
	// set vertex stream element size
	*( (uint32 *)pData ) = sizeof( float ) * 3; pData += sizeof( uint32 );
	// vertex data
	memcpy( (float*) pData, posData, numVertices * sizeof( float ) * 3 );
	pData += numVertices * sizeof( float ) * 3;

	if( normalData )
	{
		// Normals Stream ID
		*( (uint32 *)pData ) = 1; pData += sizeof( uint32 );
		// set normal stream element size
		*( (uint32 *)pData ) = sizeof( short ) * 3; pData += sizeof( uint32 );
		// normal data
		memcpy( (short*) pData, normalData, numVertices * sizeof( short ) * 3 );
		pData += numVertices * sizeof( short ) * 3;
	}

	if( tangentData && bitangentData )
	{
		// Tangent Stream ID
		*( (uint32 *)pData ) = 2; pData += sizeof( uint32 );
		// set tangent stream element size
		*( (uint32 *)pData ) = sizeof( short ) * 3; pData += sizeof( uint32 );
		// tangent data
		memcpy( (short*) pData, tangentData, numVertices * sizeof( short ) * 3 );
		pData += numVertices * sizeof( short ) * 3;
	
		// Bitangent Stream ID
		*( (uint32 *)pData ) = 3; pData += sizeof( uint32 );
		// set bitangent stream element size
		*( (uint32 *)pData ) = sizeof( short ) * 3; pData += sizeof( uint32 );
		// bitangent data
		memcpy( (short*) pData, bitangentData, numVertices * sizeof( short ) * 3 );
		pData += numVertices * sizeof( short ) * 3;
	}

	// texture coordinates stream
	if( texData1 )
	{
		*( (uint32 *)pData ) = 6; pData += sizeof( uint32 ); // Tex Set 1
		*( (uint32 *)pData ) = sizeof( float ) * 2; pData += sizeof( uint32 ); // stream element size
		memcpy( (float *)pData, texData1, sizeof( float ) * 2 * numVertices ); // stream data
		pData += sizeof( float ) * 2 * numVertices; 
	}
	if( texData2 )
	{
		*( (uint32 *)pData ) = 7; pData += sizeof( uint32 ); // Tex Set 2
		*( (uint32 *)pData ) = sizeof( float ) * 2; pData += sizeof( uint32 ); // stream element size
		memcpy( (float *)pData, texData2, sizeof( float ) * 2 * numVertices ); // stream data
		pData += sizeof( float ) * 2 * numVertices; 
	}

	// Set number of indices
	*( (uint32 *) pData ) = numTriangleIndices; pData += sizeof( uint32 );	
	
	// index data
	memcpy( pData, indexData, numTriangleIndices * sizeof( uint32 ) );
	pData += numTriangleIndices * sizeof( uint32 );				

	// Set number of morph targets to zero
	*( (uint32 *) pData ) = 0;	pData += sizeof( uint32 );

	if ( res ) h3dLoadResource( res, data, size );
	delete[] data;

	return res;
}


static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
   std::string *p = (std::string*)png_get_io_ptr(png_ptr);
   p->insert(p->end(), data, data + length);
}

bool h3dutCreatePNGImage(std::string& result, unsigned char* pixels, int width, int height)
{
   // Here's some code I found on the internet!
   png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   ASSERT(p);
   //TPngDestructor destroyPng(p);
   png_infop info_ptr = png_create_info_struct(p);
   ASSERT(info_ptr);
   ASSERT(0 == setjmp(png_jmpbuf(p)));

   png_set_IHDR(p, info_ptr, width, height, 8,
         PNG_COLOR_TYPE_RGB_ALPHA,
         PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_BASE,
         PNG_FILTER_TYPE_BASE);
   png_set_bgr(p);
   //png_set_compression_level(p, 1);
   std::vector<unsigned char*> rows(height);
   for (int y = 0; y < height; ++y) {
      rows[height - y - 1] = pixels + y * width * 4;
   }
   png_set_rows(p, info_ptr, &rows[0]);
   png_set_write_fn(p, &result, PngWriteCallback, NULL);
   png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&p, &info_ptr);
   return true;
}


void h3dutCreatePngImageFromTexture(H3DRes tex, std::string& result)
{
   int width;
   int height;

   h3dGetRenderTextureData(tex, &width, &height, nullptr, nullptr, 0);

   float *pixelsF = new float[width * height * 4];

   h3dGetRenderTextureData(tex, nullptr, nullptr, nullptr, pixelsF, width * height * 16);

   // Convert to BGRA8
   unsigned char *pixels = new unsigned char[width * height * 4];
   for( int y = 0; y < height; ++y )
   {
      for( int x = 0; x < width; ++x )
      {
         int *pixelValue = (int*)&(pixels[(y * width + x) * 4]);
         pixels[(y * width + x) * 4 + 0] = Horde3D::ftoi_r( Horde3D::clamp( pixelsF[(y * width + x) * 4 + 2], 0.f, 1.f ) * 255.f );
         pixels[(y * width + x) * 4 + 1] = Horde3D::ftoi_r( Horde3D::clamp( pixelsF[(y * width + x) * 4 + 1], 0.f, 1.f ) * 255.f );
         pixels[(y * width + x) * 4 + 2] = Horde3D::ftoi_r( Horde3D::clamp( pixelsF[(y * width + x) * 4 + 0], 0.f, 1.f ) * 255.f );
         pixels[(y * width + x) * 4 + 3] = 0;//Horde3D::ftoi_r( Horde3D::clamp( pixelsF[(y * width + x) * 4 + 3], 0.f, 1.f ) * 255.f );
         if (*pixelValue > 0) {
            pixels[(y * width + x) * 4 + 3] = 255;
         }
      }
   }
   delete[] pixelsF;
   h3dutCreatePNGImage(result, pixels, width, height);
   delete[] pixels;
}


bool h3dutWritePNGImage(FILE *fp, const unsigned char *pixels, int width, int height)
{
   png_structp png_ptr;
   png_infop info_ptr;

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr); // user_error_fn, user_warning_fn);
   if (png_ptr == NULL) {
      return false;
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      png_destroy_write_struct(&png_ptr,  NULL);
      return false;
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr))) {
      /* If we get here, we had a problem writing the file */
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return false;
   }

   /* One of the following I/O initialization functions is REQUIRED */

   /* Set up the output control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* This is the hard way */

   /* 5 is reasonable compression, going much higher (9) can yield very slow write times
    * with more complicated images.
    */
   png_set_compression_level(png_ptr, 5);

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
      PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_set_bgr(png_ptr);

#if 0
   /* Optional gamma chunk is strongly suggested if you have any guess
    * as to the correct gamma of the image.
    */
   png_set_gAMA(png_ptr, info_ptr, gamma);
#endif

   /* Other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs */

   /* Note that if sRGB is present the gAMA and cHRM chunks must be ignored
    * on read and, if your application chooses to write them, they must
    * be written in accordance with the sRGB profile
    */

   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* If you want, you can write the info in two steps, in case you need to
    * write your private chunk ahead of PLTE:
    *
    *   png_write_info_before_PLTE(write_ptr, write_info_ptr);
    *   write_my_chunk();
    *   png_write_info(png_ptr, info_ptr);
    *
    * However, given the level of known- and unknown-chunk support in 1.2.0
    * and up, this should no longer be necessary.
    */

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */

   /* The easiest way to write the image (you may have a different memory
    * layout, however, so choose what fits your needs best).  You need to
    * use the first method if you aren't handling interlacing yourself.
    */
   std::vector<png_byte*> row_pointers(height);

   /* Set up pointers into your "image" byte array */
   for (int k = 0; k < height; k++) {
      int row = height - k - 1;
     row_pointers[k] = (png_byte*)(pixels + (row * width * 3));
   }

   /* One of the following output methods is REQUIRED */
   png_write_image(png_ptr, row_pointers.data());

   /* You can write optional chunks like tEXt, zTXt, and tIME at the end
    * as well.  Shouldn't be necessary in 1.2.0 and up as all the public
    * chunks are supported and you can use png_set_unknown_chunks() to
    * register unknown chunks into the info structure to be written out.
    */

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* Clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, &info_ptr);

   /* That's it */
   return true;
}

DLLEXP bool h3dutCreateTGAImage( const unsigned char *pixels, int width, int height, int bpp,
                                 char **outData, int *outSize )
{
	if( pixels == 0x0 || outData == 0x0 || outSize == 0x0 ) return false;
	
	*outData = 0x0; *outSize = 0;
	
	if( bpp != 24 && bpp != 32 ) return false;

	*outSize = width * height * (bpp / 8) + 18;
	char *data = new char[*outSize];
	*outData = data;

	// Build TGA header
	char c;
	short s;
	c = 0;      memcpy( data, &c, 1 ); data += 1;  // idLength
	c = 0;      memcpy( data, &c, 1 ); data += 1;  // colmapType
	c = 2;      memcpy( data, &c, 1 ); data += 1;  // imageType
	s = 0;      memcpy( data, &s, 2 ); data += 2;  // colmapStart
	s = 0;      memcpy( data, &s, 2 ); data += 2;  // colmapLength
	c = 0;      memcpy( data, &c, 1 ); data += 1;  // colmapBits
	s = 0;      memcpy( data, &s, 2 ); data += 2;  // x
	s = 0;      memcpy( data, &s, 2 ); data += 2;  // y
	s = width;  memcpy( data, &s, 2 ); data += 2;  // width
	s = height; memcpy( data, &s, 2 ); data += 2;  // height
	c = bpp;    memcpy( data, &c, 1 ); data += 1;  // bpp
	c = 0;      memcpy( data, &c, 1 ); data += 1;  // imageDesc

	// Copy data
	memcpy( data, pixels, width * height * (bpp / 8) );

	return true;
}


DLLEXP void h3dutPickRay( H3DNode cameraNode, float nwx, float nwy, float *ox, float *oy, float *oz,
                          float *dx, float *dy, float *dz )
{				
	// Transform from normalized window [0, 1] to normalized device coordinates [-1, 1]
	float cx( 2.0f * nwx - 1.0f );
	float cy( 2.0f * nwy - 1.0f );   
	
	// Get projection matrix
	Matrix4f projMat;
	h3dGetCameraProjMat( cameraNode, projMat.x );
	
	// Get camera view matrix
	const float *camTrans;
	h3dGetNodeTransMats( cameraNode, 0x0, &camTrans );		
	Matrix4f viewMat( camTrans );
	viewMat = viewMat.inverted();
	
	// Create inverse view-projection matrix for unprojection
	Matrix4f invViewProjMat = (projMat * viewMat).inverted();

	// Unproject
	Vec4f p0 = invViewProjMat * Vec4f( cx, cy, -1, 1 );
	Vec4f p1 = invViewProjMat * Vec4f( cx, cy, 1, 1 );
	p0.x /= p0.w; p0.y /= p0.w; p0.z /= p0.w;
	p1.x /= p1.w; p1.y /= p1.w; p1.z /= p1.w;
	
	if( h3dGetNodeParamI( cameraNode, H3DCamera::OrthoI ) == 1 )
	{
		float frustumWidth = h3dGetNodeParamF( cameraNode, H3DCamera::RightPlaneF, 0 ) -
		                     h3dGetNodeParamF( cameraNode, H3DCamera::LeftPlaneF, 0 );
		float frustumHeight = h3dGetNodeParamF( cameraNode, H3DCamera::TopPlaneF, 0 ) -
		                      h3dGetNodeParamF( cameraNode, H3DCamera::BottomPlaneF, 0 );
		
		Vec4f p2( cx, cy, 0, 1 );

		p2.x = cx * frustumWidth * 0.5f;
		p2.y = cy * frustumHeight * 0.5f;
		viewMat.x[12] = 0; viewMat.x[13] = 0; viewMat.x[14] = 0;
		p2 = viewMat.inverted() * p2;			

		*ox = camTrans[12] + p2.x;
		*oy = camTrans[13] + p2.y;
		*oz = camTrans[14] + p2.z;
	}
	else
	{
		*ox = camTrans[12];
		*oy = camTrans[13];
		*oz = camTrans[14];
	}
	*dx = p1.x - p0.x;
	*dy = p1.y - p0.y;
	*dz = p1.z - p0.z;
}

DLLEXP H3DNode h3dutPickNode( H3DNode cameraNode, float nwx, float nwy )
{
   H3DNode rootNode = Modules::sceneMan().sceneForNode(cameraNode).getRootNode().getHandle();
   H3DSceneId rootScene = Modules::sceneMan().sceneIdFor(rootNode);

	float ox, oy, oz, dx, dy, dz;
	h3dutPickRay( cameraNode, nwx, nwy, &ox, &oy, &oz, &dx, &dy, &dz );
	
	if( h3dCastRay(rootNode, ox, oy, oz, dx, dy, dz, 1, 0 ) == 0 )
	{
		return 0;
	}
	else
	{
		H3DNode intersectionNode = 0;
		if( h3dGetCastRayResult(rootScene, 0, &intersectionNode, 0, 0, 0 ) )
			return intersectionNode;
		else
			return 0;
	}

}


DLLEXP bool h3dutScreenshot(const char *fname, H3DRes renderTexRes)
{
   if (fname == 0x0) {
      return false;
   }

   int width, height;

   if (renderTexRes) {
      h3dGetRenderTextureData(renderTexRes, &width, &height, 0x0, 0x0, 0);
   } else {
      h3dGetRenderTargetData( 0, "", 0, &width, &height, 0x0, 0x0, 0 );
   }

   float *pixelsF = new float[width * height * 4];

   if (renderTexRes) {
      h3dGetRenderTextureData(renderTexRes, 0x0, 0x0, 0x0, pixelsF, width * height * 16 );
   } else {
      h3dGetRenderTargetData( 0, "", 0, 0x0, 0x0, 0x0, pixelsF, width * height * 16 );
   }

   // Convert to BGR8
   unsigned char *pixels = new unsigned char[width * height * 3];
   for( int y = 0; y < height; ++y )
   {
      for( int x = 0; x < width; ++x )
      {
         pixels[(y * width + x) * 3 + 0] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 2], 0.f, 1.f ) * 255.f );
         pixels[(y * width + x) * 3 + 1] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 1], 0.f, 1.f ) * 255.f );
         pixels[(y * width + x) * 3 + 2] = ftoi_r( clamp( pixelsF[(y * width + x) * 4 + 0], 0.f, 1.f ) * 255.f );
      }
   }
   delete[] pixelsF;


   std::string filename(fname);
   size_t bytesWritten = 0;
   FILE *f = fopen(fname, "wb");
   if (f) {
      if (boost::algorithm::ends_with(filename, ".tga")) {      
         char *image;
         int imageSize;
         h3dutCreateTGAImage( pixels, width, height, 24, &image, &imageSize );
         bytesWritten = fwrite(image, 1, imageSize, f);
         h3dutFreeMem(&image);
      } else if (boost::algorithm::ends_with(filename, ".png")) {
         h3dutWritePNGImage(f, pixels, width, height);
      }
      fclose(f);
   }
   delete[] pixels;

   return bytesWritten == width * height * 3 + 18;
}


// =================================================================================================
// DLL entry point
// =================================================================================================

#if 0 // staticlly linking...
#ifdef PLATFORM_WIN
BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
   switch( ul_reason_for_call )
	{
	case DLL_PROCESS_DETACH:
		// Close log file
		if( outf.is_open() )
		{
			outf << "</table>\n";
			outf << "</div>\n";
			outf << "</body>\n";
			outf << "</html>";
			outf.close();
		}
	break;
	}
	
	return TRUE;
}
#endif
#endif
