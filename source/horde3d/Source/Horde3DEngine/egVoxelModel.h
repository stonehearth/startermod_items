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
		SWSkinningI,
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
	bool               softwareSkinning;

	VoxelModelNodeTpl( const std::string &name, VoxelGeometryResource *geoRes ) :
		SceneNodeTpl( SceneNodeTypes::VoxelModel, name ), geoRes( geoRes ),
			lodDist1(400), lodDist2( Math::MaxFloat ),
			lodDist3( 700 ), lodDist4( Math::MaxFloat ),
			softwareSkinning( false )
	{
	}
};

// =================================================================================================

struct VoxelMorpher	// Morph modifier
{
	std::string  name;
	uint32       index;  // Index of morph target in Geometry resource
	float        weight;
};

// =================================================================================================

class VoxelModelNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~VoxelModelNode();

	void recreateNodeList();
	void setupAnimStage( int stage, AnimationResource *anim, int layer,
	                     const std::string &startNode, bool additive );
	void setAnimParams( int stage, float time, float weight );
	bool setMorphParam( const std::string &targetName, float weight );

	int getParamI( int param );
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

	bool updateVoxelGeometry();

	VoxelGeometryResource *getVoxelGeometryResource() const { return _geometryRes; }
	bool jointExists( uint32 jointIndex ) { return jointIndex < _skinMatRows.size() / 3; }
	void setSkinningMat( uint32 index, const Matrix4f &mat )
		{ _skinMatRows[index * 3 + 0] = mat.getRow( 0 );
		  _skinMatRows[index * 3 + 1] = mat.getRow( 1 );
		  _skinMatRows[index * 3 + 2] = mat.getRow( 2 ); }
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
	PVoxelGeometryResource        _baseVoxelGeoRes;	// NULL if model does not have a private geometry copy
	float                         _lodDist1, _lodDist2, _lodDist3, _lodDist4;
	
	std::vector< VoxelMeshNode * >     _meshList;  // List of the model's meshes
	std::vector< Vec4f >          _skinMatRows;

	std::vector< VoxelMorpher >   _morphers;
	bool                          _softwareSkinning, _skinningDirty;
	bool                          _nodeListDirty;  // An animatable node has been attached to model
	bool                          _morpherUsed, _morpherDirty;
   float                         _polygon_offset[2];
   bool                          _polygon_offset_used;
   bool                          _useCoarseCollisionBox;

	friend class SceneManager;
	friend class SceneNode;
	friend class Renderer;
};

}
#endif // _egVoxelModel_H_
