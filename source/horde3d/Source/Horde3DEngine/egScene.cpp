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
   _accumulatedFlags = _flags | parentFlags;
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
      updateNodeInstanceKey(sceneNode);
   }
}


void GridSpatialGraph::updateNodeInstanceKey(SceneNode& sceneNode)
{
   GridItem *gi;
   if (sceneNode._gridId == -1) {
      gi = &_spilloverNodes[RENDER_NODES].at(sceneNode._gridPos);
   } else {
      gi = &_gridElements.at(sceneNode._gridId).nodes[RENDER_NODES].at(sceneNode._gridPos);
   }

   for (int i = 0; i < 4; i++) {
      if (sceneNode.getInstanceKey() != 0x0) {
         InstanceRenderableQueues &iqs = Modules::renderer()._instanceQueues[i];
         auto iqsIt = iqs.find(sceneNode._type);
         if (iqsIt == iqs.end()) {
            InstanceRenderableQueue newQ;
            iqsIt = iqs.emplace_hint(iqsIt, newQ);
         }
         InstanceRenderableQueue& iq = iqsIt->second;

         const InstanceKey& ik = *(sceneNode.getInstanceKey());
         auto iqIt = iq.find(ik);
         if (iqIt == iq.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            iqIt = iq.emplace_hint(iqIt, ik, newQ);
         }
         gi->renderQueues[i] = (iqIt->second);
      } else {
         RenderableQueues &rqs = Modules::renderer()._singularQueues[i];
         auto rqIt = rqs.find(sceneNode._type);
         if (rqIt == rqs.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            rqIt = rqs.emplace_hint(rqIt, newQ);
         }
         gi->renderQueues[i] = (rqIt->second);
      }
   }
}


void GridSpatialGraph::removeNode(SceneNode& sceneNode)
{
   radiant::perfmon::TimelineCounterGuard un("gsg:removeNode");
   NodeHandle h = sceneNode.getHandle();
   if (sceneNode.getType() == SceneNodeTypes::Light && ((LightNode*)&sceneNode)->getParamI(LightNodeParams::DirectionalI)) {
      _directionalLights.erase(h);
   } else {
      const int nodeType = sceneNode._renderable ? RENDER_NODES : LIGHT_NODES;
      std::vector<GridItem>*vec;
      if (sceneNode._gridId == -1) {
         vec = &_spilloverNodes[nodeType];
      } else {
         vec = &_gridElements[sceneNode._gridId].nodes[nodeType];
      }

      if (vec->size() > 1) {
         (*vec)[sceneNode._gridPos] = vec->back();
      }

      if (vec->size() > 0) {
         vec->back().node->_gridPos = sceneNode._gridPos;
         vec->pop_back();
      }

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

   if(!sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light) {
      return;
   }
   
   BoundingBox const& sceneBox = sceneNode.getBBox();
   if (!sceneBox.isValid()) {
      return;
   }

   const int nodeType = sceneNode._renderable ? RENDER_NODES : LIGHT_NODES;
   
   if (sceneNode.getFlags() & SceneNodeFlags::NoCull) {
      ASSERT(sceneNode._gridId == -1);
      _nocullNodes[sceneNode.getHandle()] = &sceneNode;
   } else {
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
      if (oldVec->size() > 1 && sceneNode._gridPos >= 0) {
         (*oldVec)[sceneNode._gridPos] = oldVec->back();
      }

      if (sceneNode._gridPos >= 0) {
         oldVec->back().node->_gridPos = sceneNode._gridPos;
         oldVec->pop_back();
      }
      newVec->emplace_back(GridItem(sceneBox, &sceneNode));
      sceneNode._gridId = gridId;
      sceneNode._gridPos = (int)(newVec->size() - 1);
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

/*void GridSpatialGraph::_queryLights(std::vector<GridItem> const& nodes, SpatialQuery const& query, std::vector<SceneNode const*>& lightQueue)
{
   const int qFilterIgnore = query.filterIgnore;
   const int qFilterRequired = query.filterRequired;
   const int qUserFlags = query.userFlags;
   const int qForceNoInstancing = query.forceNoInstancing;
   Frustum const& qFrustum = query.frustum;
   const Frustum* const qSecondaryFrustum = query.secondaryFrustum;

   for (GridItem const& g: nodes) {
      SceneNode const* node = g.node;
      if (node->_accumulatedFlags & qFilterIgnore) {
         continue;
      }
      if ((node->_accumulatedFlags & qFilterRequired) != qFilterRequired) {
         continue;
      }
      if ((node->_userFlags & qUserFlags) != qUserFlags) {
         continue;
      }

      LightNode *n = (LightNode*) node;

      // Only lights that are NOT directional can be culled.
      if (!n->getParamI(LightNodeParams::DirectionalI)) {
         if (qFrustum.cullSphere(n->getAbsPos(), n->getRadius()) || (qSecondaryFrustum != 0x0 && qSecondaryFrustum->cullSphere(n->getAbsPos(), n->getRadius()))) {
            continue;
         }
      }
      lightQueue.push_back(node);
   }
}


void GridSpatialGraph::_queryRenderables(std::vector<GridItem> const& nodes, SpatialQuery const& query, RenderableQueues& renderableQueues, 
                         InstanceRenderableQueues& instanceQueues)
{
   const int qFilterIgnore = query.filterIgnore;
   const int qFilterRequired = query.filterRequired;
   const int qUserFlags = query.userFlags;
   const int qOrder = query.order;
   const int qForceNoInstancing = query.forceNoInstancing;
   Frustum const& qFrustum = query.frustum;
   const Frustum* const qSecondaryFrustum = query.secondaryFrustum;

   for (GridItem const& g: nodes) {
      SceneNode const* node = g.node;
      if (node->_accumulatedFlags & qFilterIgnore) {
         continue;
      }
      if ((node->_accumulatedFlags & qFilterRequired) != qFilterRequired) {
         continue;
      }
      if ((node->_userFlags & qUserFlags) != qUserFlags) {
         continue;
      }

      BoundingBox const& bounds = g.bounds;

      // If the bounds are culled by either frustum, then ignore.  (Don't worry about shadows; as long as their frustums
      // are built correctly, this works fine).
      if (qFrustum.cullBox(bounds) || (qSecondaryFrustum != 0x0 && qSecondaryFrustum->cullBox(bounds))) {
         continue;
      }

      float sortKey = 0;

      switch( qOrder )
      {
      case RenderingOrder::StateChanges:
         sortKey = node->_sortKey;
         break;
      case RenderingOrder::FrontToBack:
         sortKey = nearestDistToAABB( qFrustum.getOrigin(), bounds.min(), bounds.max() );
         break;
      case RenderingOrder::BackToFront:
         sortKey = -nearestDistToAABB( qFrustum.getOrigin(), bounds.min(), bounds.max() );
         break;
      }

      if (node->getInstanceKey() != 0x0 && !qForceNoInstancing) {
         auto iqsIt = instanceQueues.find(node->_type);
         if (iqsIt == instanceQueues.end()) {
            InstanceRenderableQueue newQ;
            iqsIt = instanceQueues.emplace_hint(iqsIt, node->_type, newQ);
         }
         InstanceRenderableQueue& iq = iqsIt->second;

         const InstanceKey& ik = *(node->getInstanceKey());
         auto iqIt = iq.find(ik);
         if (iqIt == iq.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            iqIt = iq.emplace_hint(iqIt, ik, newQ);
         }
         iqIt->second.emplace_back(RendQueueItem(node->_type, sortKey, node));
      } else {
         auto rqIt = renderableQueues.find(node->_type);
         if (rqIt == renderableQueues.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            rqIt = renderableQueues.emplace_hint(rqIt, node->_type, newQ);
         }
         rqIt->second.emplace_back( RendQueueItem( node->_type, sortKey, node ) );
      }
   }
}


void GridSpatialGraph::query(SpatialQuery const& query, RenderableQueues& renderableQueues, 
                         InstanceRenderableQueues& instanceQueues, std::vector<SceneNode const*>& lightQueue)
{
   const Frustum& qFrustum = query.frustum;
   const Frustum* const qSecondaryFrustum = query.secondaryFrustum;

   ASSERT(query.useLightQueue || query.useRenderableQueue);

   Modules::sceneMan().sceneForId(_sceneId).updateNodes();

   for (auto const& ge : _gridElements) {

      // If the bounds are culled by either frustum, then ignore.  (Don't worry about shadows; as long as their frustums
      // are built correctly, this works fine).
      if (qFrustum.cullBox(ge.second.bounds) || (qSecondaryFrustum != 0x0 && qSecondaryFrustum->cullBox(ge.second.bounds))) {
         continue;
      }

      if (query.useLightQueue) {
         _queryLights(ge.second.nodes[LIGHT_NODES], query, lightQueue);
      }
      if (query.useRenderableQueue) {
         _queryRenderables(ge.second.nodes[RENDER_NODES], query, renderableQueues, instanceQueues);
      }
   }

   if (query.useLightQueue) {
      _queryLights(_spilloverNodes[LIGHT_NODES], query, lightQueue);
   }
   if (query.useRenderableQueue) {
      _queryRenderables(_spilloverNodes[RENDER_NODES], query, renderableQueues, instanceQueues);
   }

   if (query.useRenderableQueue) {
      if (query.filterRequired & SceneNodeFlags::NoCull || ((query.filterIgnore & SceneNodeFlags::NoCull) == 0)) {
         for (auto const& n : _nocullNodes) {
            if (n.second->_accumulatedFlags & query.filterIgnore) {
               continue;
            }
            if ((n.second->_accumulatedFlags & query.filterRequired) != query.filterRequired) {
               continue;
            }
            if ((n.second->_userFlags & query.userFlags) != query.userFlags) {
               continue;
            }
            if (renderableQueues.find(n.second->_type) == renderableQueues.end()) {
               renderableQueues[n.second->_type] = RenderableQueue();
               renderableQueues[n.second->_type].reserve(1000);
            }
            renderableQueues[n.second->_type].emplace_back( RendQueueItem( n.second->_type, 0, n.second ) );
         }
      }
   }

   if (query.useLightQueue) {
      for (auto const& d : _directionalLights) {
         SceneNode const* n = d.second;

         if (n->_accumulatedFlags & query.filterIgnore) {
            continue;
         }
         if ((n->_accumulatedFlags & query.filterRequired) != query.filterRequired) {
            continue;
         }
         if ((n->_userFlags & query.userFlags) != query.userFlags) {
            continue;
         }
         lightQueue.push_back(d.second);
      }
   }      

   // Sort
   if( query.order != RenderingOrder::None ) {
      for (auto& item : renderableQueues) {
         std::sort( item.second.begin(), item.second.end(), RendQueueItemCompFunc() );
      }

      for (auto& item : instanceQueues) {
         for (auto& queue : item.second) {
            std::sort(queue.second.begin(), queue.second.end(), RendQueueItemCompFunc());
         }
      }
   }
}*/


void GridSpatialGraph::_queryGrid(std::vector<GridItem> const& nodes, Frustum const& frust, std::vector<QueryResult>& results)
{
   for (GridItem const& g: nodes) {
      BoundingBox const& bounds = g.bounds;

      if (frust.cullBox(bounds)) {
         continue;
      }

      results.emplace_back(QueryResult(bounds, g.node));
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
      results.emplace_back(QueryResult(g.bounds, g.node));
   }
}

void GridSpatialGraph::query2(Frustum const& frust, std::vector<QueryResult>& results, QueryTypes::List queryTypes)
{
   Modules::sceneMan().sceneForId(_sceneId).updateNodes();

   for (auto const& ge : _gridElements) {

      if (frust.cullBox(ge.second.bounds)) {
         continue;
      }

      if (queryTypes & QueryTypes::Renderables) {
         _queryGrid(ge.second.nodes[RENDER_NODES], frust, results);
      }

      if (queryTypes & QueryTypes::Lights) {
         _queryGridLight(ge.second.nodes[LIGHT_NODES], frust, results);
      }
   }


   if (queryTypes & QueryTypes::Renderables) {
      _queryGrid(_spilloverNodes[RENDER_NODES], frust, results);
   }

   if (queryTypes & QueryTypes::Lights) {
      _queryGridLight(_spilloverNodes[LIGHT_NODES], frust, results);
   }


   /*if (query.useRenderableQueue) {
      if (query.filterRequired & SceneNodeFlags::NoCull || ((query.filterIgnore & SceneNodeFlags::NoCull) == 0)) {
         for (auto const& n : _nocullNodes) {
            if (n.second->_accumulatedFlags & query.filterIgnore) {
               continue;
            }
            if ((n.second->_accumulatedFlags & query.filterRequired) != query.filterRequired) {
               continue;
            }
            if ((n.second->_userFlags & query.userFlags) != query.userFlags) {
               continue;
            }
            if (renderableQueues.find(n.second->_type) == renderableQueues.end()) {
               renderableQueues[n.second->_type] = RenderableQueue();
               renderableQueues[n.second->_type].reserve(1000);
            }
            renderableQueues[n.second->_type].emplace_back( RendQueueItem( n.second->_type, 0, n.second ) );
         }
      }
   }*/

   if (queryTypes & QueryTypes::Lights) {
      for (auto& d : _directionalLights) {
         results.emplace_back(QueryResult(d.second->_bBox, d.second));
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
	                  NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeRenderFunc irf )
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
   //_queryCacheCount = 0;
   //_currentQuery = -1;

   _queryCacheCount2 = 0;
   _currentQuery2 = -1;
   for (int i = 0; i < QueryCacheSize; i++) {
      _queryCache2[i].result.reserve(2000);
   }

   /*for (int i = 0; i < QueryCacheSize; i++) {
      _queryCache[i].lightQueue.reserve(20);
   }*/
}

void Scene::registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
								 NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeRenderFunc irf )
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


void Scene::_updateQueuesWithNode(SceneNode& node, RenderableQueues& renderableQueues, InstanceRenderableQueues& instanceQueues)
{
   if (node._renderable) {
      if (node.getInstanceKey() != 0x0) {
         auto iqsIt = instanceQueues.find(node._type);
         if (iqsIt == instanceQueues.end()) {
            InstanceRenderableQueue newQ;
            iqsIt = instanceQueues.emplace_hint(iqsIt, node._type, newQ);
         }
         InstanceRenderableQueue& iq = iqsIt->second;

         const InstanceKey& ik = *(node.getInstanceKey());
         auto iqIt = iq.find(ik);
         if (iqIt == iq.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            iqIt = iq.emplace_hint(iqIt, ik, newQ);
         }
         iqIt->second.emplace_back(RendQueueItem(node._type, node._sortKey, &node));
      } else {
         auto rqIt = renderableQueues.find(node._type);
         if (rqIt == renderableQueues.end()) {
            RenderableQueue newQ;
            newQ.reserve(1000);
            rqIt = renderableQueues.emplace_hint(rqIt, node._type, newQ);
         }
         rqIt->second.emplace_back( RendQueueItem( node._type, node._sortKey, &node) );
      }
   }

   for (SceneNode* child : node.getChildren()) {
      _updateQueuesWithNode(*child, renderableQueues, instanceQueues);
   }
}


void Scene::updateQueuesWithNode(SceneNode& node, Frustum const& frust)
{
   if (_queryCacheCount < QueryCacheSize) {
      _queryCacheCount++;
   }
   _currentQuery = _queryCacheCount - 1;
   SpatialQueryResult& sqr = _queryCache[_currentQuery];
 
   sqr.query.filterIgnore = 0;
   sqr.query.filterRequired = 0;
   sqr.query.frustum = frust;
   sqr.query.order = Horde3D::RenderingOrder::StateChanges;
   sqr.query.secondaryFrustum = 0;
   sqr.query.useLightQueue = false;
   sqr.query.useRenderableQueue = true;
   sqr.query.forceNoInstancing = false;
   sqr.query.userFlags = 0;
 
   for (auto& item : sqr.renderableQueues) {
      item.second.resize(0);
   }

   for (auto& item : sqr.instanceRenderableQueues) {
      for (auto& instances : item.second) {
         instances.second.resize(0);
      }
   }
   _updateQueuesWithNode(node, sqr.renderableQueues, sqr.instanceRenderableQueues);
}

std::vector<QueryResult> const& Scene::queryScene(Frustum const& frust, QueryTypes::List queryTypes) {
   radiant::perfmon::TimelineCounterGuard uq("queryScene");

   for (int i = 0; i < _queryCacheCount2; i++)
   {
      CachedQueryResult& r = _queryCache2[i];

      if (r.frust == frust && r.queryTypes == queryTypes) {
         return r.result;
      }
   }

   if (_queryCacheCount2 < QueryCacheSize) {
      _queryCacheCount2++;
   } else {
      ASSERT(false);
   }
   _currentQuery2 = _queryCacheCount2 - 1;
   CachedQueryResult& r = _queryCache2[_currentQuery2];

   r.frust = frust;
   r.queryTypes = queryTypes;
   _spatialGraph->query2(frust, r.result, queryTypes);
   return r.result;
}

/*void Scene::updateQueues( const char* reason, const Frustum &frustum1, const Frustum *frustum2, RenderingOrder::List order,
                                 uint32 filterIgnore, uint32 filterRequired, bool useLightQueue, bool useRenderableQueue, bool forceNoInstancing, uint32 userFlags )
{
   radiant::perfmon::TimelineCounterGuard uq("updateQueues");

   if (LOG_IS_ENABLED(horde.scene, 7)) {
      SCENE_LOG(7) << "updating queues for " << reason;
      SCENE_LOG(7) << "   ignore flags: " << FlagsToString(filterIgnore);
      SCENE_LOG(7) << "   required flags: " << FlagsToString(filterRequired);
      SCENE_LOG(7) << "   use light queue: " << std::boolalpha << useLightQueue;
      SCENE_LOG(7) << "   use renderable queue: " << std::boolalpha << useRenderableQueue;
      SCENE_LOG(7) << "   force no instancing: " << std::boolalpha << forceNoInstancing;
      SCENE_LOG(7) << "   user flags: " << std::boolalpha << userFlags;
   }
   SpatialQuery query;
   query.filterIgnore = filterIgnore;
   query.filterRequired = filterRequired;
   query.frustum = frustum1;
   query.order = order;
   query.secondaryFrustum = frustum2;
   query.useLightQueue = useLightQueue;
   query.useRenderableQueue = useRenderableQueue;
   query.forceNoInstancing = forceNoInstancing;
   query.userFlags = userFlags;

   _currentQuery = _checkQueryCache(query);
 
    if (_currentQuery != -1)
    {
       SCENE_LOG(5) << "query " << reason << " using cached query " << _currentQuery << " instead of updating queues!";
       // Cache hit!.
       return;
    }
    // Cache miss.
 
    // We effectively use the last cache spot as a scratch space, if we see more than QueryCacheSize 
    // distinct spatial queries.
    if (_queryCacheCount < QueryCacheSize) {
       _queryCacheCount++;
    }
    _currentQuery = _queryCacheCount - 1;
    SpatialQueryResult& sqr = _queryCache[_currentQuery];
 
    sqr.query.filterIgnore = filterIgnore;
    sqr.query.filterRequired = filterRequired;
    sqr.query.frustum = frustum1;
    sqr.query.order = order;
    sqr.query.secondaryFrustum = frustum2;
    sqr.query.useLightQueue = useLightQueue;
    sqr.query.useRenderableQueue = useRenderableQueue;
    sqr.query.forceNoInstancing = forceNoInstancing;
    sqr.query.userFlags = userFlags;
 
   // Clear without affecting capacity
    if (useLightQueue) 
    {
       sqr.lightQueue.resize(0);
    }
    if (useRenderableQueue )
    {
       for (auto& item : sqr.renderableQueues) {
          item.second.resize(0);
       }

       for (auto& item : sqr.instanceRenderableQueues) {
          for (auto& instances : item.second) {
             instances.second.resize(0);
          }
       }
    }
 
    SCENE_LOG(5) << "computing query cache slot " << _currentQuery;
    _spatialGraph->query(sqr.query, sqr.renderableQueues, sqr.instanceRenderableQueues, sqr.lightQueue);
}


std::vector<SceneNode const*>& Scene::getLightQueue() 
{ 
   ASSERT(_currentQuery != -1);
   return _queryCache[_currentQuery].lightQueue;
}


RenderableQueue& Scene::getRenderableQueue(int itemType) 
{ 
   ASSERT(_currentQuery != -1);
   RenderableQueues& renderQueues = _queryCache[_currentQuery].renderableQueues;
   if (renderQueues.find(itemType) == renderQueues.end()) {
      renderQueues[itemType] = RenderableQueue();
      renderQueues[itemType].reserve(1000);
   }
   return renderQueues[itemType];
}

RenderableQueues& Scene::getRenderableQueues()
{
   ASSERT(_currentQuery != -1);
   return _queryCache[_currentQuery].renderableQueues;
}


InstanceRenderableQueues& Scene::getInstanceRenderableQueues()
{
   ASSERT(_currentQuery != -1);
   InstanceRenderableQueues& renderQueues = _queryCache[_currentQuery].instanceRenderableQueues;
   return renderQueues;
}


InstanceRenderableQueue& Scene::getInstanceRenderableQueue(int itemType)
{
   ASSERT(_currentQuery != -1);
   InstanceRenderableQueues& renderQueues = _queryCache[_currentQuery].instanceRenderableQueues;
   if (renderQueues.find(itemType) == renderQueues.end()) {
      renderQueues[itemType] = InstanceRenderableQueue();
   }
   return renderQueues[itemType];
}*/

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
   _spatialGraph->addNode( *node );
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
   updateNodeTrackers(&node);
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

   for (int i = 0; i < _queryCacheCount2; i++) {
      _queryCache2[i].result.resize(0);
   }
   _queryCacheCount2 = 0;
   _currentQuery2 = -1;
}


/*int Scene::_checkQueryCache(const SpatialQuery& query)
{
   for (int i = 0; i < _queryCacheCount; i++)
   {
      SpatialQueryResult& r = _queryCache[i];

      if (r.query.forceNoInstancing != query.forceNoInstancing) 
      {
         continue;
      }
      if (r.query.filterIgnore != query.filterIgnore)
      {
         continue;
      }
      if (r.query.filterRequired != query.filterRequired)
      {
         continue;
      }
      if (r.query.userFlags != query.userFlags)
      {
         continue;
      }
      if (r.query.order != query.order)
      {
         continue;
      }
      if (r.query.useLightQueue != query.useLightQueue)
      {
         continue;
      }
      if (r.query.useRenderableQueue != query.useRenderableQueue)
      {
         continue;
      }
      if (query.secondaryFrustum != nullptr && r.query.secondaryFrustum != nullptr)
      {
         if (*query.secondaryFrustum != *r.query.secondaryFrustum)
         {
            continue;
         }
      } else if (query.secondaryFrustum != r.query.secondaryFrustum) {
         continue;
      }

      if (query.frustum != r.query.frustum)
      {
         continue;
      }

      // Cache hit!
      return i;
   }

   return -1;
}*/

SceneNode *Scene::resolveNodeHandle(NodeHandle handle)
{
   auto i = _nodes.find(handle), end = _nodes.end();
   return i != end ? i->second : nullptr;
}

}  // namespace
