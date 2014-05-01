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
	SceneNode( modelTpl ), _geometryRes( modelTpl.geoRes ), _baseVoxelGeoRes( 0x0 ),
	_lodDist1( modelTpl.lodDist1 ), _lodDist2( modelTpl.lodDist2 ),
	_lodDist3( modelTpl.lodDist3 ), _lodDist4( modelTpl.lodDist4 ),
	_softwareSkinning( modelTpl.softwareSkinning ), _skinningDirty( false ),
	_nodeListDirty( false ), _morpherUsed( false ), _morpherDirty( false ), _polygon_offset_used(false),
   _useCoarseCollisionBox(false)
{
	if( _geometryRes != 0x0 )
		setParamI( VoxelModelNodeParams::VoxelGeoResI, _geometryRes->getHandle() );
}


VoxelModelNode::~VoxelModelNode()
{
	_geometryRes = 0x0;
	_baseVoxelGeoRes = 0x0;
}


SceneNodeTpl *VoxelModelNode::parsingFunc( map< string, std::string > &attribs )
{
	bool result = true;
	
	VoxelModelNodeTpl *modelTpl = new VoxelModelNodeTpl( "", 0x0 );
	
	map< string, std::string >::iterator itr = attribs.find( "geometry" );
	if( itr != attribs.end() )
	{
		uint32 res = Modules::resMan().addResource( ResourceTypes::VoxelGeometry, itr->second, 0, false );
		if( res != 0 )
			modelTpl->geoRes = (VoxelGeometryResource *)Modules::resMan().resolveResHandle( res );
	}
	else result = false;
	itr = attribs.find( "softwareSkinning" );
	if( itr != attribs.end() ) 
	{
		if ( _stricmp( itr->second.c_str(), "true" ) == 0 || _stricmp( itr->second.c_str(), "1" ) == 0 )
			modelTpl->softwareSkinning = true;
		else
			modelTpl->softwareSkinning = false;
	}

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


void VoxelModelNode::recreateNodeListRec( SceneNode *node, bool firstCall )
{
	if( node->getType() == SceneNodeTypes::VoxelMesh )
	{
		_meshList.push_back( (VoxelMeshNode *)node );
//		_animCtrl.registerNode( (VoxelMeshNode *)node );
	}
	else if( !firstCall ) return;  // First node is the model
	
	// Children
	for( size_t i = 0, s = node->getChildren().size(); i < s; ++i )
	{
		recreateNodeListRec( node->getChildren()[i], false );
	}
}


void VoxelModelNode::recreateNodeList()
{
	_meshList.resize( 0 );
//	_animCtrl.clearNodeList();
	
	recreateNodeListRec( this, true );
	updateLocalMeshAABBs();

	_nodeListDirty = false;
}


void VoxelModelNode::setupAnimStage( int stage, AnimationResource *anim, int layer,
                                std::string const& startNode, bool additive )
{
	if( _nodeListDirty ) recreateNodeList();
	
//	if( _animCtrl.setupAnimStage( stage, anim, layer, startNode, additive ) ) markDirty();
}


void VoxelModelNode::setAnimParams( int stage, float time, float weight )
{
//	if( _animCtrl.setAnimParams( stage, time, weight ) ) markDirty();
}


bool VoxelModelNode::setMorphParam( std::string const& targetName, float weight )
{
	if( _geometryRes == 0x0 || _morphers.empty() ) return false;

	bool result = false;
	_morpherDirty = true;
	_morpherUsed = false;

	// Set specified morph target or all targets if targetName == ""
	for( uint32 i = 0; i < _morphers.size(); ++i )
	{
		if( targetName == "" || _morphers[i].name == targetName )
		{
			_morphers[i].weight = weight;
			result = true;
		}

		if( _morphers[i].weight != 0 ) _morpherUsed = true;
	}

	markDirty();

	return result;
}


void VoxelModelNode::updateLocalMeshAABBs()
{
	if( _geometryRes == 0x0 ) return;
	
	// Update local mesh AABBs
	for( uint32 i = 0, s = (uint32)_meshList.size(); i < s; ++i )
	{
		VoxelMeshNode &mesh = *_meshList[i];
		
      mesh._localBBox.clear();
		
      if( mesh.getVertRStart(0) < _geometryRes->getVertCount() &&
		    mesh.getVertREnd(0) < _geometryRes->getVertCount() )
		{
         for( uint32 j = mesh.getVertRStart(0); j <= mesh.getVertREnd(0); ++j )
			{
				Vec3f &vertPos = _geometryRes->getVertexData()[j].pos;
            mesh._localBBox.addPoint(vertPos);
			}

			// Avoid zero box dimensions for planes
         mesh._localBBox.feather();
		}
	}
}


void VoxelModelNode::setVoxelGeometryRes( VoxelGeometryResource &geoRes )
{
	// Copy morph targets
#if 0
	_morphers.resize( geoRes._morphTargets.size() );
	for( uint32 i = 0; i < _morphers.size(); ++i )
	{	
		Morpher &morpher = _morphers[i]; 
		
		morpher.name = geoRes._morphTargets[i].name;
		morpher.index = i;
		morpher.weight = 0;
	}
#endif

	if( !_morphers.empty() || _softwareSkinning )
	{
		Resource *clonedRes = Modules::resMan().resolveResHandle(
			Modules::resMan().cloneResource( geoRes, "" ) );
		_geometryRes = (VoxelGeometryResource *)clonedRes;
		_baseVoxelGeoRes = &geoRes;
	}
	else
	{
		_geometryRes = &geoRes;
		_baseVoxelGeoRes = 0x0;
	}

	_skinningDirty = true;
	updateLocalMeshAABBs();
}


int VoxelModelNode::getParamI( int param )
{
	switch( param )
	{
	case VoxelModelNodeParams::VoxelGeoResI:
		return _geometryRes != 0x0 ? _geometryRes->_handle : 0;
	case VoxelModelNodeParams::SWSkinningI:
		return _softwareSkinning ? 1 : 0;
	}

	return SceneNode::getParamI( param );
}


void VoxelModelNode::setParamI( int param, int value )
{
	Resource *res;
	
	switch( param )
	{
	case VoxelModelNodeParams::VoxelGeoResI:
		res = Modules::resMan().resolveResHandle( value );
		if( res != 0x0 && res->getType() == ResourceTypes::VoxelGeometry )
			setVoxelGeometryRes( *(VoxelGeometryResource *)res );
		else
			Modules::setError( "Invalid handle in h3dSetNodeParamI for H3DVoxelModel::VoxelGeoResI" );
		return;
	case VoxelModelNodeParams::SWSkinningI:
		_softwareSkinning = (value != 0);
		if( _softwareSkinning ) _skinningDirty = true;
		if( _softwareSkinning && _baseVoxelGeoRes == 0x0 && _geometryRes != 0x0 )
			// Create a local resource copy since it is not yet existing
			setParamI( VoxelModelNodeParams::VoxelGeoResI, _geometryRes->getHandle() );
		else if( !_softwareSkinning && _morphers.empty() && _baseVoxelGeoRes != 0x0 )
			// Remove the local resource copy by removing reference
			setParamI( VoxelModelNodeParams::VoxelGeoResI, _baseVoxelGeoRes->getHandle() );
		return;
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
   case VoxelModelNodeParams::PolygonOffsetF:
      if (compIdx >= 0 && compIdx < 2) {
         _polygon_offset[compIdx] = value;
      }
      return;
	}

	SceneNode::setParamF( param, compIdx, value );
}


bool VoxelModelNode::updateVoxelGeometry()
{
	_skinningDirty |= _morpherDirty;
	_skinningDirty &= _softwareSkinning;
	
	if( !_skinningDirty && !_morpherDirty ) return false;

	if( _baseVoxelGeoRes == 0x0 || _baseVoxelGeoRes->getVertexData() == 0x0) return false;
	if( _geometryRes == 0x0 || _geometryRes->getVertexData() == 0x0) return false;
	
	Timer *timer = Modules::stats().getTimer( EngineStats::GeoUpdateTime );
	if( Modules::config().gatherTimeStats ) timer->setEnabled( true );
	
	// Reset vertices to base data
	memcpy( _geometryRes->getVertexData(), _baseVoxelGeoRes->getVertexData(),
	        _geometryRes->_vertCount * sizeof( VoxelVertexData ) );

	VoxelVertexData *posData = _geometryRes->getVertexData();

#if 1
   ASSERT(false);
#else
	if( _morpherUsed )
	{
		// Recalculate vertex positions for morph targets
		for( uint32 i = 0; i < _morphers.size(); ++i )
		{
			if( _morphers[i].weight > Math::Epsilon )
			{
				MorphTarget &mt = _geometryRes->_morphTargets[_morphers[i].index];
				float weight = _morphers[i].weight;
				
				for( uint32 j = 0; j < mt.diffs.size(); ++j )
				{
					MorphDiff &md = mt.diffs[j];
					
					posData[md.vertIndex] += md.posDiff * weight;
					tanData[md.vertIndex].normal += md.normDiff * weight;
					tanData[md.vertIndex].tangent += md.tanDiff * weight;
				}
			}
		}
	}

	if( _skinningDirty )
	{
		Matrix4f skinningMat;
		Vec4f *rows = &_skinMatRows[0];

		for( uint32 i = 0, s = _geometryRes->getVertCount(); i < s; ++i )
		{
			Vec4f *row0 = &rows[ftoi_r( staticData[i].jointVec[0] ) * 3];
			Vec4f *row1 = &rows[ftoi_r( staticData[i].jointVec[1] ) * 3];
			Vec4f *row2 = &rows[ftoi_r( staticData[i].jointVec[2] ) * 3];
			Vec4f *row3 = &rows[ftoi_r( staticData[i].jointVec[3] ) * 3];

			Vec4f weights = *((Vec4f *)&staticData[i].weightVec[0]);

			skinningMat.x[0] = (row0)->x * weights.x + (row1)->x * weights.y + (row2)->x * weights.z + (row3)->x * weights.w;
			skinningMat.x[1] = (row0+1)->x * weights.x + (row1+1)->x * weights.y + (row2+1)->x * weights.z + (row3+1)->x * weights.w;
			skinningMat.x[2] = (row0+2)->x * weights.x + (row1+2)->x * weights.y + (row2+2)->x * weights.z + (row3+2)->x * weights.w;
			skinningMat.x[4] = (row0)->y * weights.x + (row1)->y * weights.y + (row2)->y * weights.z + (row3)->y * weights.w;
			skinningMat.x[5] = (row0+1)->y * weights.x + (row1+1)->y * weights.y + (row2+1)->y * weights.z + (row3+1)->y * weights.w;
			skinningMat.x[6] = (row0+2)->y * weights.x + (row1+2)->y * weights.y + (row2+2)->y * weights.z + (row3+2)->y * weights.w;
			skinningMat.x[8] = (row0)->z * weights.x + (row1)->z * weights.y + (row2)->z * weights.z + (row3)->z * weights.w;
			skinningMat.x[9] = (row0+1)->z * weights.x + (row1+1)->z * weights.y + (row2 + 1)->z * weights.z + (row3+1)->z * weights.w;
			skinningMat.x[10] = (row0+2)->z * weights.x + (row1+2)->z * weights.y + (row2+2)->z * weights.z + (row3+2)->z * weights.w;
			skinningMat.x[12] = (row0)->w * weights.x + (row1)->w * weights.y + (row2)->w * weights.z + (row3)->w * weights.w;
			skinningMat.x[13] = (row0+1)->w * weights.x + (row1+1)->w * weights.y + (row2+1)->w * weights.z + (row3+1)->w * weights.w;
			skinningMat.x[14] = (row0+2)->w * weights.x + (row1+2)->w * weights.y + (row2+2)->w * weights.z + (row3+2)->w * weights.w;

			// Skin position
			posData[i] = skinningMat * posData[i];

			// Skin tangent space basis
			// Note: We skip the normalization of the tangent space basis for performance reasons;
			//       the error is usually not huge and should be hardly noticable
			tanData[i].normal = skinningMat.mult33Vec( tanData[i].normal ); //.normalized();
			tanData[i].tangent = skinningMat.mult33Vec( tanData[i].tangent ); //.normalized();
		}
	}
	else if( _morpherUsed )
	{
		// Renormalize tangent space basis
		for( uint32 i = 0, s = _geometryRes->getVertCount(); i < s; ++i )
		{
			tanData[i].normal.Normalize();
			tanData[i].tangent.Normalize();
		}
	}
#endif

	_morpherDirty = false;
	_skinningDirty = false;
	
	// Upload geometry
	_geometryRes->updateDynamicVertData();

	timer->setEnabled( false );

	return true;
}

void VoxelModelNode::onPostUpdate()
{
	if( _nodeListDirty ) recreateNodeList();

#if 0
   // Put this back when we use this thing for geo with morph targets
	if( _animCtrl.animate() )
	{	
		_skinningDirty = true;
		markDirty();
	}
#endif
}


void VoxelModelNode::onFinishedUpdate()
{
	// Update AABBs of skinned meshes
	if( _skinningDirty && !_geometryRes != 0x0 )
	{
		for( uint32 i = 0, s = (uint32)_meshList.size(); i < s; ++i )
		{
			_meshList[i]->_bBox = _meshList[i]->_localBBox;
			_meshList[i]->_bBox.transform( _meshList[i]->_absTrans );
		}
	}

	// Calculate model AABB from mesh AABBs
	_bBox.clear();
	for( uint32 i = 0, s = (uint32)_meshList.size(); i < s; ++i )
	{
		_bBox.makeUnion( _meshList[i]->_bBox ); 
	}

	// Update geometry for morphers or software skinning
	updateVoxelGeometry();
}

}  // namespace
