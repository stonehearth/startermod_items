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
   for (int i = 0; i < MAX_SYNCS; i++) {
      _pinnedMemory[i] = 0;
   }
   _streambuff = new unsigned char[size];


   if (!_usePinnedMemory)
   {
	   _buffer = gRDI->createPixelBuffer(GL_PIXEL_UNPACK_BUFFER, size, nullptr);
      if( _buffer == 0 ) {
         initDefault();
      }
   } else {
      _curSync = 0;
      memset(_pinnedMemFences, 0, sizeof(GLsync) * MAX_SYNCS);
      for (int i = 0; i < MAX_SYNCS; i++) {
         // We need 4K page alignment, otherwise Bad Things happen....
         _pinnedMemory[i] = new char[size + 0x1000];
         _pinnedMemoryAligned[i] = (void*)((unsigned(_pinnedMemory[i]) + 0xfff) & (~0xfff));
         _pinnedBuffers[i] = gRDI->createPixelBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, size, _pinnedMemoryAligned[i]);
      }
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
   _curSync = 0;
   for (int i = 0; i < MAX_SYNCS; i++) {
      _pinnedBuffers[i] = 0;
      _pinnedMemory[i] = nullptr;
      _pinnedMemoryAligned[i] = nullptr;
      _pinnedMemFences[i] = nullptr;
   }
   _streambuff = 0x0;
}


void PixelBufferResource::release()
{
   if (_buffer)
   {
	   gRDI->destroyBuffer(_buffer);
      _buffer = 0;
   }

   if (_streambuff) {
      delete[] _streambuff;
      _streambuff = 0x0;
   }

   for (int i = 0; i < MAX_SYNCS; i++) {
      if(_pinnedBuffers[i]) {
	      gRDI->destroyBuffer(_pinnedBuffers[i]);
         _pinnedBuffers[i] = 0;
      }
      if (_pinnedMemFences[i] != nullptr) {
         glDeleteSync(_pinnedMemFences[i]);
         _pinnedMemFences[i] = nullptr;
      }
      if (_pinnedMemory[i])
      {
         delete[] _pinnedMemory[i];
         _pinnedMemory[i] = nullptr;
      }
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
      if (_pinnedMemFences[_curSync] != nullptr) {
         glClientWaitSync(_pinnedMemFences[_curSync], GL_SYNC_FLUSH_COMMANDS_BIT, ~0ull);
      }
      return _pinnedMemoryAligned[_curSync];
   }

   return _streambuff;
}


void PixelBufferResource::unmapStream(int bytesMapped)
{
   if (!_usePinnedMemory)
   {
      bytesMapped = bytesMapped > 0 ? bytesMapped : _size;
      gRDI->updateBufferData(_buffer, 0, bytesMapped, _streambuff);
   }
}

void PixelBufferResource::markSync()
{
   if (!_usePinnedMemory) {
      return;
   }
   _pinnedMemFences[_curSync] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
   _curSync = (_curSync + 1) % MAX_SYNCS;
}

int PixelBufferResource::getBufferObject() {
   if (!_usePinnedMemory) {
      return _buffer;
   }
   return _pinnedBuffers[_curSync];
}

}  // namespace
