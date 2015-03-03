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

#include "egVoxelGeometry.h"
#include "egResource.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"
#include <cstring>

#include "utDebug.h"


namespace Horde3D {

using namespace std;

uint32 VoxelGeometryResource::defVertexBuffer = 0;
uint32 VoxelGeometryResource::defIndexBuffer = 0;

void VoxelGeometryResource::initializationFunc()
{
	defVertexBuffer = gRDI->createVertexBuffer( 0, STATIC, 0x0 );
	defIndexBuffer = gRDI->createIndexBuffer( 0, 0x0 );
}

void VoxelGeometryResource::releaseFunc()
{
	gRDI->destroyBuffer( defVertexBuffer );
	gRDI->destroyBuffer( defIndexBuffer );
}

VoxelGeometryResource::VoxelGeometryResource( std::string const& name, int flags ) :
	Resource( ResourceTypes::VoxelGeometry, name, flags )
{
	initDefault();
}

VoxelGeometryResource::~VoxelGeometryResource()
{
	release();
}

Resource *VoxelGeometryResource::clone()
{
	VoxelGeometryResource *res = new VoxelGeometryResource( "", _flags );

	*res = *this;

	// Make a deep copy of the data
	res->_indexData = new uint32[_indexCount];
	res->_vertexData = new VoxelVertexData[_vertCount];
	memcpy( res->_indexData, _indexData, _indexCount * sizeof(uint32) );
	memcpy( res->_vertexData, _vertexData, _vertCount * sizeof( VoxelVertexData ) );
	res->_indexBuf = gRDI->createIndexBuffer( _indexCount * sizeof(uint32), _indexData );
	res->_vertexBuf = gRDI->createVertexBuffer( _vertCount * sizeof( Vec3f ), STATIC, _vertexData );
	
	return res;
}


void VoxelGeometryResource::initDefault()
{
   _mappedWriteStream = -1;
   _loaded = true;
	_indexCount = 0;
	_vertCount = 0;
	_indexData = 0x0;
	_vertexData = 0x0;
	_indexBuf = defIndexBuffer;
	_vertexBuf = defVertexBuffer;
   _numLodLevels = 1;
}


void VoxelGeometryResource::release()
{
	if( _vertexBuf != 0 && _vertexBuf != defVertexBuffer )
	{
		gRDI->destroyBuffer( _vertexBuf );
		_vertexBuf = 0;
	}
	
	if( _indexBuf != 0 && _indexBuf != defIndexBuffer )
	{
		gRDI->destroyBuffer( _indexBuf );
		_indexBuf = 0;
	}

	delete[] _indexData; _indexData = 0x0;
	delete[] _vertexData; _vertexData = 0x0;
}


bool VoxelGeometryResource::raiseError( std::string const& msg )
{
	// Reset
	release();
	initDefault();

	Modules::log().writeError( "VoxelGeometry resource '%s': %s", _name.c_str(), msg.c_str() );
	
	return false;
}

bool VoxelGeometryResource::loadData(VoxelVertexData *vertices, int vertexOffsets[], uint32 *indicies, int indexOffsets[], int numLodLevels)
{
   _numLodLevels = numLodLevels;
   for (int i = 0; i < numLodLevels + 1; i++) {
      _vertexOffsets[i] = vertexOffsets[i];
      _indexOffsets[i] = indexOffsets[i];
   }

	_vertCount = vertexOffsets[numLodLevels];
	_vertexData = new VoxelVertexData[_vertCount];
   ::memcpy(_vertexData, vertices, _vertCount * sizeof VoxelVertexData);

	_indexCount = indexOffsets[numLodLevels];

	_indexData = new uint32[_indexCount];

   ::memcpy(_indexData, indicies, _indexCount * sizeof uint32);

	// Upload data
	if( _vertCount > 0 && _indexCount > 0 )
	{
		// Upload indices
		_indexBuf = gRDI->createIndexBuffer( _indexCount * sizeof uint32, _indexData );
		
		// Upload vertices
		_vertexBuf = gRDI->createVertexBuffer(_vertCount * sizeof VoxelVertexData, STATIC, _vertexData );
	}
   return true;
}


bool VoxelGeometryResource::load( const char *data, int size )
{
   _loaded = true;
   return true;
}


int VoxelGeometryResource::getElemCount( int elem )
{
	switch( elem )
	{
	case VoxelGeometryResData::VoxelGeometryElem:
		return 1;
	default:
		return Resource::getElemCount( elem );
	}
}


int VoxelGeometryResource::getElemParamI( int elem, int elemIdx, int param )
{
	switch( elem )
	{
	case VoxelGeometryResData::VoxelGeometryElem:
		switch( param )
		{
		case VoxelGeometryResData::VoxelGeoIndexCountI:
			return (int)_indexCount;
		case VoxelGeometryResData::VoxelGeoVertexCountI:
			return (int)_vertCount;
		}
		break;
	}
	
	return Resource::getElemParamI( elem, elemIdx, param );
}


void *VoxelGeometryResource::mapStream( int elem, int elemIdx, int stream, bool read, bool write )
{
	if( read || write )
	{
		_mappedWriteStream = -1;
		
		switch( elem )
		{
		case VoxelGeometryResData::VoxelGeometryElem:
			switch( stream )
			{
			case VoxelGeometryResData::VoxelGeoIndexStream:
				if( write ) _mappedWriteStream = VoxelGeometryResData::VoxelGeoIndexStream;
				return _indexData;
			case VoxelGeometryResData::VoxelGeoVertexStream:
				if( write ) _mappedWriteStream = VoxelGeometryResData::VoxelGeoVertexStream;
				return _vertexData != 0x0 ? _vertexData : 0x0;
			}
		}
	}

	return Resource::mapStream( elem, elemIdx, stream, read, write );
}


void VoxelGeometryResource::unmapStream(int bytesMapped)
{
	if( _mappedWriteStream )
	{
		switch( _mappedWriteStream )
		{
		case VoxelGeometryResData::VoxelGeoIndexStream:
			if( _indexData != 0x0 )
				gRDI->updateBufferData( _indexBuf, 0, _indexCount * sizeof(uint32), _indexData );
			break;
		case VoxelGeometryResData::VoxelGeoVertexStream:
			if( _vertexData != 0x0 )
				gRDI->updateBufferData( _vertexBuf, 0, _vertCount * sizeof( VoxelVertexData ), _vertexData );
			break;
		}

		_mappedWriteStream = -1;
	}
}


void VoxelGeometryResource::updateDynamicVertData()
{
	// Upload dynamic stream data
	if( _vertexData != 0x0 )
	{
		gRDI->updateBufferData( _vertexBuf, 0, _vertCount * sizeof( VoxelVertexData ), _vertexData );
	}
}


inline uint32 VoxelGeometryResource::clampLodLevel(int lodLevel) const
{
   return lodLevel < _numLodLevels ? lodLevel : _numLodLevels - 1;
}

uint32 VoxelGeometryResource::getBatchStart(int lodLevel) const
{
   return _indexOffsets[clampLodLevel(lodLevel)];
}

uint32 VoxelGeometryResource::getBatchCount(int lodLevel) const
{
   lodLevel = clampLodLevel(lodLevel);
   int r1 = _indexOffsets[lodLevel + 1];
   int r2 = _indexOffsets[lodLevel];
   return r1 - r2;
}

uint32 VoxelGeometryResource::getVertRStart(int lodLevel) const
{
   return _vertexOffsets[clampLodLevel(lodLevel)];
}

uint32 VoxelGeometryResource::getVertREnd(int lodLevel) const
{
   return _vertexOffsets[clampLodLevel(lodLevel) + 1] - 1;
}


}  // namespace
