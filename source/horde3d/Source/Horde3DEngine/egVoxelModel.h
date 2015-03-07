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

#ifndef _egVoxelModel_H_
#define _egVoxelModel_H_

#include "egPrerequisites.h"
#include "egScene.h"
#include "egVoxelMesh.h"
#include "egVoxelGeometry.h"
#include "egMaterial.h"
#include "utMath.h"


namespace Horde3D {

// =================================================================================================
// VoxelModel Node
// =================================================================================================

struct VoxelModelNodeParams
{
	enum List
	{
		VoxelGeoResI = 200,
		LodDist1F,
		LodDist2F,
		LodDist3F,
		LodDist4F,
      PolygonOffsetEnabledI,
      PolygonOffsetF,
      UseCoarseCollisionBoxI
	};
};

// =================================================================================================

struct VoxelModelNodeTpl : public SceneNodeTpl
{
	PVoxelGeometryResource  geoRes;
	float              lodDist1, lodDist2, lodDist3, lodDist4;

	VoxelModelNodeTpl( std::string const& name, VoxelGeometryResource *geoRes ) :
		SceneNodeTpl( SceneNodeTypes::VoxelModel, name ), geoRes( geoRes ),
			lodDist1(400), lodDist2( Math::MaxFloat ),
			lodDist3( 700 ), lodDist4( Math::MaxFloat )
	{
	}
};

// =================================================================================================

class VoxelModelNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~VoxelModelNode();

	void recreateNodeList();

	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

	bool updateVoxelGeometry();

	VoxelGeometryResource *getVoxelGeometryResource() const { return _geometryRes; }
	void markNodeListDirty() { _nodeListDirty = true; }

   bool getPolygonOffset(float &x, float &y) {
      if (_polygon_offset_used) {
         x = _polygon_offset[0];
         y = _polygon_offset[1];
      }
      return _polygon_offset_used;
   }
   bool useCoarseCollisionBox() {
      return _useCoarseCollisionBox;
   }

protected:
	VoxelModelNode( const VoxelModelNodeTpl &modelTpl );

	void recreateNodeListRec( SceneNode *node, bool firstCall );
	void updateLocalMeshAABBs();
	void setVoxelGeometryRes( VoxelGeometryResource &geoRes );

	void onPostUpdate();
	void onFinishedUpdate();

protected:
	PVoxelGeometryResource        _geometryRes;
	float                         _lodDist1, _lodDist2, _lodDist3, _lodDist4;
	
	std::vector<VoxelMeshNode*>   _meshList;  // List of the model's meshes
   std::unordered_map<int, SceneNode*> _boneLookup;

	bool                          _nodeListDirty;  // An animatable node has been attached to model
   float                         _polygon_offset[2];
   bool                          _polygon_offset_used;
   bool                          _useCoarseCollisionBox;

	friend class SceneManager;
	friend class SceneNode;
	friend class Renderer;
};

}
#endif // _egVoxelModel_H_
