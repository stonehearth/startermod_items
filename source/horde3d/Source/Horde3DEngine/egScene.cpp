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
	_name( tpl.name ), _attachment( tpl.attachmentString ), _userFlags(0)
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


void SceneNode::setFlags( int flags, bool recursive )
{
   SCENE_LOG(8) << "setting flags on node " << _handle << " to " << FlagsToString(flags);
   _flags = flags;

   if( recursive )
   {
      for( size_t i = 0, s = _children.size(); i < s; ++i )
      {
         _children[i]->setFlags( flags, true );
      }
   }
}

void SceneNode::twiddleFlags( int flags, bool on, bool recursive )
{
   int oldFlags = _flags;
   if (on) {
      _flags |= flags;
   } else {
      _flags &= ~flags;
   }
   SCENE_LOG(8) << "twiddling " << FlagsToString(flags) << " flags on node " << _handle << " (was:" << FlagsToString(oldFlags) << " now:" << FlagsToString(_flags);

   if( recursive )
   {
      for( size_t i = 0, s = _children.size(); i < s; ++i )
      {
         _children[i]->twiddleFlags( flags, on, true );
      }
   }
}


int SceneNode::getParamI( int param )
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

	onPostUpdate();

	_dirty = false;

   // Visit children
	for( uint32 i = 0, s = (uint32)_children.size(); i < s; ++i )
	{
		_children[i]->update();
	}	

	onFinishedUpdate();

   Modules::sceneMan().updateSpatialNode(_handle);
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


// =================================================================================================
// Class SpatialGraph
// =================================================================================================

SpatialGraph::SpatialGraph()
{
}


void SpatialGraph::addNode( SceneNode &sceneNode )
{	
   ASSERT(sceneNode._handle);
   _nodes[sceneNode._handle] = &sceneNode;
   SCENE_LOG(9) << "adding node " << sceneNode._handle << " to SpatialGraph";
}


void SpatialGraph::removeNode( uint32 sgHandle )
{
   if (sgHandle) {
      auto i = _nodes.find(sgHandle), end = _nodes.end();
      ASSERT(i != end);
      if (i != end) {
         SCENE_LOG(9) << "removing node " << sgHandle << " from SpatialGraph";
         _nodes.erase(i);
      }
   }
}


void SpatialGraph::updateNode( uint32 sgHandle )
{
	// Since the spatial graph is just a flat list of objects,
	// there is nothing to do at the moment
}


struct RendQueueItemCompFunc
{
	bool operator()( const RendQueueItem &a, const RendQueueItem &b ) const
		{ return a.sortKey < b.sortKey; }
};


void SpatialGraph::query(const SpatialQuery& query, RenderableQueues& renderableQueues, 
                         InstanceRenderableQueues& instanceQueues, std::vector<SceneNode*>& lightQueue)
{
   ASSERT(query.useLightQueue || query.useRenderableQueue);

#define QUERY_LOG() SCENE_LOG(9) << "  query node (" << node->_handle << " " << node->getName() << ") " 

   Modules::sceneMan().updateNodes();

   SCENE_LOG(9) << "running spatial graph query:";
   // Culling
   for (auto const& entry : _nodes) {
      SceneNode *node = entry.second;
      if (node == 0x0) {
         SCENE_LOG(9) << "  node " << entry.first << " is null!  ignoring.";
         continue;
      }
      QUERY_LOG() "considering...";

      if (node->_flags & query.filterIgnore) {
         QUERY_LOG() << "flags " << FlagsToString(node->_flags) << " intersect query ignore flags " << FlagsToString(query.filterIgnore) << ".  ignorning.";
         continue;
      }
      if ((node->_flags & query.filterRequired) != query.filterRequired) {
         QUERY_LOG() << "flags " << FlagsToString(node->_flags) << " do not match query required flags " << FlagsToString(query.filterRequired) << ".  ignorning.";
         continue;
      }
      if ((node->_userFlags & query.userFlags) != query.userFlags) {
         QUERY_LOG() << "flags " << FlagsToString(node->_flags) << " do not match query user flags" << FlagsToString(query.userFlags) << ".  ignorning.";
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
            iq[ik].push_back(RendQueueItem(node->_type, sortKey, node));
            QUERY_LOG() << "added to instance renderable queue (geo:" << ik.geoResource->getHandle() << " " << ik.geoResource->getName() << ").";
         } else {
            if (renderableQueues.find(node->_type) == renderableQueues.end()) {
               renderableQueues[node->_type] = RenderableQueue();
               renderableQueues[node->_type].reserve(1000);
            }
            renderableQueues[node->_type].push_back( RendQueueItem( node->_type, sortKey, node ) );
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

   _spatialGraph = new SpatialGraph();  
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


std::vector<SceneNode*>& SceneManager::getLightQueue() 
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

	// Remove children
	for( uint32 i = 0; i < node._children.size(); ++i )
	{
		removeNodeRec( *node._children[i] );
	}
	
	// Delete node
	if( handle != RootNode )
	{
		_spatialGraph->removeNode(node._handle);
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

   _findNodes(startNode, name, type);
	
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


void SceneManager::castRayInternal( SceneNode &node, int userFlags )
{
	if( !(node._flags & SceneNodeFlags::NoRayQuery) )
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

	if( node._flags & SceneNodeFlags::NoRayQuery ) return 0;

	_rayOrigin = rayOrig;
	_rayDirection = rayDir;
	_rayNum = numNearest;

	castRayInternal( node, userFlags );

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
