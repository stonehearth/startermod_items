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

#include "egVoxelMesh.h"
#include "egVoxelModel.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egRenderer.h"
#include "csg/ray.h"
#include "csg/cube.h"
#include "csg/util.h"

#include "utDebug.h"


namespace Horde3D {

using namespace std;


// *************************************************************************************************
// Class VoxelMeshNode
// *************************************************************************************************

VoxelMeshNode::VoxelMeshNode( const VoxelMeshNodeTpl &meshTpl ) :
	SceneNode( meshTpl ),
   _materialRes( meshTpl.matRes ),
   _geometryRes( meshTpl.geoRes ),
	_parentModel( 0x0 )
{
	_renderable = true;
   _noInstancing = false;
   _instanceKey.matResource = _materialRes;
   _instanceKey.geoResource = _geometryRes;
	if( _materialRes != 0x0 && _geometryRes != 0x0) {
      _instanceKey.updateHash();
		_sortKey = (float)_materialRes->getHandle();
   }
}


VoxelMeshNode::~VoxelMeshNode()
{
	_materialRes = 0x0;
   _geometryRes = 0x0;
}


SceneNodeTpl *VoxelMeshNode::parsingFunc( map< string, std::string > &attribs )
{
	return 0x0;
}


SceneNode *VoxelMeshNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::VoxelMesh ) return 0x0;
	
	return new VoxelMeshNode( *(VoxelMeshNodeTpl *)&nodeTpl );
}

bool VoxelMeshNode::canAttach( SceneNode &parent )
{
	// Important: VoxelMeshes may not live outside of models
	return (parent.getType() == SceneNodeTypes::VoxelModel) ||
		   (parent.getType() == SceneNodeTypes::VoxelMesh) ||
		   (parent.getType() == SceneNodeTypes::Joint);
}


int VoxelMeshNode::getParamI( int param )
{
	switch( param )
	{
	case VoxelMeshNodeParams::MatResI:
		if( _materialRes != 0x0 ) return _materialRes->getHandle();
		else return 0;
   case VoxelMeshNodeParams::NoInstancingI:
      return _noInstancing;
	}

	return SceneNode::getParamI( param );
}


void VoxelMeshNode::setParamI( int param, int value )
{
	Resource *res;
	
	switch( param )
	{
	case VoxelMeshNodeParams::MatResI:
		res = Modules::resMan().resolveResHandle( value );
		if( res != 0x0 && res->getType() == ResourceTypes::Material )
		{
			_materialRes = (MaterialResource *)res;
			_sortKey = (float)_materialRes->getHandle();
		}
		else
		{
			Modules::setError( "Invalid handle in h3dSetNodeParamI for H3DVoxelMesh::MatResI" );
		}
      _instanceKey.matResource = _materialRes;
      _instanceKey.updateHash();
		return;
   case VoxelMeshNodeParams::NoInstancingI:
      _noInstancing = (value != 0);
      return;
	}

	SceneNode::setParamI( param, value );
}

void VoxelMeshNode::setVoxelGeometryRes( VoxelGeometryResource &geoRes )
{
	//updateLocalMeshAABBs();
}

bool VoxelMeshNode::checkIntersectionInternal( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const
{
   if (_parentModel->useCoarseCollisionBox()) {
      double d;
      radiant::csg::Point3f origin(rayOrig.x, rayOrig.y, rayOrig.z);
      radiant::csg::Point3f dir(rayDir.x, rayDir.y, rayDir.z);
      dir.Normalize();

      radiant::csg::Ray3 ray(origin, dir);

      radiant::csg::Cube3f cube(radiant::csg::Point3f(_bBox.min().x, _bBox.min().y, _bBox.min().z),
                                radiant::csg::Point3f(_bBox.max().x, _bBox.max().y, _bBox.max().z));

      if (!radiant::csg::Cube3Intersects(cube, ray, d)) {
         return false;
      }
      // super gross fudge factor: make the distance shorter to make sure we get picked
      // instaed of what we're coplanar with
      d -= 0.05f;
      radiant::csg::Point3f intersection = origin + (dir * d);
      intsPos.x = (float)intersection.x;
      intsPos.y = (float)intersection.y;
      intsPos.z = (float)intersection.z;
      intsNorm.x = 0; // lies!
      intsNorm.y = 1; // lies!
      intsNorm.z = 0; // lies!
      return true;
   }

	if( _geometryRes == 0x0 || _geometryRes->getIndexData() == 0x0 || _geometryRes->getVertexData() == 0x0 ) return false;
	
	// Transform ray to local space

	Vec3f nearestIntsPos = Vec3f( Math::MaxFloat, Math::MaxFloat, Math::MaxFloat );
	bool intersection = false;
	
	// Check triangles
   int oldBoneNum = -1;
   Matrix4f closestM;
   Matrix4f m = _absTrans.inverted();
	Vec3f orig = m * rayOrig;
	Vec3f dir = m * (rayOrig + rayDir) - orig;
   std::vector<SceneNode*> const& boneLookup = _parentModel->getBoneLookup();

   // Select the maximum LOD level for triangle iteration (since there will be fewer triangles!)
   int maxLod = _geometryRes->clampLodLevel(2);

   for( uint32 i = getBatchStart(maxLod); i < getBatchStart(maxLod) + getBatchCount(maxLod); i += 3 )
	{
      int boneNum = (int)_geometryRes->getVertexData()[_geometryRes->_indexData[i + 0]].boneIndex;

      if (boneNum != oldBoneNum) {
         oldBoneNum = boneNum;
         if ((int)boneLookup.size() > boneNum) {
            m = boneLookup[boneNum]->getAbsTrans().inverted();
         } else {
            m = _absTrans.inverted();
         }
	      orig = m * rayOrig;
	      dir = m * (rayOrig + rayDir) - orig;
      }

      Vec3f const& vert0 = _geometryRes->getVertexData()[_geometryRes->_indexData[i + 0]].pos * _parentModel->getModelScale();
		Vec3f const& vert1 = _geometryRes->getVertexData()[_geometryRes->_indexData[i + 1]].pos * _parentModel->getModelScale();
		Vec3f const& vert2 = _geometryRes->getVertexData()[_geometryRes->_indexData[i + 2]].pos * _parentModel->getModelScale();

		if( rayTriangleIntersection( orig, dir, vert0, vert1, vert2, intsPos ) )
		{
			intersection = true;
			if( (intsPos - orig).length() < (nearestIntsPos - orig).length() ) {
				nearestIntsPos = intsPos;
            closestM = m;
            intsNorm = (vert1 - vert0).cross(vert2 - vert1);
         }
		}
	}

   intsPos = closestM.inverted() * nearestIntsPos;
	
	return intersection;
}


void VoxelMeshNode::onAttach( SceneNode &parentNode )
{
	// Find parent model node
	SceneNode *node = &parentNode;
	while( node->getType() != SceneNodeTypes::VoxelModel ) node = node->getParent();
	_parentModel = (VoxelModelNode *)node;
	_parentModel->markNodeListDirty();
   _instanceKey.scale = _parentModel->getModelScale();
   _instanceKey.updateHash();
}


void VoxelMeshNode::onDetach( SceneNode &/*parentNode*/ )
{
	if( _parentModel != 0x0 ) _parentModel->markNodeListDirty();
}


void VoxelMeshNode::onPostUpdate()
{
   if (_parentModel && _instanceKey.scale != _parentModel->getModelScale()) {
      _instanceKey.scale = _parentModel->getModelScale();
      _instanceKey.updateHash();
   }
}


uint32 VoxelMeshNode::getBatchStart(int lodLevel) const {
   return _geometryRes->getBatchStart(lodLevel);
}


uint32 VoxelMeshNode::getBatchCount(int lodLevel) const { 
   return _geometryRes->getBatchCount(lodLevel);
}


uint32 VoxelMeshNode::getVertRStart(int lodLevel) const {
   return _geometryRes->getVertRStart(lodLevel);
}


uint32 VoxelMeshNode::getVertREnd(int lodLevel) const {
   return _geometryRes->getVertREnd(lodLevel);
}


}  // namespace
