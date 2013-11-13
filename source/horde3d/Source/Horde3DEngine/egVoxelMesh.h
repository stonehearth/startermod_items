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

#ifndef _egVoxelMesh_H_
#define _egVoxelMesh_H_

#include "egPrerequisites.h"
#include "egScene.h"
#include "egAnimation.h"
#include "utMath.h"


namespace Horde3D {

class MaterialResource;
class VoxelModelNode;


// =================================================================================================
// VoxelMesh Node
// =================================================================================================

struct VoxelMeshNodeParams
{
	enum List
	{
		MatResI = 300,
		BatchStartI,
		BatchCountI,
		VertRStartI,
		VertREndI,
		LodLevelI
	};
};

// =================================================================================================

struct VoxelMeshNodeTpl : public SceneNodeTpl
{
	PMaterialResource  matRes;
	uint32             batchStart, batchCount;
	uint32             vertRStart, vertREnd;
	uint32             lodLevel;

	VoxelMeshNodeTpl( const std::string &name, MaterialResource *materialRes, uint32 batchStart,
	             uint32 batchCount, uint32 vertRStart, uint32 vertREnd ) :
		SceneNodeTpl( SceneNodeTypes::VoxelMesh, name ), matRes( materialRes ), batchStart( batchStart ),
		batchCount( batchCount ), vertRStart( vertRStart ), vertREnd( vertREnd ), lodLevel( 0 )
	{
	}
};

// =================================================================================================

class VoxelMeshNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	bool canAttach( SceneNode &parent );
	int getParamI( int param );
	void setParamI( int param, int value );
	bool checkIntersection( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const;

	void onAttach( SceneNode &parentNode );
	void onDetach( SceneNode &parentNode );
	void onPostUpdate();

	MaterialResource *getMaterialRes() { return _materialRes; }
	uint32 getBatchStart() { return _batchStart; }
	uint32 getBatchCount() { return _batchCount; }
	uint32 getVertRStart() { return _vertRStart; }
	uint32 getVertREnd() { return _vertREnd; }
	uint32 getLodLevel() { return _lodLevel; }
	VoxelModelNode *getParentModel() { return _parentModel; }

protected:
	VoxelMeshNode( const VoxelMeshNodeTpl &meshTpl );
	~VoxelMeshNode();

protected:
	PMaterialResource   _materialRes;
	uint32              _batchStart, _batchCount;
	uint32              _vertRStart, _vertREnd;
	uint32              _lodLevel;
	
	VoxelModelNode      *_parentModel;
	BoundingBox         _localBBox;
	bool                _ignoreAnim;

	std::vector< uint32 >  _occQueries;
	std::vector< uint32 >  _lastVisited;

	friend class SceneManager;
	friend class SceneNode;
	friend class VoxelModelNode;
	friend class Renderer;
};

}
#endif // _egVoxelMesh_H_
