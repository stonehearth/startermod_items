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
#include "egVoxelGeometry.h"
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
		VoxelGeoResI
	};
};

// =================================================================================================

struct VoxelMeshNodeTpl : public SceneNodeTpl
{
	PMaterialResource  matRes;
	PVoxelGeometryResource  geoRes;
   
	VoxelMeshNodeTpl( std::string const& name, MaterialResource *materialRes, VoxelGeometryResource *geometryRes) :
		SceneNodeTpl( SceneNodeTypes::VoxelMesh, name ), matRes( materialRes ), geoRes( geometryRes )
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
	virtual int getParamI( int param ) const;
	virtual void setParamI( int param, int value );
	bool checkIntersectionInternal( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const;

	void onAttach( SceneNode &parentNode );
	void onDetach( SceneNode &parentNode );
	void onPostUpdate();

	MaterialResource *getMaterialRes() const { return _materialRes; }
	uint32 getBatchStart(int lodLevel) const;
	uint32 getBatchCount(int lodLevel) const;
	uint32 getVertRStart(int lodLevel) const;
	uint32 getVertREnd(int lodLevel) const;
	VoxelModelNode *getParentModel() const { return _parentModel; }
	bool updateVoxelGeometry();

	VoxelGeometryResource *getVoxelGeometryResource() const { return _geometryRes; }

protected:
	VoxelMeshNode( const VoxelMeshNodeTpl &meshTpl );
	~VoxelMeshNode();

   void setVoxelGeometryRes( VoxelGeometryResource &geoRes );

protected:
	PMaterialResource   _materialRes;
	
	VoxelModelNode      *_parentModel;
	BoundingBox         _localBBox;
	PVoxelGeometryResource        _geometryRes;

private:

	friend class SceneManager;
	friend class SceneNode;
	friend class VoxelModelNode;
	friend class Renderer;
};

}
#endif // _egVoxelMesh_H_
