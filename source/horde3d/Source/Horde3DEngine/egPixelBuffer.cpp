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
   return gRDI->getCaps().cardType == CardType::ATI && gRDI->getCaps().hasPinnedMemory;
}


PixelBufferResource::PixelBufferResource( std::string const& name) :
	Resource( ResourceTypes::PixelBuffer, name, 0)
{
	initDefault();
}


PixelBufferResource::PixelBufferResource( std::string const& name, uint32 size) :
	Resource( ResourceTypes::PixelBuffer, name, 0 ), _size(size), _buffer( 0 )
{	
	_loaded = true;
   _usePinnedMemory = usePinnedMemory();
   _pinnedMemory = nullptr;
   _streambuff = new unsigned char[size];

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
   _streambuff = 0x0;
}


void PixelBufferResource::release()
{
   if (_pinnedMemory)
   {
      delete[] _pinnedMemory;
      _pinnedMemory = nullptr;
   }

   if (_buffer)
   {
	   gRDI->destroyBuffer(_buffer);
      _buffer = 0;
   }

   if (_streambuff) {
      delete[] _streambuff;
      _streambuff = 0x0;
   }
}


bool PixelBufferResource::raiseError( std::string const& msg )
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

   return _streambuff;
}


void PixelBufferResource::unmapStream(int bytesMapped)
{
   if (!_usePinnedMemory)
   {
      gRDI->updateBufferData(_buffer, 0, _size, _streambuff);
   }
}

}  // namespace
