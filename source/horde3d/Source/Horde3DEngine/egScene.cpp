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
   ADD_FLAG(Selected)
#undef ADD_FLAG
   s << ")";
   return s.str();
};

// *************************************************************************************************
// Class SceneNode
// *************************************************************************************************

SceneNode::SceneNode( const SceneNodeTpl &tpl ) :
	_parent( 0x0 ), _type( tpl.type ), _handle( 0 ), _flags( 0 ), _sortKey( 0 ),
	_dirty( true ), _transformed( true ), _renderable( false ),
   _name( tpl.name ), _attachment( tpl.attachmentString ), _userFlags(0), _accumulatedFlags(0),
   _renderStamp(0), _noInstancing(true)
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
	if( _dirty ) Modules::sceneMan().updateNodes();
	
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
	
	_relTrans = Matrix4f::ScaleMat( scale.x, scale.y, scale.z );
	_relTrans.rotate( degToRad( rot.x ), degToRad( rot.y ), degToRad( rot.z ) );
	_relTrans.translate( trans.x, trans.y, trans.z );

   if (_relTrans.c[3][0] != 0.0f && !(_relTrans.c[3][0] < 0.0f) && !(_relTrans.c[3][0] > 0.0f)) {
      Modules::log().writeDebugInfo( "Invalid transform! Zero edition.");
      DEBUG_ONLY( DebugBreak();)
   }

	markDirty();
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
	
	_relTrans = mat;

   if (_relTrans.c[3][0] != 0.0f && !(_relTrans.c[3][0] < 0.0f) && !(_relTrans.c[3][0] > 0.0f)) {
      Modules::log().writeDebugInfo( "Invalid transform! Zero edition.");
      DEBUG_ONLY( DebugBreak();)
   }
	
	markDirty();
}


void SceneNode::getTransMatrices( const float **relMat, const float **absMat ) const
{
	if( relMat != 0x0 )
	{
		if( _dirty ) Modules::sceneMan().updateNodes();
		*relMat = &_relTrans.x[0];
	}
	
	if( absMat != 0x0 )
	{
		if( _dirty ) Modules::sceneMan().updateNodes();
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
   markDirty();
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
		if( !child->_dirty )
		{	
			child->_dirty = true;
			child->_transformed = true;
			child->markChildrenDirty();
		}
	}
}


void SceneNode::markDirty()
{
	_dirty = true;
	_transformed = true;
	
	SceneNode *node = _parent;
	while( node != 0x0 )
	{
		node->_dirty = true;
		node = node->_parent;
	}

	markChildrenDirty();
}


void SceneNode::update()
{
	if( !_dirty ) return;
	
	onPreUpdate();
	
	// Calculate absolute matrix
	if( _parent != 0x0 ) {
		Matrix4f::fastMult43( _absTrans, _parent->_absTrans, _relTrans );
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

	_dirty = false;

   // Visit children
	for( uint32 i = 0, s = (uint32)_children.size(); i < s; ++i )
	{
		_children[i]->update();
	}	

	onFinishedUpdate();

   Modules::sceneMan().updateSpatialNode(*this);
}


bool SceneNode::checkIntersection( const Vec3f &/*rayOrig*/, const Vec3f &/*rayDir*/, Vec3f &/*intsPos*/, Vec3f &/*intsNorm*/ ) const
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

GridSpatialGraph::GridSpatialGraph()
{
   _renderStamp = 0;
}


void GridSpatialGraph::addNode(SceneNode const& sceneNode)
{	
   if(!sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light) {
      return;
   }

   if (sceneNode.getType() == SceneNodeTypes::Light && sceneNode.getParamI(LightNodeParams::DirectionalI)) {
      _directionalLights[sceneNode.getHandle()] = &sceneNode;
   } else {
      _nodeGridLookup[sceneNode.getHandle()] = boost::container::flat_set<uint32>();
      updateNode(sceneNode);
   }
}


void GridSpatialGraph::removeNode(SceneNode const& sceneNode)
{
   NodeHandle h = sceneNode.getHandle();
   if (sceneNode.getType() == SceneNodeTypes::Light && sceneNode.getParamI(LightNodeParams::DirectionalI)) {
      _directionalLights.erase(h);
   } else {
      // Find all old references and remove them.
      for (uint32 gridNum : _nodeGridLookup[h]) {
         int numRemoved = _gridElements[gridNum]._nodes.erase(&sceneNode);
         ASSERT(numRemoved == 1);
      }
      ASSERT(_nodeGridLookup.erase(h) == 1);
      if (_nocullNodes.find(h) != _nocullNodes.end()) {
         _nocullNodes.erase(h);
      }
   }
}

void GridSpatialGraph::boundingBoxToGrids(BoundingBox const& aabb, boost::container::flat_set<uint32>& gridElementList) const
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

   for (int x = minX; x <= maxX; x++) {
      for (int z = minZ; z <= maxZ; z++) {
         const uint32 v = hashGridPoint(x, z);
         gridElementList.insert(v);
      }
   }
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


void GridSpatialGraph::updateNode(SceneNode const& sceneNode)
{
   NodeHandle nh = sceneNode.getHandle();
   BoundingBox const& sceneBox = sceneNode.getBBox();
   if (!sceneBox.isValid()) {
      return;
   }

   if (sceneNode.getFlags() & SceneNodeFlags::NoCull) {
      _nocullNodes[sceneNode.getHandle()] = &sceneNode;
   } else {
      newGrids.clear();
      toRemove.clear();
   
      // Generate new bounds for this node.
      boundingBoxToGrids(sceneBox, newGrids);
      auto& newGridsEnd = newGrids.end();
      auto& nodeGridLookup = _nodeGridLookup[nh];

      // Remove obsolete grid references for the new node (if any).
      for (uint32 oldGridE : nodeGridLookup) {
         if (newGrids.find(oldGridE) == newGridsEnd) {
            toRemove.push_back(oldGridE);
         }
      }
      for (uint32 removeE : toRemove) {
         nodeGridLookup.erase(removeE);
         _gridElements[removeE]._nodes.erase(&sceneNode);
      }

      // Add new element references; update existing refs.
      for (uint32 newGridE : newGrids) {
         nodeGridLookup.insert(newGridE);
         auto const gei = _gridElements.find(newGridE);
         if (gei == _gridElements.end()) {
            GridElement newGridElement;
            int gX, gY;
            unhashGridHash(newGridE, &gX, &gY);
            newGridElement.bounds.addPoint(Vec3f(gX * GRIDSIZE, sceneBox.min().y, gY * GRIDSIZE));
            newGridElement.bounds.addPoint(Vec3f(gX * GRIDSIZE + GRIDSIZE, sceneBox.max().y, gY * GRIDSIZE + GRIDSIZE));
            _gridElements[newGridE] = newGridElement;
         }
         GridElement &ger = _gridElements[newGridE];
         
         // Insert does a check for redundant elements.
         ger._nodes.insert(&sceneNode);

         const Vec3f& boundsMin = ger.bounds.min();
         const Vec3f& boundsMax = ger.bounds.max();
         ger.bounds.addPoint(Vec3f(boundsMin.x, sceneBox.min().y, boundsMin.z));
         ger.bounds.addPoint(Vec3f(boundsMax.x, sceneBox.max().y, boundsMax.z));
      }
   }
}

void GridSpatialGraph::castRay(const Vec3f& rayOrigin, const Vec3f& rayDirection, std::function<void(boost::container::flat_set<SceneNode const*> const&nodes)> cb) {
   Vec3f rayDirN = rayDirection.normalized();
   for (auto const& ge : _gridElements) {
      float t = ge.second.bounds.intersectionOf(rayOrigin, rayDirN);
      if (t < 0.0) {
         continue;
      }
      cb(ge.second._nodes);
   }
}

void GridSpatialGraph::query(SpatialQuery const& query, RenderableQueues& renderableQueues, 
                         InstanceRenderableQueues& instanceQueues, std::vector<SceneNode const*>& lightQueue)
{
   _renderStamp++;
   const int curRenderStamp = _renderStamp;
   const int qFilterIgnore = query.filterIgnore;
   const int qFilterRequired = query.filterRequired;
   const int qUserFlags = query.userFlags;
   const int qUseRenderableQueue = query.useRenderableQueue;
   const int qOrder = query.order;
   const int qForceNoInstancing = query.forceNoInstancing;
   const int qUseLightQueue = query.useLightQueue;
   const Frustum& qFrustum = query.frustum;
   const Frustum* const qSecondaryFrustum = query.secondaryFrustum;

   ASSERT(query.useLightQueue || query.useRenderableQueue);

   Modules::sceneMan().updateNodes();

   for (auto const& ge : _gridElements) {
      if (qFrustum.cullBox(ge.second.bounds) && (qSecondaryFrustum == 0x0 || qSecondaryFrustum->cullBox(ge.second.bounds))) {
         continue;
      }

      for (SceneNode const* const node: ge.second._nodes) {
         int nRenderStamp = node->getRenderStamp();
         if (nRenderStamp == curRenderStamp) {
            continue;
         }
         node->setRenderStamp(curRenderStamp);

         if (node->_accumulatedFlags & qFilterIgnore) {
            continue;
         }
         if ((node->_accumulatedFlags & qFilterRequired) != qFilterRequired) {
            continue;
         }
         if ((node->_userFlags & qUserFlags) != qUserFlags) {
            continue;
         }

         if (qUseLightQueue && node->_type == SceneNodeTypes::Light) {		 
            lightQueue.push_back(node);
         } else if (qUseRenderableQueue) {
            if (!node->_renderable) {
               continue;
            }

            if (qFrustum.cullBox(node->_bBox) && (qSecondaryFrustum == 0x0 || qSecondaryFrustum->cullBox(node->_bBox))) {
               continue;
            }

            float sortKey = 0;

            switch( qOrder )
            {
            case RenderingOrder::StateChanges:
               sortKey = node->_sortKey;
               break;
            case RenderingOrder::FrontToBack:
               sortKey = nearestDistToAABB( qFrustum.getOrigin(), node->_bBox.min(), node->_bBox.max() );
               break;
            case RenderingOrder::BackToFront:
               sortKey = -nearestDistToAABB( qFrustum.getOrigin(), node->_bBox.min(), node->_bBox.max() );
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
   }

   if (query.useRenderableQueue) {
      for (auto const& nc : _nocullNodes) {
         SceneNode const* n = nc.second;
         n->setRenderStamp(_renderStamp);

         if (n->_accumulatedFlags & query.filterIgnore) {
            continue;
         }
         if ((n->_accumulatedFlags & query.filterRequired) != query.filterRequired) {
            continue;
         }
         if ((n->_userFlags & query.userFlags) != query.userFlags) {
            continue;
         }
         if (renderableQueues.find(n->_type) == renderableQueues.end()) {
            renderableQueues[n->_type] = RenderableQueue();
            renderableQueues[n->_type].reserve(1000);
         }
         renderableQueues[n->_type].emplace_back( RendQueueItem( n->_type, 0, n ) );
      }
   }

   if (query.useLightQueue) {
      for (auto const& d : _directionalLights) {
         SceneNode const* n = d.second;
         if (n->getRenderStamp() == _renderStamp) {
            continue;
         }
         n->setRenderStamp(_renderStamp);

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
}
// =================================================================================================
// Class SpatialGraph
// =================================================================================================

SpatialGraph::SpatialGraph()
{
}


void SpatialGraph::addNode(SceneNode const& sceneNode)
{	
	if( !sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light ) return;
   ASSERT(sceneNode._handle);
   _nodes[sceneNode._handle] = &sceneNode;
   SCENE_LOG(9) << "adding node " << sceneNode._handle << " to SpatialGraph";
}


void SpatialGraph::removeNode(SceneNode const& sceneNode )
{
	if( !sceneNode._renderable && sceneNode._type != SceneNodeTypes::Light ) return;

   auto i = _nodes.find(sceneNode._handle), end = _nodes.end();
   ASSERT(i != end);
   if (i != end) {
      SCENE_LOG(9) << "removing node " << sceneNode._handle << " from SpatialGraph";
      _nodes.erase(i);
   }
}


void SpatialGraph::updateNode(SceneNode const& sceneNode)
{
	// Since the spatial graph is just a flat list of objects,
	// there is nothing to do at the moment
}


void SpatialGraph::query(const SpatialQuery& query, RenderableQueues& renderableQueues, 
                         InstanceRenderableQueues& instanceQueues, std::vector<SceneNode const*>& lightQueue)
{
   ASSERT(query.useLightQueue || query.useRenderableQueue);

#define QUERY_LOG() SCENE_LOG(9) << "  query node (" << node->_handle << " " << node->getName() << ") " 

   Modules::sceneMan().updateNodes();

   SCENE_LOG(9) << "running spatial graph query:";
   // Culling
   for (auto const& entry : _nodes) {
      SceneNode const* node = entry.second;
      QUERY_LOG() "considering...";

      if (node->_accumulatedFlags & query.filterIgnore) {
         QUERY_LOG() << "flags " << FlagsToString(node->_accumulatedFlags) << " intersect query ignore flags " << FlagsToString(query.filterIgnore) << ".  ignorning.";
         continue;
      }
      if ((node->_accumulatedFlags & query.filterRequired) != query.filterRequired) {
         QUERY_LOG() << "flags " << FlagsToString(node->_accumulatedFlags) << " do not match query required flags " << FlagsToString(query.filterRequired) << ".  ignorning.";
         continue;
      }
      if ((node->_userFlags & query.userFlags) != query.userFlags) {
         QUERY_LOG() << "flags " << FlagsToString(node->_userFlags) << " do not match query user flags" << FlagsToString(query.userFlags) << ".  ignorning.";
         continue;
      }

      if (query.useRenderableQueue) {
         if (!node->_renderable) {
            QUERY_LOG() << "not renderable.  igorning";
            continue;
         }

         if (query.frustum.cullBox( node->_bBox ) && (query.secondaryFrustum == 0x0 || query.secondaryFrustum->cullBox( node ->_bBox ))) {
            QUERY_LOG() << "culled by frustum.  igorning";
            continue;
         }

         float sortKey = 0;

         switch( query.order )
         {
         case RenderingOrder::StateChanges:
            sortKey = node->_sortKey;
            break;
         case RenderingOrder::FrontToBack:
            sortKey = nearestDistToAABB( query.frustum.getOrigin(), node->_bBox.min(), node->_bBox.max() );
            break;
         case RenderingOrder::BackToFront:
            sortKey = -nearestDistToAABB( query.frustum.getOrigin(), node->_bBox.min(), node->_bBox.max() );
            break;
         }

         if (node->getInstanceKey() != 0x0 && !query.forceNoInstancing) {
            if (instanceQueues.find(node->_type) == instanceQueues.end()) {
               instanceQueues[node->_type] = InstanceRenderableQueue();
            }
            InstanceRenderableQueue& iq = instanceQueues[node->_type];

            const InstanceKey& ik = *(node->getInstanceKey());
            if (iq.find(ik) == iq.end()) {
               iq[ik] = RenderableQueue();
               iq[ik].reserve(1000);
            }
            iq[ik].emplace_back(RendQueueItem(node->_type, sortKey, node));
            QUERY_LOG() << "added to instance renderable queue (geo:" << ik.geoResource->getHandle() << " " << ik.geoResource->getName() << ").";
         } else {
            if (renderableQueues.find(node->_type) == renderableQueues.end()) {
               renderableQueues[node->_type] = RenderableQueue();
               renderableQueues[node->_type].reserve(1000);
            }
            renderableQueues[node->_type].emplace_back( RendQueueItem( node->_type, sortKey, node ) );
            QUERY_LOG() << "added to renderable queue.";
         }
      }
      if (query.useLightQueue && node->_type == SceneNodeTypes::Light) {		 
         QUERY_LOG() << "added to light queue.";
         lightQueue.push_back( node );
      }
   }

#undef QUERY_LOG

   // Sort
   if( query.order != RenderingOrder::None ) {
      for (auto& item : renderableQueues) {
         SCENE_LOG(9) << "sorting renderable queue " << item.first << " of length " << item.second.size();
         std::sort( item.second.begin(), item.second.end(), RendQueueItemCompFunc() );
      }

      for (auto& item : instanceQueues) {
         SCENE_LOG(9) << "sorting instance queue " << item.first << " of length " << item.second.size();
         for (auto& queue : item.second) {
            std::sort(queue.second.begin(), queue.second.end(), RendQueueItemCompFunc());
         }
      }
   }
}


// *************************************************************************************************
// Class SceneManager
// *************************************************************************************************

SceneManager::SceneManager()
{
   initialize();
}


SceneManager::~SceneManager()
{
   shutdown();
}

void SceneManager::reset()
{
   shutdown();
   initialize();
}

void SceneManager::shutdown()
{
   delete _spatialGraph; _spatialGraph = nullptr;

   for (auto &entry : _nodes) {
      delete entry.second;
   }
   _nodes.clear();
}

void SceneManager::initialize()
{
   SceneNode *rootNode = GroupNode::factoryFunc( GroupNodeTpl( "RootNode" ) );
   rootNode->_handle = RootNode;
   _nodes[RootNode] = rootNode;

   _spatialGraph = new GridSpatialGraph();  
   _queryCacheCount = 0;
   _currentQuery = -1;

   for (int i = 0; i < QueryCacheSize; i++) {
      _queryCache[i].lightQueue.reserve(20);
   }
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
}


NodeRegEntry *SceneManager::findType( int type )
{
	map< int, NodeRegEntry >::iterator itr = _registry.find( type );
	
	if( itr != _registry.end() ) return &itr->second;
	else return 0x0;
}


NodeRegEntry *SceneManager::findType( std::string const& typeString )
{
	map< int, NodeRegEntry >::iterator itr = _registry.begin();

	while( itr != _registry.end() )
	{
		if( itr->second.typeString == typeString ) return &itr->second;

		++itr;
	}
	
	return 0x0;
}


void SceneManager::updateNodes()
{
   radiant::perfmon::TimelineCounterGuard un("updateNodes");
	getRootNode().update();
}


void SceneManager::updateQueues( const char* reason, const Frustum &frustum1, const Frustum *frustum2, RenderingOrder::List order,
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


std::vector<SceneNode const*>& SceneManager::getLightQueue() 
{ 
   ASSERT(_currentQuery != -1);
   return _queryCache[_currentQuery].lightQueue;
}


RenderableQueue& SceneManager::getRenderableQueue(int itemType) 
{ 
   ASSERT(_currentQuery != -1);
   RenderableQueues& renderQueues = _queryCache[_currentQuery].renderableQueues;
   if (renderQueues.find(itemType) == renderQueues.end()) {
      renderQueues[itemType] = RenderableQueue();
      renderQueues[itemType].reserve(1000);
   }
   return renderQueues[itemType];
}

RenderableQueues& SceneManager::getRenderableQueues()
{
   ASSERT(_currentQuery != -1);
   return _queryCache[_currentQuery].renderableQueues;
}

InstanceRenderableQueue& SceneManager::getInstanceRenderableQueue(int itemType)
{
   ASSERT(_currentQuery != -1);
   InstanceRenderableQueues& renderQueues = _queryCache[_currentQuery].instanceRenderableQueues;
   if (renderQueues.find(itemType) == renderQueues.end()) {
      renderQueues[itemType] = InstanceRenderableQueue();
   }
   return renderQueues[itemType];
}

NodeHandle SceneManager::parseNode( SceneNodeTpl &tpl, SceneNode *parent )
{
	if( parent == 0x0 ) return 0;
	
	SceneNode *sn = 0x0;

	if( tpl.type == 0 )
	{
		// Reference node
		NodeHandle handle = parseNode( *((ReferenceNodeTpl *)&tpl)->sgRes->getRootNode(), parent );
		sn = Modules::sceneMan().resolveNodeHandle( handle );
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


NodeHandle SceneManager::addNode( SceneNode *node, SceneNode &parent )
{
	if( node == 0x0 ) return 0;

   _registry[node->_type].nodes[node->getHandle()] = node;
	
	// Check if node can be attached to parent
	if( !node->canAttach( parent ) )
	{
		Modules::log().writeDebugInfo( "Can't attach node '%s' to parent '%s'", node->_name.c_str(), parent._name.c_str() );
		delete node; node = 0x0;
		return 0;
	}
	
	node->_parent = &parent;
	node->_handle = _nextHandleValue++;
	_nodes[node->_handle] = node;

	// Attach to parent
	parent._children.push_back( node );

	// Raise event
	node->onAttach( parent );

	// Mark tree as dirty
	node->markDirty();

	// Register node in spatial graph
	_spatialGraph->addNode( *node );
        return node->_handle;
}


NodeHandle SceneManager::addNodes( SceneNode &parent, SceneGraphResource &sgRes )
{
	// Parse root node
	return parseNode( *sgRes.getRootNode(), &parent );
}


void SceneManager::removeNodeRec( SceneNode &node )
{
	NodeHandle handle = node._handle;
	
	// Raise event
	if( handle != RootNode ) node.onDetach( *node._parent );

   _registry[node.getType()].nodes.erase(node.getHandle());

	// Remove children
	for( uint32 i = 0; i < node._children.size(); ++i )
	{
		removeNodeRec( *node._children[i] );
	}
	
	// Delete node
	if( handle != RootNode )
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


void SceneManager::removeNode( SceneNode &node )
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
				parent->_children.erase( parent->_children.begin() + i );
				break;
			}
		}
		parent->markDirty();
	}
	else  // Rootnode
	{
		node._children.clear();
		node.markDirty();
	}
}


bool SceneManager::relocateNode( SceneNode &node, SceneNode &parent )
{
	if( node._handle == RootNode ) return false;
	
	if( !node.canAttach( parent ) )
	{	
		Modules::log().writeDebugInfo( "Can't attach node to parent in h3dSetNodeParent" );
		return false;
	}
	
	// Detach from old parent
	node.onDetach( *node._parent );
	for( uint32 i = 0; i < node._parent->_children.size(); ++i )
	{
		if( node._parent->_children[i] == &node )
		{
			node._parent->_children.erase( node._parent->_children.begin() + i );
			break;
		}
	}

	// Attach to new parent
	parent._children.push_back( &node );
	node._parent = &parent;
	node.onAttach( parent );
	
	parent.markDirty();
	node._parent->markDirty();
	
	return true;
}


int SceneManager::findNodes( SceneNode &startNode, std::string const& name, int type )
{
   _findResults.clear();


   if (startNode.getHandle() == RootNode) {
      // Since we want all nodes of that type, just consult the registry.
      for (auto& n : _registry[type].nodes) {
         if (name == "" || n.second->_name == name) {
            _findResults.push_back(n.second);
         }
      }
   } else {
      _findNodes(startNode, name, type);
   }

	return _findResults.size();
}


void SceneManager::_findNodes( SceneNode &startNode, std::string const& name, int type )
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


void SceneManager::fastCastRayInternal(int userFlags)
{
   _spatialGraph->castRay(-_rayOrigin, -_rayDirection, [this, userFlags](boost::container::flat_set<SceneNode const*> const&nodes) {
      for (SceneNode const* sn : nodes) {
         if( !(sn->_accumulatedFlags & SceneNodeFlags::NoRayQuery) )
	      {
		      Vec3f intsPos, intsNorm;
            if( (sn->_userFlags & userFlags) == userFlags && sn->checkIntersection( _rayOrigin, _rayDirection, intsPos, intsNorm ) )
		      {
			      float dist = (intsPos - _rayOrigin).length();

			      CastRayResult crr;
			      crr.node = sn;
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
         }
      }
   });
}


void SceneManager::castRayInternal( SceneNode &node, int userFlags )
{
   if( !(node._accumulatedFlags & SceneNodeFlags::NoRayQuery) )
	{
		Vec3f intsPos, intsNorm;
      if( (node._userFlags & userFlags) == userFlags && node.checkIntersection( _rayOrigin, _rayDirection, intsPos, intsNorm ) )
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


int SceneManager::castRay( SceneNode &node, const Vec3f &rayOrig, const Vec3f &rayDir, int numNearest, int userFlags )
{
	_castRayResults.resize( 0 );  // Clear without affecting capacity

   if( node._accumulatedFlags & SceneNodeFlags::NoRayQuery ) return 0;

	_rayOrigin = rayOrig;
	_rayDirection = rayDir;
	_rayNum = numNearest;

   if (node.getHandle() == RootNode) {
      fastCastRayInternal(userFlags);
   } else {
   	castRayInternal( node, userFlags );
   }
	return (int)_castRayResults.size();
}


bool SceneManager::getCastRayResult( int index, CastRayResult &crr )
{
	if( (uint32)index < _castRayResults.size() )
	{
		crr = _castRayResults[index];

		return true;
	}

	return false;
}


int SceneManager::checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion )
{
	// Note: This function is a bit hacky with all the hard-coded node types
	
	if( node._dirty ) updateNodes();

	// Check occlusion
	if( checkOcclusion && cam._occSet >= 0 )
	{
		if( node.getType() == SceneNodeTypes::Mesh && cam._occSet < (int)((MeshNode *)&node)->_occQueries.size() )
		{
			if( gRDI->getQueryResult( ((MeshNode *)&node)->_occQueries[cam._occSet] ) < 1 )
				return -1;
		}
		else if( node.getType() == SceneNodeTypes::Emitter && cam._occSet < (int)((EmitterNode *)&node)->_occQueries.size() )
		{
			if( gRDI->getQueryResult( ((EmitterNode *)&node)->_occQueries[cam._occSet] ) < 1 )
				return -1;
		}
		else if( node.getType() == SceneNodeTypes::Light && cam._occSet < (int)((LightNode *)&node)->_occQueries.size() )
		{
			if( gRDI->getQueryResult( ((LightNode *)&node)->_occQueries[cam._occSet] ) < 1 )
				return -1;
		}
	}
	
	// Frustum culling
	if( cam.getFrustum().cullBox( node.getBBox() ) )
		return -1;
	else
		return 0;
}


void SceneManager::clearQueryCache()
{
   // Do a pass to see if we have too many renderable queues.  TODO(klochek): collect some stats
   // for each queue, so that we can more-intelligently get rid of disused queues.
   SCENE_LOG(5) << "clearing query cache";

   for (int i = 0; i < _queryCacheCount; i++) {
      int totalRQs = 0;
      for (const auto& iqs : _queryCache[i].instanceRenderableQueues) {
         totalRQs += iqs.second.size();
      }

      totalRQs += _queryCache[i].renderableQueues.size();

      if (totalRQs > MaxActiveRenderQueues) {
         Modules::log().writeWarning("Total RQs for query %d exceeds maximum; flushing.", i);
         _queryCache[i].instanceRenderableQueues.clear();
         _queryCache[i].renderableQueues.clear();
      }
   }

   _queryCacheCount = 0;
   _currentQuery = -1;
}


int SceneManager::_checkQueryCache(const SpatialQuery& query)
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
}

SceneNode *SceneManager::resolveNodeHandle(NodeHandle handle)
{
   auto i = _nodes.find(handle), end = _nodes.end();
   return i != end ? i->second : nullptr;
}

}  // namespace
