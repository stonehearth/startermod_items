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

#include "egVoxelModel.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egRenderer.h"
#include "egCom.h"
#include <cstring>

#include "utDebug.h"

namespace Horde3D {

using namespace std;


VoxelModelNode::VoxelModelNode( const VoxelModelNodeTpl &modelTpl ) :
	SceneNode( modelTpl ),
	_lodDist1( modelTpl.lodDist1 ), _lodDist2( modelTpl.lodDist2 ),
	_lodDist3( modelTpl.lodDist3 ), _lodDist4( modelTpl.lodDist4 ),
	_nodeListDirty( false ),  _polygon_offset_used(false),
   _useCoarseCollisionBox(false), _modelScale(1.0)
{
   _meshNode = nullptr;
}


VoxelModelNode::~VoxelModelNode()
{
}


SceneNodeTpl *VoxelModelNode::parsingFunc( map< string, std::string > &attribs )
{
	bool result = true;
	
	VoxelModelNodeTpl *modelTpl = new VoxelModelNodeTpl( "");
	
	map< string, std::string >::iterator itr = attribs.find( "geometry" );
	itr = attribs.find( "lodDist1" );
	if( itr != attribs.end() ) modelTpl->lodDist1 = (float)atof( itr->second.c_str() );
	itr = attribs.find( "lodDist2" );
	if( itr != attribs.end() ) modelTpl->lodDist2 = (float)atof( itr->second.c_str() );
	itr = attribs.find( "lodDist3" );
	if( itr != attribs.end() ) modelTpl->lodDist3 = (float)atof( itr->second.c_str() );
	itr = attribs.find( "lodDist4" );
	if( itr != attribs.end() ) modelTpl->lodDist4 = (float)atof( itr->second.c_str() );

	if( !result )
	{
		delete modelTpl; modelTpl = 0x0;
	}
	
	return modelTpl;
}


SceneNode *VoxelModelNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::VoxelModel ) return 0x0;

	return new VoxelModelNode( *(VoxelModelNodeTpl *)&nodeTpl );
}

void VoxelModelNode::recreateNodeListRec(SceneNode *node, bool firstCall)
{
   for (auto& child : node->getChildren()) {
      if (child->getType() == SceneNodeTypes::VoxelJointNode) {
         uint32 idx = ((VoxelJointNode*)child)->getJointIndex();
         if (_boneLookup.size() <= idx) {
            _boneLookup.resize(idx + 1);
         }
         _boneLookup[idx] = child;
      } else if (child->getType() == SceneNodeTypes::VoxelMesh) {
         _meshNode = (VoxelMeshNode *)child;
      }
   }

   if (_meshNode != nullptr) {
      VoxelMeshNode &mesh = *_meshNode;
		
      for( uint32 j = mesh.getVertRStart(0); j <= mesh.getVertREnd(0); ++j )
	   {
         VoxelVertexData const& vert = mesh._geometryRes->getVertexData()[j];

         const int boneIndex = (int)vert.boneIndex;
         _boneBounds[boneIndex].addPoint(mesh._geometryRes->getVertexData()[j].pos * _modelScale);
	   }
   }
} 


void VoxelModelNode::recreateNodeList()
{
   _meshNode = nullptr;
   _boneLookup.clear();
   _boneBounds.clear();
	
   recreateNodeListRec(this, true);

   _nodeListDirty = false;
}


// Because all of our bones are rigid, we can use the local-space AABBs we built, once, and transform them
// over and over to get the AABB of the entire model/mesh.
void VoxelModelNode::updateLocalMeshAABBs()
{
   if (_meshNode == nullptr) {
      return;
   }

   VoxelMeshNode &mesh = *_meshNode;
		
   mesh._localBBox.clear();

   for (auto& bounds : _boneBounds) {
      BoundingBox boneBounds = bounds.second;
      uint32 idx = bounds.first;
      if (_boneLookup.size() > idx) {
         boneBounds.transform(_boneLookup[idx]->getRelTrans());
      }
      mesh._localBBox.makeUnion(boneBounds);
   }

	// Avoid zero box dimensions for planes
   mesh._localBBox.feather();

   // Update the child mesh.  Because this makes sense.
   mesh._bBox = mesh._localBBox;
   mesh._bBox.transform(_absTrans);
}


int VoxelModelNode::getParamI( int param )
{
	return SceneNode::getParamI( param );
}


void VoxelModelNode::setParamI( int param, int value )
{	
	switch( param )
	{
   case VoxelModelNodeParams::PolygonOffsetEnabledI:
      _polygon_offset_used = (value != 0);
      return;
   case VoxelModelNodeParams::UseCoarseCollisionBoxI:
      _useCoarseCollisionBox = (value != 0);
      return;
	}

	SceneNode::setParamI( param, value );
}


float VoxelModelNode::getParamF( int param, int compIdx )
{
	switch( param )
	{
	case VoxelModelNodeParams::LodDist1F:
		return _lodDist1;
	case VoxelModelNodeParams::LodDist2F:
		return _lodDist2;
	case VoxelModelNodeParams::LodDist3F:
		return _lodDist3;
	case VoxelModelNodeParams::LodDist4F:
		return _lodDist4;
	}

	return SceneNode::getParamF( param, compIdx );
}


void VoxelModelNode::setParamF( int param, int compIdx, float value )
{
	switch( param )
	{
	case VoxelModelNodeParams::LodDist1F:
		_lodDist1 = value;
		return;
	case VoxelModelNodeParams::LodDist2F:
		_lodDist2 = value;
		return;
	case VoxelModelNodeParams::LodDist3F:
		_lodDist3 = value;
		return;
	case VoxelModelNodeParams::LodDist4F:
		_lodDist4 = value;
		return;
   case VoxelModelNodeParams::ModelScaleF:
      _modelScale = value;
      markDirty(SceneNodeDirtyKind::All);
      return;
   case VoxelModelNodeParams::PolygonOffsetF:
      if (compIdx >= 0 && compIdx < 2) {
         _polygon_offset[compIdx] = value;
      }
      return;
	}

	SceneNode::setParamF( param, compIdx, value );
}

void VoxelModelNode::onPostUpdate()
{
	if (_nodeListDirty) {
      recreateNodeList();
   }
   updateLocalMeshAABBs();

   if (_boneRelTransLookup.size() < _boneLookup.size()) {
      _boneRelTransLookup.resize(_boneLookup.size());
   }
   for (int i = 0; i < (int)_boneLookup.size(); i++) {
      _boneRelTransLookup[i] = _boneLookup[i]->getRelTrans();
   }
}

std::vector<SceneNode*> const& VoxelModelNode::getBoneLookup() const
{
   return _boneLookup;
}

std::vector<Matrix4f> const& VoxelModelNode::getBoneRelTransLookup() const
{
   return _boneRelTransLookup;
}

void VoxelModelNode::markNodeListDirty() 
{ 
   _nodeListDirty = true; 
   markDirty(SceneNodeDirtyKind::All);
}


void VoxelModelNode::onFinishedUpdate()
{
	// Our bounds is just our child's bounds.

   if (_meshNode != nullptr) {
      _bBox = _meshNode->_bBox;
   }
}

}  // namespace
