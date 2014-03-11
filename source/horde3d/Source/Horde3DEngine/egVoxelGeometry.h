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

#ifndef _egVoxelGeometry_H_
#define _egVoxelGeometry_H_

#include "egPrerequisites.h"
#include "egResource.h"
#include "egPrimitives.h"
#include "utMath.h"


namespace Horde3D {

// =================================================================================================
// VoxelGeometry Resource
// =================================================================================================

struct VoxelGeometryResData
{
	enum List
	{
		VoxelGeometryElem = 2000,
		VoxelGeoIndexCountI,
		VoxelGeoVertexCountI,
		VoxelGeoIndices16I,
		VoxelGeoIndexStream,
		VoxelGeoVertexStream,
	};
};

// =================================================================================================

struct VoxelVertexData
{
   Vec3f  pos;
	Vec3f  normal;
	Vec3f  color;
};

// =================================================================================================

class VoxelGeometryResource : public Resource
{
public:
	static void initializationFunc();
	static void releaseFunc();
	static Resource *factoryFunc( const std::string &name, int flags )
		{ return new VoxelGeometryResource( name, flags ); }
	
	VoxelGeometryResource( const std::string &name, int flags );
	~VoxelGeometryResource();
	Resource *clone();
	
	void initDefault();
	void release();
	bool load( const char *data, int size );
   bool loadData(VoxelVertexData *vertices, int vertexOffsets[], uint32 *indicies, int indexOffsets[], int numLodLevels);

	int getElemCount( int elem );
	int getElemParamI( int elem, int elemIdx, int param );
	void *mapStream( int elem, int elemIdx, int stream, bool read, bool write );
	void unmapStream();

	void updateDynamicVertData();

	uint32 getVertCount() const { return _vertCount; }
	char *getIndexData() const { return _indexData; }
	VoxelVertexData *getVertexData() const { return _vertexData; }
	uint32 getVertexBuf() const { return _vertexBuf; }
	uint32 getIndexBuf() const { return _indexBuf; }

   uint32 getBatchStart(int lodLevel) const;
   uint32 getBatchCount(int lodLevel) const;
   uint32 getVertRStart(int lodLevel) const;
   uint32 getVertREnd(int lodLevel) const;
   uint32 clampLodLevel(int lodLevel) const;
public:
	static uint32 defVertexBuffer, defIndexBuffer;

private:
	bool raiseError( const std::string &msg );

private:
	int                         _mappedWriteStream;
	uint32                      _indexBuf, _vertexBuf;
	uint32                      _indexCount, _vertCount;
	bool                        _16BitIndices;
	char                        *_indexData;
	VoxelVertexData             *_vertexData;

   int                         _vertexOffsets[5], _indexOffsets[5];
   int                         _numLodLevels;

	friend class Renderer;
	friend class VoxelModelNode;
	friend class VoxelMeshNode;
};

typedef SmartResPtr< VoxelGeometryResource > PVoxelGeometryResource;

}
#endif //_egVoxelGeometry_H_
