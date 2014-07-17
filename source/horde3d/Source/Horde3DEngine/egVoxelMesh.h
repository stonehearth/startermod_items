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
		LodLevelI,
      NoInstancingI,
	};
};

// =================================================================================================

struct VoxelMeshNodeTpl : public SceneNodeTpl
{
	PMaterialResource  matRes;
   
	VoxelMeshNodeTpl( std::string const& name, MaterialResource *materialRes) :
		SceneNodeTpl( SceneNodeTypes::VoxelMesh, name ), matRes( materialRes )
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

	MaterialResource *getMaterialRes() const { return _materialRes; }
	uint32 getBatchStart(int lodLevel) const;
	uint32 getBatchCount(int lodLevel) const;
	uint32 getVertRStart(int lodLevel) const;
	uint32 getVertREnd(int lodLevel) const;
	VoxelModelNode *getParentModel() const { return _parentModel; }

   const InstanceKey* getInstanceKey() const;

protected:
	VoxelMeshNode( const VoxelMeshNodeTpl &meshTpl );
	~VoxelMeshNode();

protected:
	PMaterialResource   _materialRes;
	
	VoxelModelNode      *_parentModel;
	BoundingBox         _localBBox;
	bool                _ignoreAnim;
   bool                _noInstancing;

	std::vector< uint32 >  _occQueries;
	std::vector< uint32 >  _lastVisited;

private:
   InstanceKey         _instanceKey;

	friend class SceneManager;
	friend class SceneNode;
	friend class VoxelModelNode;
	friend class Renderer;
};

}
#endif // _egVoxelMesh_H_
