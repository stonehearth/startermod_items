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

#include "radiant.h"
#include "radiant_logger.h"
#include "egScene.h"
#include "egSceneGraphRes.h"
#include "egLight.h"
#include "egCamera.h"
#include "egParticle.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"

#include "utDebug.h"
#include "lib/perfmon/perfmon.h"
#include <boost/math/special_functions/round.hpp>

#define SCENE_LOG(level)      LOG(horde.scene, level)

static const int MaxActiveRenderQueues = 1000;

#define RENDER_NODES 0
#define LIGHT_NODES 1

namespace Horde3D {

using namespace std;

static std::string FlagsToString(int flags)
{
   std::ostringstream s;
   s << "(flags (" << flags << "): ";
#define ADD_FLAG(f) if (flags & SceneNodeFlags::f) { s << #f << " "; }
   ADD_FLAG(NoDraw)
   ADD_FLAG(NoCastShadow)
   ADD_FLAG(NoRayQuery)
#undef ADD_FLAG
   s << ")";
   return s.str();
};


InstanceKey::InstanceKey() {
   geoResource = nullptr;
   matResource = nullptr;
   node = nullptr;
   scale = 1.0;
}

void InstanceKey::updateHash() {
   ASSERT(node != nullptr);
   hash = (uint32)(((uintptr_t)(geoResource) ^ (uintptr_t)(matResource)) >> 2) ^ (uint32)(101 * scale);
   Modules::sceneMan().sceneForNode(node->getHandle()).updateNodeInstanceKey(*node);
}

bool InstanceKey::operator==(const InstanceKey& other) const {
   return geoResource == other.geoResource &&
      matResource == other.matResource &&
      scale == other.scale;
}
bool InstanceKey::operator!=(const InstanceKey& other) const {
   return !(other == *this);
}


// *************************************************************************************************
// Class SceneNode
// *************************************************************************************************

SceneNode::SceneNode( const SceneNodeTpl &tpl ) :
	_parent( 0x0 ), _type( tpl.type ), _handle( 0 ), _flags( 0 ), _sortKey( 0 ),
   _dirty( SceneNodeDirtyState::Dirty ), _transformed( true ), _renderable( false ),
   _name( tpl.name ), _attachment( tpl.attachmentString ), _userFlags(0), _accumulatedFlags(0),
   _gridId(-1), _gridPos(-1), _noInstancing(true)
{
	_relTrans = Matrix4f::ScaleMat( tpl.scale.x, tpl.scale.y, tpl.scale.z );
	_relTrans.rotate( degToRad( tpl.rot.x ), degToRad( tpl.rot.y ), degToRad( tpl.rot.z ) );
	_relTrans.translate( tpl.trans.x, tpl.trans.y, tpl.trans.z );
}


SceneNode::~SceneNode()
{
}


void SceneNode::getTransform( Vec3f &trans, Vec3f &rot, Vec3f &scale )
{
   if (_dirty != SceneNodeDirtyState::Clean) {
      Modules::sceneMan().sceneForNode(_handle).updateNodes();
   }
	
	_relTrans.decompose( trans, rot, scale );
	rot.x = radToDeg( rot.x );
	rot.y = radToDeg( rot.y );
	rot.z = radToDeg( rot.z );
}

void SceneNode::getTransformFast( Vec3f &trans, Vec3f &rot, Vec3f &scale ) const
{
	_relTrans.decompose( trans, rot, scale );
	rot.x = radToDeg( rot.x );
	rot.y = radToDeg( rot.y );
	rot.z = radToDeg( rot.z );
}

void SceneNode::setTransform( Vec3f trans, Vec3f rot, Vec3f scale )
{
	// Hack to avoid making setTransform virtual
	if( _type == SceneNodeTypes::Joint )
	{
		((JointNode *)this)->_parentModel->_skinningDirty = true;
		((JointNode *)this)->_ignoreAnim = true;
	}
	else if( _type == SceneNodeTypes::Mesh )
	{
		((MeshNode *)this)->_ignoreAnim = true;
	}
	
   SCENE_LOG(9) << "setting relative transform for node " << _name << " to " << "(" << trans.x << ", " << trans.y << ", " << trans.z << ")";

	Matrix4f newRelTrans = Matrix4f::ScaleMat( scale.x, scale.y, scale.z );
	newRelTrans.rotate( degToRad( rot.x ), degToRad( rot.y ), degToRad( rot.z ) );
	newRelTrans.translate( trans.x, trans.y, trans.z );

   if (newRelTrans == _relTrans) {
      return;
   }

   _relTrans = newRelTrans;

   if (_relTrans.c[3][0] != 0.0f && !(_relTrans.c[3][0] < 0.0f) && !(_relTrans.c[3][0] > 0.0f)) {
      Modules::log().writeDebugInfo( "Invalid transform! Zero edition.");
      DEBUG_ONLY( DebugBreak();)
   }

   markDirty(SceneNodeDirtyKind::All);
}


void SceneNode::setTransform( const Matrix4f &mat )
{
	// Hack to avoid making setTransform virtual
	if( _type == SceneNodeTypes::Joint )
	{
		((JointNode *)this)->_parentModel->_skinningDirty = true;
		((JointNode *)this)->_ignoreAnim = true;
	}
	else if( _type == SceneNodeTypes::Mesh )
	{
		((MeshNode *)this)->_parentModel->_skinningDirty = true;
		((MeshNode *)this)->_ignoreAnim = true;
	}
	
   if (_relTrans == mat) {
      return;
   }
	_relTrans = mat;

   if (_relTrans.c[3][0] != 0.0f && !(_relTrans.c[3][0] < 0.0f) && !(_relTrans.c[3][0] > 0.0f)) {
      Modules::log().writeDebugInfo( "Invalid transform! Zero edition.");
      DEBUG_ONLY( DebugBreak();)
   }
	
   markDirty(SceneNodeDirtyKind::All);
}


void SceneNode::getTransMatrices( const float **relMat, const float **absMat ) const
{
	if( relMat != 0x0 )
	{
		if(_dirty != SceneNodeDirtyState::Clean) {
         Modules::sceneMan().sceneForNode(_handle).updateNodes();
      }
		*relMat = &_relTrans.x[0];
	}
	
	if( absMat != 0x0 )
	{
		if( _dirty != SceneNodeDirtyState::Clean ) {
         Modules::sceneMan().sceneForNode(_handle).updateNodes();
      }
		*absMat = &_absTrans.x[0];
	}
}

void SceneNode::updateAccumulatedFlags()
{
   int parentFlags = _parent ? _parent->_accumulatedFlags : 0;

   int oldFlags = _accumulatedFlags;
   _accumulatedFlags = _flags | parentFlags;

   if (_accumulatedFlags & SceneNodeFlags::NoDraw) {
      if ((oldFlags & SceneNodeFlags::NoDraw) == 0) {
         Modules::sceneMan().sceneForNode(_handle).setNodeHidden(*this, true);
      }
   } else {
      if (oldFlags & SceneNodeFlags::NoDraw) {
         Modules::sceneMan().sceneForNode(_handle).setNodeHidden(*this, false);
      }
   }
}

void SceneNode::setFlags( int flags, bool recursive )
{
   if (flags == _flags && !recursive) {
      SCENE_LOG(8) << "ignorning identity, non-recursive set flags on node " << _handle << " to " << FlagsToString(flags);
      return;
   }
   updateFlagsRecursive(flags, SET, recursive);
}

void SceneNode::twiddleFlags( int flags, bool on, bool recursive )
{
   updateFlagsRecursive(flags, on ? TWIDDLE_ON : TWIDDLE_OFF, recursive);
}

void SceneNode::updateFlagsRecursive(int flags, SetFlagsMode mode, bool recursive)
{
   int oldFlags = _flags;
   if (mode == SET) {
      _flags = flags;
      SCENE_LOG(8) << "setting flags on node " << _handle << " to " << FlagsToString(flags);
   } else if (mode == TWIDDLE_ON) {
      _flags |= flags;
      SCENE_LOG(8) << "twiddling " << FlagsToString(flags) << " flags on node " << _handle << " (was:" << FlagsToString(oldFlags) << " now:" << FlagsToString(_flags);
   } else if (mode == TWIDDLE_OFF) {
      _flags &= ~flags;
      SCENE_LOG(8) << "twiddling " << FlagsToString(flags) << " flags on node " << _handle << " (was:" << FlagsToString(oldFlags) << " now:" << FlagsToString(_flags);
   }

   // Update our accmulated flags and iterate through all our children
   // unconditionally.  `recursive` refers only to the flags.  if it's
   // off, switch our mode to NOP on the next call so we'll just 
   // accumulate flags all the way down.
   updateAccumulatedFlags();
   for (SceneNode* child : _children) {
      child->updateFlagsRecursive(flags, recursive ? mode : NOP, recursive);
   }
}

int SceneNode::getParamI(int param) const
{
   switch (param) {
   case SceneNodeParams::UserFlags:
      return _userFlags;
   }
	Modules::setError( "Invalid param in h3dGetNodeParamI" );
	return Math::MinInt32;
}

void SceneNode::setParamI( int param, int value )
{
   switch (param)
   {
   case SceneNodeParams::UserFlags:
      _userFlags = value;
      return;
   }
	Modules::setError( "Invalid param in h3dSetNodeParamI" );
}

float SceneNode::getParamF( int param, int compIdx )
{
	Modules::setError( "Invalid param in h3dGetNodeParamF" );
	return Math::NaN;
}

void SceneNode::setParamF( int param, int compIdx, float value )
{
	Modules::setError( "Invalid param in h3dSetNodeParamF" );
}

const char *SceneNode::getParamStr( int param )
{
	switch( param )
	{
	case SceneNodeParams::NameStr:
		return _name.c_str();
	case SceneNodeParams::AttachmentStr:
		return _attachment.c_str();
	}

	Modules::setError( "Invalid param in h3dGetNodeParamStr" );
	return "";
}

void SceneNode::setParamStr( int param, const char *value )
{
	switch( param )
	{
	case SceneNodeParams::NameStr:
		_name = value;
		return;
	case SceneNodeParams::AttachmentStr:
		_attachment = value;
		return;
	}

	Modules::setError( "Invalid param in h3dSetNodeParamStr" );
}

void* SceneNode::mapParamV(int param)
{
	Modules::setError( "Invalid param in h3dMapParamV" );
   return nullptr;
}

void SceneNode::unmapParamV(int param, int mappedLength)
{
	Modules::setError( "Invalid param in h3dUnmapParamV" );
}

void SceneNode::updateBBox(const BoundingBox& b)
{
   _bBox = b;
   markDirty(SceneNodeDirtyKind::Ancestors);
}


void SceneNode::setLodLevel( int lodLevel )
{
}


bool SceneNode::canAttach( SceneNode &/*parent*/ )
{
	return true;
}


void SceneNode::markChildrenDirty()
{
   for (const auto& child : _children)
   {      
      if (child->_dirty != SceneNodeDirtyState::Dirty) {
         SCENE_LOG(9) << "marking  child node (handle:" << child->_handle << " name:" << child->_name << " dirty via markChildrenDirty ";
         child->_dirty = SceneNodeDirtyState::Dirty;
		   child->_transformed = true;
		   child->markChildrenDirty();
      } else {
         SCENE_LOG(9) << "skipping child node (handle:" << child->_handle << " name:" << child->_name << " dirty via markChildrenDirty (already dirty)";
      }
	}
}

void SceneNode::markDirty(uint32 dirtyKind)
{
	_transformed = true;

   // To start, we assume our children are partly dirty, partly clean, and hence we are partial.
   _dirty = SceneNodeDirtyState::Partial;

   SCENE_LOG(9) << "marking node (handle:" << _handle << " name:" << _name << " dirty";

   if (dirtyKind & SceneNodeDirtyKind::Children) {
      // If we mark all children as dirty, then we are dirty.
   	markChildrenDirty();
      _dirty = SceneNodeDirtyState::Dirty;
   }

   if (dirtyKind & SceneNodeDirtyKind::Ancestors) {
	   SceneNode *node = _parent;
	   while( node != 0x0 )
	   {
         SCENE_LOG(9) << "marking ancestor node (handle:" << node->_handle << " name:" << node->_name << " dirty via SceneNodeDirtyKind::Ancestors";

         // At first, assume the ancestor is dirty, and try to prove it isn't.
         node->_dirty = SceneNodeDirtyState::Dirty;

         for (const auto& child : node->_children) {
            if (child->_dirty != SceneNodeDirtyState::Dirty) {
               // If any child is NOT dirty, then the ancestor cannot be dirty.
               node->_dirty = SceneNodeDirtyState::Partial;
               break;
            }
         }

		   node = node->_parent;
	   }
   }
}


void SceneNode::update()
{
   if( _dirty == SceneNodeDirtyState::Clean ) return;

   SCENE_LOG(9) << "updating node (handle:" << _handle << " name:" << _name << ")";

	onPreUpdate();
	
	// Calculate absolute matrix
	if( _parent != 0x0 ) {
		Matrix4f::fastMult43( _absTrans, _parent->_absTrans, _relTrans );
      SCENE_LOG(9) << "calculated absolute transform for (handle:" << _handle << " name:" << _name << " to " << "(" << _absTrans.c[3][0] << ", " << _absTrans.c[3][1] << ", " << _absTrans.c[3][2] << ") matrix addr:" << (void*)&_absTrans.c;
	} else {
		_absTrans = _relTrans;
   }
   if (_absTrans.c[3][0] != 0.0f && !(_absTrans.c[3][0] < 0.0f) && !(_absTrans.c[3][0] > 0.0f)) {
      DebugBreak();
   }

   // :update() is called in-order on all the nodes in the tree, starting at the root.
   // so by now we're guarantee that all our parent nodes have been updated, including
   // the accumualted flags.  simply accumulate our parents flags into ours and continue
   // the descent.
   updateAccumulatedFlags();
	
	onPostUpdate();

   _dirty = SceneNodeDirtyState::Clean;

   // Visit children
	for( uint32 i = 0, s = (uint32)_children.size(); i < s; ++i )
	{
		_children[i]->update();
	}	

	onFinishedUpdate();

   Modules::sceneMan().sceneForNode(_handle).updateSpatialNode(*this);
}


bool SceneNode::checkIntersection(const Vec3f &rayOrig, const Vec3f& rayEnd, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm) const
{
   if (!segmentIntersectsAABB(rayOrig, rayEnd, _bBox.min(), _bBox.max())) {
      return false;
   }

   return checkIntersectionInternal(rayOrig, rayDir, intsPos, intsNorm);
}

bool SceneNode::checkIntersectionInternal( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const
{
   return false;
}

// *************************************************************************************************
// Class GroupNode
// *************************************************************************************************

GroupNode::GroupNode( const GroupNodeTpl &groupTpl ) :
	SceneNode( groupTpl )
{
   //_bBox.addPoint(Vec3f(0, 0, 0));
}


SceneNodeTpl *GroupNode::parsingFunc( map< string, std::string > &attribs )
{
	map< string, std::string >::iterator itr;
	GroupNodeTpl *groupTpl = new GroupNodeTpl( "" );
	
	return groupTpl;
}


SceneNode *GroupNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::Group ) return 0x0;
	
	return new GroupNode( *(GroupNodeTpl *)&nodeTpl );
}

struct RendQueueItemCompFunc
{
	bool operator()( const RendQueueItem &a, const RendQueueItem &b ) const
		{ return a.sortKey < b.sortKey; }
};


// =================================================================================================
// Class GridSpatialGraph
// =================================================================================================

#define GRIDSIZE 150
#define I_GRIDSIZE (1.0f / GRIDSIZE)

GridSpatialGraph::GridSpatialGraph(SceneId sceneId)
{
   _sceneId = sceneId;
}


GridItem* GridSpatialGraph::gridItemForNode(SceneNode const& node)
{
   if (node._gridPos == -1) {
      return nullptr;
   }
   GridItem *gi;
    
   if (node._gridId == -1) {
      if (node._accumulatedFlags & SceneNodeFlags::NoCull) {
         gi = &_noCullNodes[node._gridPos];
      } else {
         gi = &_spilloverNodes[RENDER_NODES].at(node._gridPos);
      }
   } else {
      gi = &_gridElements.at(node._gridId).nodes[RENDER_NODES].at(node._gridPos);
   }

   return gi;
}


void GridSpatialGraph::addNode(SceneNode& sceneNode)
{	
   radiant::perfmon::TimelineCounterGuard un("gsg:addNode");
   const NodeHandle nh = sceneNode.getHandle();
   if(!sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light) {
      return;
   }

   if (sceneNode.getType() == SceneNodeTypes::Light && ((LightNode*)&sceneNode)->getParamI(LightNodeParams::DirectionalI)) {
      _directionalLights[nh] = &sceneNode;
   } else {
      updateNode(sceneNode);
   }
}


void GridSpatialGraph::updateNodeInstanceKey(SceneNode& sceneNode)
{
   GridItem *gi = gridItemForNode(sceneNode);
   if (gi == nullptr) {
      // It's trivial for nodes being created to not have a valid bounding box yet (mesh nodes can have geometry attached at a later
      // time, for example).  So, don't worry about them; changes to their bounding box will trigger this update.
      return;
   }

   for (int i = 0; i < RenderCacheSize; i++) {
      if (sceneNode.getInstanceKey() != 0x0) {
         InstanceRenderableQueues &iqs = Modules::renderer()._instanceQueues[i];
         auto iqsIt = iqs.find(sceneNode._type);
         if (iqsIt == iqs.end()) {
            InstanceRenderableQueue newQ;
            iqsIt = iqs.emplace_hint(iqsIt, sceneNode._type, newQ);
         }
         InstanceRenderableQueue& iq = iqsIt->second;

         const InstanceKey& ik = *(sceneNode.getInstanceKey());
         auto iqIt = iq.find(ik);
         if (iqIt == iq.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            iqIt = iq.emplace_hint(iqIt, ik, std::make_shared<RenderableQueue>(newQ));
         }
         gi->renderQueues[i] = iqIt->second.get();
      } else {
         RenderableQueues &rqs = Modules::renderer()._singularQueues[i];
         auto rqIt = rqs.find(sceneNode._type);
         if (rqIt == rqs.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            rqIt = rqs.emplace_hint(rqIt, sceneNode._type, std::make_shared<RenderableQueue>(newQ));
         }
         gi->renderQueues[i] = rqIt->second.get();
      }
   }
}

void GridSpatialGraph::swapAndRemove(std::vector<GridItem>& vec, int index) {
   if (vec.size() > 1) {
      vec[index] = vec.back();
   }

   vec.back().node->_gridPos = index;
   vec.pop_back();
}

void GridSpatialGraph::removeNode(SceneNode& sceneNode)
{
   radiant::perfmon::TimelineCounterGuard un("gsg:removeNode");
   NodeHandle h = sceneNode.getHandle();
   if (sceneNode.getType() == SceneNodeTypes::Light && ((LightNode*)&sceneNode)->getParamI(LightNodeParams::DirectionalI)) {
      _directionalLights.erase(h);
   } else {
      if (sceneNode._gridPos == -1) {
         return;
      }

      std::vector<GridItem>* vec;
      if (sceneNode._accumulatedFlags & SceneNodeFlags::NoCull) {
         vec = &_noCullNodes;
      } else {
         const int nodeType = sceneNode._renderable ? RENDER_NODES : LIGHT_NODES;
         if (sceneNode._gridId == -1) {
            vec = &_spilloverNodes[nodeType];
         } else {
            vec = &_gridElements[sceneNode._gridId].nodes[nodeType];
         }
      }

      swapAndRemove(*vec, sceneNode._gridPos);

      sceneNode._gridId = -1;
      sceneNode._gridPos = -1;
   }
}

int GridSpatialGraph::boundingBoxToGrid(BoundingBox const& aabb) const
{
   Vec3f const& min = aabb.min();
   Vec3f const& max = aabb.max();
   
   const float minx = min.x * I_GRIDSIZE;
   const float minz = min.z * I_GRIDSIZE;
   const float maxx = max.x * I_GRIDSIZE;
   const float maxz = max.z * I_GRIDSIZE;

   const int minX = minx < 0 ? (int) minx == minx ? (int) minx : (int) minx - 1 : (int) minx;
   const int minZ = minz < 0 ? (int) minz == minz ? (int) minz : (int) minz - 1 : (int) minz;
   const int maxX = maxx < 0 ? (int) maxx == maxx ? (int) maxx : (int) maxx - 1 : (int) maxx;
   const int maxZ = maxz < 0 ? (int) maxz == maxz ? (int) maxz : (int) maxz - 1 : (int) maxz;

   ASSERT(maxX >= minX);
   ASSERT(maxZ >= minZ);

   if (minX != maxX || minZ != maxZ) {
      return -1;
   }

   return hashGridPoint(minX, minZ);
}


inline uint32 GridSpatialGraph::hashGridPoint(int x, int y) const
{
   uint32 lx = x + 0x8000;
   uint32 ly = y + 0x8000;
   return lx | (ly << 16);
}


inline void GridSpatialGraph::unhashGridHash(uint32 hash, int* x, int* y) const
{
   *x = (hash & 0xFFFF) - 0x8000;
   *y = (hash >> 16) - 0x8000;
}

void GridSpatialGraph::updateNode(SceneNode& sceneNode)
{
   radiant::perfmon::TimelineCounterGuard un("gsg:updateNode");

   if (sceneNode._accumulatedFlags & SceneNodeFlags::NoDraw) {
      return;
   }

   if (!sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light) {
      return;
   }

   BoundingBox const& sceneBox = sceneNode.getBBox();
   const int nodeType = sceneNode._renderable ? RENDER_NODES : LIGHT_NODES;
   
   if (sceneNode.getFlags() & SceneNodeFlags::NoCull) {
      ASSERT(sceneNode._gridId == -1);
      if (sceneNode._gridPos == -1) {
         _noCullNodes.emplace_back(GridItem(sceneBox, &sceneNode));
         sceneNode._gridPos = (int)(_noCullNodes.size() - 1);
         updateNodeInstanceKey(sceneNode);
      } else { 
         _noCullNodes.at(sceneNode._gridPos).bounds = sceneBox;
      }
   } else {
      if (!sceneBox.isValid()) {
         return;
      }
      int gridId = boundingBoxToGrid(sceneBox);

      if (sceneNode._gridPos >= 0 && gridId == sceneNode._gridId) {
         // We haven't moved; update the bounds and go away.
         std::vector<GridItem>* vec;
         if (gridId == -1) {
            vec = &_spilloverNodes[nodeType];
         } else {
            vec = &_gridElements.at(gridId).nodes[nodeType];
            const Vec3f& boundsMin = sceneBox.min();
            const Vec3f& boundsMax = sceneBox.max();

            _gridElements.at(gridId).bounds.addPoint(boundsMin);
            _gridElements.at(gridId).bounds.addPoint(boundsMax);
         }
         vec->at(sceneNode._gridPos).bounds = sceneBox;
         return;
      }

      std::vector<GridItem>* oldVec;
      std::vector<GridItem>* newVec;
      if (sceneNode._gridId == -1) {
         oldVec = &_spilloverNodes[nodeType];
      } else {
         oldVec = &_gridElements.at(sceneNode._gridId).nodes[nodeType];
      }
      
      if (gridId == -1) {
         newVec = &_spilloverNodes[nodeType];
      } else {
         auto& e = _gridElements.find(gridId);
         if (e == _gridElements.end()) {
            GridElement newGridElement;
            int gX, gY;
            unhashGridHash(gridId, &gX, &gY);
            newGridElement.bounds.addPoint(Vec3f((float)gX * GRIDSIZE, sceneBox.min().y, (float)gY * GRIDSIZE));
            newGridElement.bounds.addPoint(Vec3f((float)gX * GRIDSIZE + GRIDSIZE, sceneBox.max().y, (float)gY * GRIDSIZE + GRIDSIZE));
            
            std::pair<uint32, GridElement> p(gridId, newGridElement);
            e = _gridElements.insert(e, p);
         }
         const Vec3f& boundsMin = sceneBox.min();
         const Vec3f& boundsMax = sceneBox.max();
         e->second.bounds.addPoint(boundsMin);
         e->second.bounds.addPoint(boundsMax);
         newVec = &e->second.nodes[nodeType];
      }
      // Remove the node from the old grid list by swapping with the rear element;
      if (sceneNode._gridPos >= 0) {
         swapAndRemove(*oldVec, sceneNode._gridPos);
      }

      newVec->emplace_back(GridItem(sceneBox, &sceneNode));
      sceneNode._gridId = gridId;
      sceneNode._gridPos = (int)(newVec->size() - 1);

      // We moved to a new grid element, so initialize those render queues appropriately.
      updateNodeInstanceKey(sceneNode);
   }
}


void GridSpatialGraph::castRay(const Vec3f& rayOrigin, const Vec3f& rayDirection, std::function<void(std::vector<GridItem> const& nodes)> cb) {
   Vec3f rayDirN = rayDirection.normalized();
   for (auto const& ge : _gridElements) {
      float t = ge.second.bounds.intersectionOf(rayOrigin, rayDirN);
      if (t < 0.0) {
         continue;
      }
      cb(ge.second.nodes[RENDER_NODES]);
      cb(ge.second.nodes[LIGHT_NODES]);
   }
   cb(_spilloverNodes[RENDER_NODES]);
   cb(_spilloverNodes[LIGHT_NODES]);
}


void GridSpatialGraph::_queryGrid(std::vector<GridItem> const& nodes, Frustum const& frust, std::vector<QueryResult>& results)
{
   for (GridItem const& g: nodes) {
      BoundingBox const& bounds = g.bounds;

      if (frust.cullBox(bounds)) {
         continue;
      }

      results.emplace_back(QueryResult(bounds, g.node, g.renderQueues));
   }
}

void GridSpatialGraph::_queryGridLight(std::vector<GridItem> const& nodes, Frustum const& frust, std::vector<QueryResult>& results)
{
   for (GridItem const& g: nodes) {
      LightNode *n = (LightNode*)g.node;

      if (!n->getParamI(LightNodeParams::DirectionalI)) {
         if (frust.cullSphere(n->getAbsPos(), n->getRadius())) {
            continue;
         }
      }
      results.emplace_back(QueryResult(g.bounds, g.node, g.renderQueues));
   }
}

void GridSpatialGraph::query(Frustum const& frust, std::vector<QueryResult>& results, QueryTypes::List queryTypes)
{
   Modules::sceneMan().sceneForId(_sceneId).updateNodes();

   for (auto const& ge : _gridElements) {

      if (frust.cullBox(ge.second.bounds)) {
         continue;
      }

      if (queryTypes & QueryTypes::CullableRenderables) {
         _queryGrid(ge.second.nodes[RENDER_NODES], frust, results);
      }

      if (queryTypes & QueryTypes::Lights) {
         _queryGridLight(ge.second.nodes[LIGHT_NODES], frust, results);
      }
   }


   if (queryTypes & QueryTypes::CullableRenderables) {
      _queryGrid(_spilloverNodes[RENDER_NODES], frust, results);
   }

   if (queryTypes & QueryTypes::UncullableRenderables) {
      for (auto const& n : _noCullNodes) {
         results.emplace_back(QueryResult(n.bounds, n.node, n.renderQueues));
      }
   }

   if (queryTypes & QueryTypes::Lights) {
      _queryGridLight(_spilloverNodes[LIGHT_NODES], frust, results);
   }

   if (queryTypes & QueryTypes::Lights) {
      for (auto& d : _directionalLights) {
         results.emplace_back(QueryResult(d.second->_bBox, (LightNode*)d.second));
      }
   }   
}


// *************************************************************************************************
// Class SceneManager
// *************************************************************************************************


SceneManager::SceneManager() {
   reset();
}

SceneManager::~SceneManager() {

}

void SceneManager::reset()
{
   for (auto& s : _scenes) {
      s->shutdown();
   }

   _scenes.clear();

   addScene("default");
}

void SceneManager::clear()
{
   for (auto& s : _scenes) {
      s->removeNode(s->getRootNode());
   }
}

std::vector<Scene*> const& SceneManager::getScenes() const
{
   return _scenes;
}


Scene& SceneManager::sceneForNode(NodeHandle handle)
{
   SceneId sceneId = sceneIdFor(handle);
   ASSERT(sceneId < _scenes.size());
   return *_scenes[sceneId];
}

Scene& SceneManager::sceneForId(SceneId sceneId)
{
   ASSERT(sceneId < _scenes.size());
   return *_scenes[sceneId];
}

SceneId SceneManager::sceneIdFor(NodeHandle handle) const
{
   return (handle & 0xFC000000) >> 26;
}

void SceneManager::registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
	                  NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeInstanceRenderFunc irf )
{
	NodeRegEntry entry;
	entry.typeString = typeString;
	entry.parsingFunc = pf;
	entry.factoryFunc = ff;
	entry.renderFunc = rf;
   entry.instanceRenderFunc = irf;
	_registry[type] = entry;

   for (auto& scene : _scenes) {
      scene->registerType(type, typeString, pf, ff, rf, irf);
   }
}

SceneId SceneManager::addScene(const char* name)
{
   SceneId newId = (SceneId)_scenes.size();
   Scene* sc = new Scene(newId);

   for (auto const& t : _registry) {
      sc->registerType(t.first, t.second.typeString, t.second.parsingFunc, t.second.factoryFunc, t.second.renderFunc, t.second.instanceRenderFunc);
   }

   _scenes.push_back(sc);

   return newId;
}

SceneNode* SceneManager::resolveNodeHandle(NodeHandle handle){
   return sceneForNode(handle).resolveNodeHandle(handle);
}

int SceneManager::checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion )
{
   return sceneForNode(node.getHandle()).checkNodeVisibility(node, cam, checkOcclusion);
}

NodeHandle SceneManager::addNode( SceneNode *node, SceneNode &parent )
{
   return sceneForNode(parent.getHandle()).addNode(node, parent);
}

NodeHandle SceneManager::addNodes( SceneNode &parent, SceneGraphResource &sgRes )
{
   return sceneForNode(parent.getHandle()).addNodes(parent, sgRes);
}

void SceneManager::removeNode( SceneNode &node )
{
   sceneForNode(node.getHandle()).removeNode(node);
}

bool SceneManager::relocateNode( SceneNode &node, SceneNode &parent )
{
   return sceneForNode(node.getHandle()).relocateNode(node, parent);
}

void SceneManager::clearQueryCaches()
{
   for (auto& s : _scenes) {
      s->clearQueryCache();
   }
}

bool SceneManager::sceneExists(SceneId sceneId) const
{
   return sceneId < _scenes.size();
}


// *************************************************************************************************
// Class Scene
// *************************************************************************************************

Scene::Scene(SceneId sceneId)
{
   _sceneId = sceneId;
   initialize();
}


Scene::~Scene()
{
   shutdown();
}

void Scene::reset()
{
   shutdown();
   initialize();
}

SceneId Scene::getId() const
{
   return _sceneId;
}

void Scene::shutdown()
{
   delete _spatialGraph; _spatialGraph = nullptr;

   for (auto &entry : _nodes) {
      delete entry.second;
   }
   _nodes.clear();

   for (auto &entry : _registry) {
      entry.second.nodes.clear();
   }

   _nodeTrackers.clear();
}

NodeHandle Scene::nextNodeId()
{
   return (_sceneId << 26) | _nextHandleValue++;
}

void Scene::initialize()
{
   SceneNode *rootNode = GroupNode::factoryFunc( GroupNodeTpl( "RootNode" ) );
	rootNode->_handle = nextNodeId();
   _rootNodeId = rootNode->_handle;
   _nodes[_rootNodeId] = rootNode;

   _spatialGraph = new GridSpatialGraph(_sceneId);  

   _queryCacheCount = 0;
   _currentQuery = -1;
   for (int i = 0; i < QueryCacheSize; i++) {
      _queryCache[i].result.reserve(2000);
   }
}

void Scene::registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
								 NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeInstanceRenderFunc irf )
{
	NodeRegEntry entry;
	entry.typeString = typeString;
	entry.parsingFunc = pf;
	entry.factoryFunc = ff;
	entry.renderFunc = rf;
   entry.instanceRenderFunc = irf;
	_registry[type] = entry;
}


NodeRegEntry *Scene::findType( int type )
{
	map< int, NodeRegEntry >::iterator itr = _registry.find( type );
	
	if( itr != _registry.end() ) return &itr->second;
	else return 0x0;
}


NodeRegEntry *Scene::findType( std::string const& typeString )
{
	map< int, NodeRegEntry >::iterator itr = _registry.begin();

	while( itr != _registry.end() )
	{
		if( itr->second.typeString == typeString ) return &itr->second;

		++itr;
	}
	
	return 0x0;
}


void Scene::setNodeHidden(SceneNode& node, bool hidden) {
   if (hidden) {
      if (node._gridPos >= 0) {
         _spatialGraph->removeNode(node);
      }
   } else {
      _spatialGraph->addNode(node);
   }
}


void Scene::updateNodes()
{
   radiant::perfmon::TimelineCounterGuard un("updateNodes");
	getRootNode().update();
}


void Scene::updateSpatialNode(SceneNode& node) 
{
   _spatialGraph->updateNode(node);
   updateNodeTrackers(&node);
}

void Scene::updateNodeInstanceKey(SceneNode& node)
{
   _spatialGraph->updateNodeInstanceKey(node);
}


void Scene::_queryNode(SceneNode& node, std::vector<QueryResult>& results)
{
   if (node._renderable) {
      GridItem* gi = _spatialGraph->gridItemForNode(node);
      results.emplace_back(QueryResult(node.getBBox(), &node, gi->renderQueues));
   }

   for (SceneNode* child : node.getChildren()) {
      _queryNode(*child, results);
   }
}

std::vector<QueryResult> const& Scene::queryNode(SceneNode& node)
{
   for (int i = 0; i < _queryCacheCount; i++)
   {
      CachedQueryResult& r = _queryCache[i];

      if (r.singleNode == &node) {
         return r.result;
      }
   }

   if (_queryCacheCount < QueryCacheSize) {
      _queryCacheCount++;
   } else {
      ASSERT(false);
   }
   _currentQuery = _queryCacheCount - 1;
   CachedQueryResult& r = _queryCache[_currentQuery];

   r.singleNode = &node;
   _queryNode(node, r.result);
   return r.result;
}


std::vector<QueryResult> const& Scene::queryScene(Frustum const& frust, QueryTypes::List queryTypes) 
{
   radiant::perfmon::TimelineCounterGuard uq("queryScene");

   for (int i = 0; i < _queryCacheCount; i++)
   {
      CachedQueryResult& r = _queryCache[i];

      if (r.frust == frust && r.queryTypes == queryTypes && r.singleNode == nullptr) {
         return r.result;
      }
   }

   if (_queryCacheCount < QueryCacheSize) {
      _queryCacheCount++;
   } else {
      ASSERT(false);
   }
   _currentQuery = _queryCacheCount - 1;
   CachedQueryResult& r = _queryCache[_currentQuery];

   r.frust = frust;
   r.queryTypes = queryTypes;
   r.singleNode = nullptr;
   _spatialGraph->query(frust, r.result, queryTypes);
   return r.result;
}


std::vector<QueryResult> const& Scene::subQuery(std::vector<QueryResult> const& queryResults, SceneNodeFlags::List ignoreFlags)
{
   radiant::perfmon::TimelineCounterGuard uq("subQuery");

   // Just store the sub query in the last place of the cache, since that is effectively a 'scratch'
   // location anyway.
   CachedQueryResult& r = _queryCache[QueryCacheSize - 1];

   r.result.clear();

   for (auto& qr : queryResults) {
      if (!(qr.node->_accumulatedFlags & ignoreFlags)) {
         r.result.emplace_back(qr);
      }
   }

   return r.result;
}


NodeHandle Scene::parseNode( SceneNodeTpl &tpl, SceneNode *parent )
{
	if( parent == 0x0 ) return 0;
	
	SceneNode *sn = 0x0;

	if( tpl.type == 0 )
	{
		// Reference node
		NodeHandle handle = parseNode( *((ReferenceNodeTpl *)&tpl)->sgRes->getRootNode(), parent );
		sn = resolveNodeHandle( handle );
		if( sn != 0x0 )
		{	
			sn->_name = tpl.name;
			sn-> setTransform( tpl.trans, tpl.rot, tpl.scale );
			sn->_attachment = tpl.attachmentString;
		}
	}
	else
	{
		map< int, NodeRegEntry >::iterator itr = _registry.find( tpl.type );
		if( itr != _registry.end() ) sn = (*itr->second.factoryFunc)( tpl );
		if( sn != 0x0 ) addNode( sn, *parent );
	}

	if( sn == 0x0 ) return 0;

	// Parse children
	for( uint32 i = 0; i < tpl.children.size(); ++i )
	{
		parseNode( *tpl.children[i], sn );
	}

	return sn->getHandle();
}


NodeHandle Scene::addNode( SceneNode *node, SceneNode &parent )
{
	if( node == 0x0 ) return 0;
	
	// Check if node can be attached to parent
	if( !node->canAttach( parent ) )
	{
		Modules::log().writeDebugInfo( "Can't attach node '%s' to parent '%s'", node->_name.c_str(), parent._name.c_str() );
		delete node; node = 0x0;
		return 0;
	}
	
	node->_parent = &parent;
	node->_handle = nextNodeId();
	_nodes[node->_handle] = node;

   _registry[node->_type].nodes[node->getHandle()] = node;

   // Attach to parent
   SCENE_LOG(9) << "adding child (handle:" << node->_handle << " name:" << node->_name << ") to node (handle:" << parent._handle << " name:" << parent._name << ")";
	parent._children.push_back( node );

	// Raise event
	node->onAttach( parent );

	// Mark tree as dirty
   node->markDirty(SceneNodeDirtyKind::All);

	// Register node in spatial graph
   if ((node->_accumulatedFlags & SceneNodeFlags::NoDraw) == 0) {
      _spatialGraph->addNode(*node);
   }
   return node->_handle;
}


NodeHandle Scene::addNodes( SceneNode &parent, SceneGraphResource &sgRes )
{
	// Parse root node
	return parseNode( *sgRes.getRootNode(), &parent );
}


void Scene::removeNodeRec( SceneNode &node )
{
	NodeHandle handle = node._handle;
	
	// Raise event
   if (handle != _rootNodeId) 
   {
      node.onDetach(*node._parent);
   }

   _registry[node.getType()].nodes.erase(node.getHandle());

	// Remove children
   while (node._children.size() > 0) 
   {
      removeNodeRec(*node._children.back());
      node._children.erase(node._children.end() - 1);
	}
	
   updateNodeTrackers(&node);

   // Delete node
   if( handle != _rootNodeId )
	{
		_spatialGraph->removeNode(node);
      auto i = _nodes.find(node._handle), end = _nodes.end();
      ASSERT(i != _nodes.end());
      if (i != _nodes.end()) {
         delete i->second;
         _nodes.erase(i);
      }
	}
}


void Scene::removeNode( SceneNode &node )
{
	SceneNode *parent = node._parent;
	SceneNode *nodeAddr = &node;
	
	removeNodeRec( node );  // node gets deleted if it is not the rootnode
	
	// Remove node from parent
	if( parent != 0x0 )
	{
		// Find child
		for( uint32 i = 0; i < parent->_children.size(); ++i )
		{
			if( parent->_children[i] == nodeAddr )
			{
            SCENE_LOG(9) << "removing child (handle:" << node._handle << " name:" << node._name << ") from node (handle:" << parent->_handle << " name:" << parent->_name << ")";
				parent->_children.erase( parent->_children.begin() + i );
				break;
			}
		}
      parent->markDirty(SceneNodeDirtyKind::Ancestors);
	}
	else  // Rootnode
	{
		node._children.clear();
      node.markDirty(SceneNodeDirtyKind::All);
	}
}


bool Scene::relocateNode( SceneNode &node, SceneNode &parent )
{
   if( node._handle == _rootNodeId ) return false;
	
	if( !node.canAttach( parent ) )
	{	
		Modules::log().writeDebugInfo( "Can't attach node to parent in h3dSetNodeParent" );
		return false;
	}
	
   SceneNode* oldParent = node._parent;
	// Detach from old parent
	node.onDetach( *node._parent );
	for( uint32 i = 0; i < node._parent->_children.size(); ++i )
	{
		if( node._parent->_children[i] == &node )
		{
         SCENE_LOG(9) << "removing child (handle:" << node._handle << " name:" << node._name << ") from node (handle:" << node._parent->_handle << " name:" << node._parent->_name << ") relocate";
			node._parent->_children.erase( node._parent->_children.begin() + i );
			break;
		}
	}

	// Attach to new parent
   SCENE_LOG(9) << "adding child (handle:" << node._handle << " name:" << node._name << ") from node (handle:" << parent._handle << " name:" << parent._name << ") relocate";
   parent._children.push_back( &node );
	node._parent = &parent;
	node.onAttach( parent );
	
	parent.markDirty(SceneNodeDirtyKind::All);
   oldParent->markDirty(SceneNodeDirtyKind::Ancestors);
	
	return true;
}


void Scene::registerNodeTracker(SceneNode const* tracker, std::function<void(SceneNode const* updatedNode)> nodeChangedCb)
{
   _nodeTrackers[tracker] = nodeChangedCb;
}


void Scene::clearNodeTracker(SceneNode const* tracker)
{
   auto &i = _nodeTrackers.find(tracker);

   if (i != _nodeTrackers.end()) {
      _nodeTrackers.erase(i);
   }
}


void Scene::updateNodeTrackers(SceneNode const* node)
{
   for (auto const& i : _nodeTrackers) {
      if (i.first->getBBox().intersects(node->getBBox())) {
         i.second(node);
      }
   }
}


int Scene::findNodes( SceneNode &startNode, std::string const& name, int type )
{
   _findResults.clear();


   if (startNode.getHandle() == _rootNodeId) {
      // Since we want all nodes of that type, just consult the registry.
      for (auto& n : _registry[type].nodes) {
         if (name == "" || n.second->_name == name) {
            _findResults.push_back(n.second);
         }
      }
   } else {
      _findNodes(startNode, name, type);
   }

	return (int)_findResults.size();
}


void Scene::_findNodes( SceneNode &startNode, std::string const& name, int type )
{
	
	if( type == SceneNodeTypes::Undefined || startNode._type == type )
	{
		if( name == "" || startNode._name == name )
		{
			_findResults.push_back( &startNode );
		}
	}

	for( uint32 i = 0; i < startNode._children.size(); ++i )
	{
		_findNodes( *startNode._children[i], name, type );
	}
}

void Scene::fastCastRayInternal(int userFlags)
{
   const Vec3f rayEnd = _rayOrigin + _rayDirection;
   _spatialGraph->castRay(_rayOrigin, _rayDirection, [this, userFlags, rayEnd](std::vector<GridItem> const& items) {
      for (GridItem const& gi : items) {
         if (gi.node->_accumulatedFlags & SceneNodeFlags::NoRayQuery)
	      {
            continue;
         }
         if ((gi.node->_userFlags & userFlags) != userFlags) {
            continue;
         }

         Vec3f intsPos, intsNorm;
         if (!gi.node->checkIntersection(_rayOrigin, rayEnd, _rayDirection, intsPos, intsNorm))
		   {
            continue;
         }

			float dist = (intsPos - _rayOrigin).length();

			CastRayResult crr;
			crr.node = gi.node;
			crr.distance = dist;
			crr.intersection = intsPos;
         crr.normal = intsNorm;

			bool inserted = false;
			for( vector< CastRayResult >::iterator it = _castRayResults.begin(); it != _castRayResults.end(); ++it )
			{
				if( dist < it->distance )
				{
					_castRayResults.insert( it, crr );
					inserted = true;
					break;
				}
			}

         if ((int)_castRayResults.size() > _rayNum) {
            _castRayResults.pop_back();
         } else if( !inserted && (int)_castRayResults.size() < _rayNum) {
				_castRayResults.push_back( crr );
			}
      }
   });
}


void Scene::castRayInternal( SceneNode &node, int userFlags )
{
   if( !(node._accumulatedFlags & SceneNodeFlags::NoRayQuery) )
	{
		Vec3f intsPos, intsNorm;
      if((node._userFlags & userFlags) == userFlags && node.checkIntersection( _rayOrigin, _rayOrigin + _rayDirection, _rayDirection, intsPos, intsNorm ) )
		{
			float dist = (intsPos - _rayOrigin).length();

			CastRayResult crr;
			crr.node = &node;
			crr.distance = dist;
			crr.intersection = intsPos;
         crr.normal = intsNorm;

			bool inserted = false;
			for( vector< CastRayResult >::iterator it = _castRayResults.begin(); it != _castRayResults.end(); ++it )
			{
				if( dist < it->distance )
				{
					_castRayResults.insert( it, crr );
					inserted = true;
					break;
				}
			}

			if( !inserted )
			{
				_castRayResults.push_back( crr );
			}

			if( _rayNum > 0 && (int)_castRayResults.size() > _rayNum )
			{
				_castRayResults.pop_back();
			}
		}

		for( size_t i = 0, s = node._children.size(); i < s; ++i )
		{
			castRayInternal( *node._children[i], userFlags );
		}
	}
}


int Scene::castRay( SceneNode &node, const Vec3f &rayOrig, const Vec3f &rayDir, int numNearest, int userFlags )
{
	_castRayResults.resize( 0 );  // Clear without affecting capacity

   if( node._accumulatedFlags & SceneNodeFlags::NoRayQuery ) return 0;

	_rayOrigin = rayOrig;
	_rayDirection = rayDir;
	_rayNum = numNearest;

   if (node.getHandle() == _rootNodeId) {
      fastCastRayInternal(userFlags);
   } else {
   	castRayInternal( node, userFlags );
   }
	return (int)_castRayResults.size();
}


bool Scene::getCastRayResult( int index, CastRayResult &crr )
{
	if( (uint32)index < _castRayResults.size() )
	{
		crr = _castRayResults[index];

		return true;
	}

	return false;
}


int Scene::checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion )
{
	// Note: This function is a bit hacky with all the hard-coded node types
	
   if( node._dirty != SceneNodeDirtyState::Clean ) updateNodes();
	
	// Frustum culling
	if( cam.getFrustum().cullBox( node.getBBox() ) )
		return -1;
	else
		return 0;
}


void Scene::clearQueryCache()
{
   SCENE_LOG(5) << "clearing query cache";

   for (int i = 0; i < _queryCacheCount; i++) {
      _queryCache[i].result.resize(0);
   }
   _queryCacheCount = 0;
   _currentQuery = -1;
}

SceneNode *Scene::resolveNodeHandle(NodeHandle handle)
{
   auto i = _nodes.find(handle), end = _nodes.end();
   return i != end ? i->second : nullptr;
}

}  // namespace
