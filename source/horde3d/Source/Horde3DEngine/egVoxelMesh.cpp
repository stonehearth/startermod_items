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
	_materialRes( meshTpl.matRes ), _batchStart( meshTpl.batchStart ), _batchCount( meshTpl.batchCount ),
	_vertRStart( meshTpl.vertRStart ), _vertREnd( meshTpl.vertREnd ), _lodLevel( meshTpl.lodLevel ),
	_parentModel( 0x0 )
{
	_renderable = true;
	
	if( _materialRes != 0x0 )
		_sortKey = (float)_materialRes->getHandle();
}


VoxelMeshNode::~VoxelMeshNode()
{
	_materialRes = 0x0;
	for( uint32 i = 0; i < _occQueries.size(); ++i )
	{
		if( _occQueries[i] != 0 )
			gRDI->destroyQuery( _occQueries[i] );
	}
}


SceneNodeTpl *VoxelMeshNode::parsingFunc( map< string, std::string > &attribs )
{
	bool result = true;
	
	map< string, std::string >::iterator itr;
	VoxelMeshNodeTpl *meshTpl = new VoxelMeshNodeTpl( "", 0x0, 0, 0, 0, 0 );

	itr = attribs.find( "material" );
	if( itr != attribs.end() )
	{
		uint32 res = Modules::resMan().addResource( ResourceTypes::Material, itr->second, 0, false );
		if( res != 0 )
			meshTpl->matRes = (MaterialResource *)Modules::resMan().resolveResHandle( res );
	}
	else result = false;
	itr = attribs.find( "batchStart" );
	if( itr != attribs.end() ) meshTpl->batchStart = atoi( itr->second.c_str() );
	else result = false;
	itr = attribs.find( "batchCount" );
	if( itr != attribs.end() ) meshTpl->batchCount = atoi( itr->second.c_str() );
	else result = false;
	itr = attribs.find( "vertRStart" );
	if( itr != attribs.end() ) meshTpl->vertRStart = atoi( itr->second.c_str() );
	else result = false;
	itr = attribs.find( "vertREnd" );
	if( itr != attribs.end() ) meshTpl->vertREnd = atoi( itr->second.c_str() );
	else result = false;

	itr = attribs.find( "lodLevel" );
	if( itr != attribs.end() ) meshTpl->lodLevel = atoi( itr->second.c_str() );

	if( !result )
	{
		delete meshTpl; meshTpl = 0x0;
	}
	
	return meshTpl;
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
	case VoxelMeshNodeParams::BatchStartI:
		return _batchStart;
	case VoxelMeshNodeParams::BatchCountI:
		return _batchCount;
	case VoxelMeshNodeParams::VertRStartI:
		return _vertRStart;
	case VoxelMeshNodeParams::VertREndI:
		return _vertREnd;
	case VoxelMeshNodeParams::LodLevelI:
		return _lodLevel;
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
		return;
	case VoxelMeshNodeParams::LodLevelI:
		_lodLevel = value;
		return;
	}

	SceneNode::setParamI( param, value );
}


bool VoxelMeshNode::checkIntersection( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const
{
	// Collision check is only done for base LOD
	if( _lodLevel != 0 ) return false;

	if( !rayAABBIntersection( rayOrig, rayDir, _bBox.min(), _bBox.max() ) ) return false;

   if (_parentModel->useCoarseCollisionBox()) {
      float d;
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
      intsPos.x = intersection.x;
      intsPos.y = intersection.y;
      intsPos.z = intersection.z;
      intsNorm.x = 0; // lies!
      intsNorm.y = 1; // lies!
      intsNorm.z = 0; // lies!
      return true;
   }

	VoxelGeometryResource *geoRes = _parentModel->getVoxelGeometryResource();
	if( geoRes == 0x0 || geoRes->getIndexData() == 0x0 || geoRes->getVertexData() == 0x0 ) return false;
	
	// Transform ray to local space
	Matrix4f m = _absTrans.inverted();
	Vec3f orig = m * rayOrig;
	Vec3f dir = m * (rayOrig + rayDir) - orig;

	Vec3f nearestIntsPos = Vec3f( Math::MaxFloat, Math::MaxFloat, Math::MaxFloat );
	bool intersection = false;
	
	// Check triangles
	for( uint32 i = _batchStart; i < _batchStart + _batchCount; i += 3 )
	{
		Vec3f *vert0, *vert1, *vert2;
		
		if( geoRes->_16BitIndices )
		{
			vert0 = &geoRes->getVertexData()[((uint16 *)geoRes->_indexData)[i + 0]].pos;
			vert1 = &geoRes->getVertexData()[((uint16 *)geoRes->_indexData)[i + 1]].pos;
			vert2 = &geoRes->getVertexData()[((uint16 *)geoRes->_indexData)[i + 2]].pos;
		}
		else
		{
			vert0 = &geoRes->getVertexData()[((uint32 *)geoRes->_indexData)[i + 0]].pos;
			vert1 = &geoRes->getVertexData()[((uint32 *)geoRes->_indexData)[i + 1]].pos;
			vert2 = &geoRes->getVertexData()[((uint32 *)geoRes->_indexData)[i + 2]].pos;
		}

      Plane p(*vert0, *vert1, *vert2);

      if (p.distToPoint(orig) < 0) {
         continue;
      }
		
		if( rayTriangleIntersection( orig, dir, *vert0, *vert1, *vert2, intsPos ) )
		{
			intersection = true;
			if( (intsPos - orig).length() < (nearestIntsPos - orig).length() ) {
				nearestIntsPos = intsPos;
            intsNorm = (*vert1 - *vert0).cross(*vert2 - *vert1);
         }
		}
	}

	intsPos = _absTrans * nearestIntsPos;
	
	return intersection;
}


void VoxelMeshNode::onAttach( SceneNode &parentNode )
{
	// Find parent model node
	SceneNode *node = &parentNode;
	while( node->getType() != SceneNodeTypes::VoxelModel ) node = node->getParent();
	_parentModel = (VoxelModelNode *)node;
	_parentModel->markNodeListDirty();
}


void VoxelMeshNode::onDetach( SceneNode &/*parentNode*/ )
{
	if( _parentModel != 0x0 ) _parentModel->markNodeListDirty();
}


void VoxelMeshNode::onPostUpdate()
{
	_bBox = _localBBox;
	_bBox.transform( _absTrans );
}

}  // namespace
