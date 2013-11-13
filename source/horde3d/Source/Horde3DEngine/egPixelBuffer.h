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

#ifndef _egPixelBuffer_H_
#define _egPixelBuffer_H_

#include "egPrerequisites.h"
#include "egResource.h"
#include "egRendererBase.h"


namespace Horde3D {

struct RenderBuffer;

// =================================================================================================

class PixelBufferResource : public Resource
{

public:
   static Resource *factoryFunc( const std::string &name, int flags )
      { return new PixelBufferResource( name ); }
	
   PixelBufferResource( const std::string &name);
   PixelBufferResource( const std::string &name, uint32 size );
   ~PixelBufferResource();
	
   void initDefault();
   void release();
   bool load( const char *data, int size );
   bool loadFrom( const Resource* res );

   void *mapStream( int elem, int elemIdx, int stream, bool read, bool write );
   void unmapStream();

   int getBufferObject() { return _buffer; }
   uint32 getSize() { return _size; }

protected:
   bool raiseError( const std::string &msg );
	
   uint32                _size;
   uint32                _buffer;
   void*                 _pinnedMemory;
   void*                 _pinnedMemoryAligned;
   bool                  _usePinnedMemory;

   friend class ResourceManager;
};

typedef SmartResPtr< PixelBufferResource > PPixelBufferResource;

}
#endif // _egPixelBuffer_H_
