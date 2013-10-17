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

#include "egPixelBuffer.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include <cstring>

#include "utDebug.h"


namespace Horde3D {

using namespace std;


PixelBufferResource::PixelBufferResource( const std::string &name) :
	Resource( ResourceTypes::PixelBuffer, name, 0)
{
	initDefault();
}


PixelBufferResource::PixelBufferResource( const std::string &name, uint32 size) :
	Resource( ResourceTypes::PixelBuffer, name, 0 ), _size(size), _buffer( 0 )
{	
	_loaded = true;

	_buffer = gRDI->createPixelBuffer(size, nullptr);
	if( _buffer == 0 ) {
      initDefault();
   }
}


PixelBufferResource::~PixelBufferResource()
{
	release();
}


void PixelBufferResource::initDefault()
{
   _size = 0;
   _buffer = 0;
}


void PixelBufferResource::release()
{
	gRDI->destroyBuffer(_buffer);
   _buffer = 0;
}


bool PixelBufferResource::raiseError( const std::string &msg )
{
	// Reset
	release();
	initDefault();

	Modules::log().writeError( "Pixel buffer resource '%s': %s", _name.c_str(), msg.c_str() );
	
	return false;
}


bool PixelBufferResource::load( const char *data, int size )
{
	Modules::log().writeError( "Pixel buffer load not implemented yet.");
	return false;
}


void *PixelBufferResource::mapStream( int elem, int elemIdx, int stream, bool read, bool write )
{
   return gRDI->mapBuffer(_buffer);
}


void PixelBufferResource::unmapStream()
{
   gRDI->unmapBuffer(_buffer);
}

}  // namespace
