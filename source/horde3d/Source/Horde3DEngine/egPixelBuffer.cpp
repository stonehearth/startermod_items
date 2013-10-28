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

bool usePinnedMemory()
{
   // For now, just blindly check that the vendor starts with "ATI", and that it's 'modern'.
   return (strncmp("ATI", gRDI->getCaps().vendor, 3) == 0) && 
      (glExt::majorVersion > 3 || (glExt::majorVersion == 3 && glExt::minorVersion >= 2));
}


PixelBufferResource::PixelBufferResource( const std::string &name) :
	Resource( ResourceTypes::PixelBuffer, name, 0)
{
	initDefault();
}


PixelBufferResource::PixelBufferResource( const std::string &name, uint32 size) :
	Resource( ResourceTypes::PixelBuffer, name, 0 ), _size(size), _buffer( 0 )
{	
	_loaded = true;
   _usePinnedMemory = usePinnedMemory();
   _pinnedMemory = nullptr;

   if (!_usePinnedMemory)
   {
	   _buffer = gRDI->createPixelBuffer(GL_PIXEL_UNPACK_BUFFER, size, nullptr);
   } else {
      // We need 4K page alignment, otherwise Bad Things happen....
      _pinnedMemory = new char[size + 0x1000];
      _pinnedMemoryAligned = (void*)((unsigned(_pinnedMemory) + 0xfff) & (~0xfff));
	   _buffer = gRDI->createPixelBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, size, _pinnedMemoryAligned);
   }
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
   if (_pinnedMemory)
   {
      delete _pinnedMemory;
      _pinnedMemory = nullptr;
   }

   if (_buffer)
   {
	   gRDI->destroyBuffer(_buffer);
      _buffer = 0;
   }
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
   if (_usePinnedMemory)
   {
      return _pinnedMemoryAligned;
   }
   return gRDI->mapBuffer(_buffer);
}


void PixelBufferResource::unmapStream()
{
   if (!_usePinnedMemory)
   {
     gRDI->unmapBuffer(_buffer);
   }
}

}  // namespace
