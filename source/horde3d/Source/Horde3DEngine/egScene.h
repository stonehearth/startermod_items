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

#ifndef _egScene_H_
#define _egScene_H_

#include "egPrerequisites.h"
#include "utMath.h"
#include "egPrimitives.h"
#include "egPipeline.h"
#include <map>
#include <unordered_map>
#include <boost\container\flat_map.hpp>
#include <boost\container\flat_set.hpp>


namespace Horde3D {

struct SceneNodeTpl;
class CameraNode;
class SceneGraphResource;


const int QueryCacheSize = 32;

class SceneNode;
class InstanceKey {
public:
   Resource* geoResource;
   MaterialResource* matResource;
   SceneNode* node;
   float scale;
   size_t hash;

   InstanceKey();
   void updateHash();
   bool operator==(const InstanceKey& other) const;
   bool operator!=(const InstanceKey& other) const;
};

struct hash_InstanceKey {
   inline size_t operator()(InstanceKey const & x) const {
      return x.hash;
   }
};

// =================================================================================================
// Scene Node
// =================================================================================================

struct SceneNodeDirtyKind
{
   enum List
   {
      Ancestors = 0x1,
      Children = 0x2,
      All = 0x3
   };
};

struct SceneNodeTypes
{
	enum List
	{
		Undefined = 0,
		Group,
		Model,
		Mesh,
		Joint,
		Light,
		Camera,
		Emitter,
      VoxelModel,
      VoxelMesh,
      HudElement,
      InstanceNode,
      ProjectorNode,
      VoxelJointNode
	};
};

struct SceneNodeParams
{
	enum List
	{
		NameStr = 1,
		AttachmentStr,
      UserFlags
	};
};

struct SceneNodeFlags
{
	enum List
	{
		NoDraw = 0x1,
		NoCastShadow = 0x2,
		NoRayQuery = 0x4,
		Inactive = 0x7,  // NoDraw | NoCastShadow | NoRayQuery
      NoCull = 0x8
	};
};

struct SceneNodeDirtyState
{
   enum List
   {
      Clean = 0,
      Partial = 1,
      Dirty = 2
   };
};

// =================================================================================================

struct SceneNodeTpl
{
	int                            type;
	std::string                    name;
	Vec3f                          trans, rot, scale;
	std::string                    attachmentString;
	std::vector< SceneNodeTpl * >  children;

	SceneNodeTpl( int type, std::string const& name ) :
		type( type ), name( name ), scale( Vec3f ( 1, 1, 1 ) )
	{
	}
	
	virtual ~SceneNodeTpl()
	{
		for( uint32 i = 0; i < children.size(); ++i ) delete children[i];
	}
};

// =================================================================================================

class SceneNode
{
public:
	SceneNode( const SceneNodeTpl &tpl );
	virtual ~SceneNode();

   void getTransformFast(Vec3f &trans, Vec3f &rot, Vec3f &scale) const;
   void getAbsTransform(Vec3f &trans, Vec3f &rot, Vec3f &scale);
	void getTransform( Vec3f &trans, Vec3f &rot, Vec3f &scale );	// Not virtual for performance
	void setTransform( Vec3f trans, Vec3f rot, Vec3f scale );	// Not virtual for performance
	void setTransform( const Matrix4f &mat );
	void getTransMatrices( const float **relMat, const float **absMat ) const;

	int getFlags() const { return _flags; }
   void setFlags( int flags, bool recursive );
   void twiddleFlags( int flags, bool on, bool recursive );

	virtual int getParamI(int param) const;
	virtual void setParamI( int param, int value );
	virtual float getParamF( int param, int compIdx );
	virtual void setParamF( int param, int compIdx, float value );
	virtual const char *getParamStr( int param );
	virtual void setParamStr( int param, const char* value );
   virtual void* mapParamV( int param );
   virtual void unmapParamV( int param, int mappedLength );

	virtual void setLodLevel( int lodLevel );

	virtual bool canAttach( SceneNode &parent );
   void markDirty(uint32 dirtyKind);
	void update();

   bool checkIntersection(const Vec3f &rayOrig, const Vec3f& rayEnd, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm) const;
   virtual bool checkIntersectionInternal( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const;

   inline const InstanceKey* getInstanceKey() const { 
      if (_noInstancing || _instanceKey.geoResource == nullptr || _instanceKey.matResource == nullptr) {
         return nullptr;
      }
      return &_instanceKey; 
   }
   virtual const long sortKey() { return 0x0; }
	int getType() const { return _type; }
	NodeHandle getHandle() const { return _handle; }
	SceneNode *getParent() const { return _parent; }
	std::string const& getName() const { return _name; }
	std::vector< SceneNode * > &getChildren() { return _children; }
	Matrix4f const& getRelTrans() const { return _relTrans; }
	Matrix4f const& getAbsTrans() const { return _absTrans; }
	const BoundingBox &getBBox() const { return _bBox; }
   void updateBBox(const BoundingBox& bbox);
   bool isRenderable() const { return _renderable; }

	std::string const& getAttachmentString() { return _attachment; }
	void setAttachmentString( const char* attachmentData ) { _attachment = attachmentData; }
	bool checkTransformFlag( bool reset )
		{ bool b = _transformed; if( reset ) _transformed = false; return b; }

protected:
   enum SetFlagsMode {
      SET,
      TWIDDLE_ON,
      TWIDDLE_OFF,
      NOP
   };
	void markChildrenDirty();
   void updateAccumulatedFlags();
   void updateFlagsRecursive(int flags, SetFlagsMode mode, bool recursive);

	virtual void onPreUpdate() {}  // Called before absolute transformation is updated
	virtual void onPostUpdate() {}  // Called after absolute transformation has been updated
	virtual void onFinishedUpdate() {}  // Called after children have been updated
	virtual void onAttach( SceneNode &parentNode ) {}  // Called when node is attached to parent
	virtual void onDetach( SceneNode &parentNode ) {}  // Called when node is detached from parent

protected:
	Matrix4f                    _relTrans, _absTrans;  // Transformation matrices
	SceneNode                   *_parent;  // Parent node
	int                         _type;
	NodeHandle                  _handle;
   int                         _gridId;
   int                         _gridPos;


	uint32                      _flags;
   uint32                      _userFlags;
   uint32                      _accumulatedFlags;
	float                       _sortKey;
   InstanceKey                 _instanceKey;
	uint32                      _dirty;  // Does the node need to be updated?
	bool                        _transformed;
	bool                        _renderable;
   bool                        _noInstancing;

	BoundingBox                 _bBox;  // AABB in world space

	std::vector< SceneNode * >  _children;  // Child nodes
	std::string                 _name;
	std::string                 _attachment;  // User defined data


private:

   friend class Scene;
	friend class SpatialGraph;
   friend class GridSpatialGraph;
	friend class Renderer;
};


// =================================================================================================
// Group Node
// =================================================================================================

struct GroupNodeTpl : public SceneNodeTpl
{
	GroupNodeTpl( std::string const& name ) :
		SceneNodeTpl( SceneNodeTypes::Group, name )
	{
	}
};

// =================================================================================================

class GroupNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	friend class Renderer;
	friend class Scene;

protected:
	GroupNode( const GroupNodeTpl &groupTpl );
};


// =================================================================================================
// Spatial Graph
// =================================================================================================

struct RendQueueItem
{
	SceneNode  const*node;
	float      sortKey;

	RendQueueItem() {}
	RendQueueItem(float sortKey, SceneNode const*node) : node(node), sortKey(sortKey) {}
};

typedef std::vector<RendQueueItem> RenderableQueue;
typedef boost::container::flat_map<int, std::shared_ptr<RenderableQueue> > RenderableQueues;
typedef std::unordered_map<InstanceKey, std::shared_ptr<RenderableQueue>, hash_InstanceKey > InstanceRenderableQueue;
typedef boost::container::flat_map<int, InstanceRenderableQueue > InstanceRenderableQueues;


struct GridItem {
   GridItem(BoundingBox const& b, SceneNode* n) : bounds(b), node(n) 
   {
   }
   BoundingBox bounds;
   SceneNode* node;
   float sortKey;
   RenderableQueue* renderQueues[RenderCacheSize];
};

struct GridElement 
{
   BoundingBox bounds;
   std::vector<GridItem> nodes[2];
};


struct QueryTypes
{
   enum List
   {
      CullableRenderables = 1,
      Lights = 2,
      UncullableRenderables = 4,
      AllRenderables = 5
   };
};
struct QueryResult
{
   QueryResult() {}
   QueryResult(BoundingBox const& b, SceneNode* n) : bounds(b), node(n) {
   }
   QueryResult(BoundingBox const& b, SceneNode* n, RenderableQueue* const queues[]) : bounds(b), node(n) {
      for (int i = 0; i < RenderCacheSize; i++) {
         renderQueues[i] =  queues[i];
      }
   }

   BoundingBox bounds;
   SceneNode* node;
   float sortKey;
   RenderableQueue* renderQueues[RenderCacheSize];
};


class GridSpatialGraph
{
public:
   GridSpatialGraph(SceneId sceneId);

   void addNode(SceneNode& sceneNode);
	void removeNode(SceneNode& sceneNode);
	void updateNode(SceneNode& sceneNode);

   GridItem* gridItemForNode(SceneNode const& node);
   void query(Frustum const& frust, std::vector<QueryResult>& results, QueryTypes::List queryTypes);
   void castRay(const Vec3f& rayOrigin, const Vec3f& rayDirection, std::function<void(std::vector<GridItem> const& nodes)> cb);
   void updateNodeInstanceKey(SceneNode& sceneNode);

protected:
   void swapAndRemove(std::vector<GridItem>& vec, int index);
   int boundingBoxToGrid(BoundingBox const& aabb) const;
   inline uint32 hashGridPoint(int x, int y) const;
   inline void unhashGridHash(uint32 hash, int* x, int* y) const;
   void _queryGrid(std::vector<GridItem> const& nodes, Frustum const& frust, std::vector<QueryResult>& results);
   void _queryGridLight(std::vector<GridItem> const& nodes, Frustum const& frust, std::vector<QueryResult>& results);
   std::unordered_map<NodeHandle, SceneNode *> _directionalLights;

   std::unordered_map<uint32, GridElement> _gridElements;
   std::vector<GridItem> _noCullNodes;
   
   std::vector<GridItem> _spilloverNodes[2];

private:
   SceneId _sceneId;
};


// =================================================================================================
// Scene Manager
// =================================================================================================

typedef SceneNodeTpl *(*NodeTypeParsingFunc)( std::map< std::string, std::string > &attribs );
typedef SceneNode *(*NodeTypeFactoryFunc)( const SceneNodeTpl &tpl );
typedef void (*NodeTypeRenderFunc)(SceneId sceneId, std::string const& shaderContext, bool debugView,
                                    const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                                    int occSet, int lodLevel );
typedef void (*NodeTypeInstanceRenderFunc)(SceneId sceneId, std::string const& shaderContext, bool debugView,
                                    const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                                    int occSet, int lodLevel, bool cached );

struct NodeRegEntry
{
	std::string          typeString;
	NodeTypeParsingFunc  parsingFunc;
	NodeTypeFactoryFunc  factoryFunc;
	NodeTypeRenderFunc   renderFunc;
   NodeTypeInstanceRenderFunc   instanceRenderFunc;
   std::unordered_map<NodeHandle, SceneNode*> nodes;
};

struct CastRayResult
{
	SceneNode const* node;
	float      distance;
	Vec3f      intersection;
   Vec3f      normal;
};

struct CachedQueryResult 
{
   Frustum frust;
   std::vector<QueryResult> result;
   QueryTypes::List queryTypes;
   SceneNode const* singleNode;
};

// =================================================================================================
class SceneManager {
public:
   SceneManager();
   void reset();
   ~SceneManager();

	void registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
	                   NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeInstanceRenderFunc irf );
   bool sceneExists(SceneId sceneId) const;
   Scene& sceneForNode(NodeHandle handle);
   Scene& sceneForId(SceneId handle);
   SceneId sceneIdFor(NodeHandle handle) const;
   SceneId addScene(const char* name);
   void clear();
   std::vector<Scene*> const& getScenes() const;

	SceneNode *resolveNodeHandle(NodeHandle handle);
	int checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion );
	NodeHandle addNode( SceneNode *node, SceneNode &parent );
	NodeHandle addNodes( SceneNode &parent, SceneGraphResource &sgRes );
	void removeNode( SceneNode &node );
	bool relocateNode( SceneNode &node, SceneNode &parent );
   void clearQueryCaches();

private:
   std::vector<Scene*> _scenes;
	std::map< int, NodeRegEntry >  _registry;  // Registry of node types
};


class Scene
{
public:
	Scene(SceneId sceneId);
	~Scene();

   void reset();
   SceneId getId() const;

	void registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
	                   NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeInstanceRenderFunc irf );
	NodeRegEntry *findType( int type );
	NodeRegEntry *findType( std::string const& typeString );
	
	void updateNodes();
	void updateSpatialNode(SceneNode& node);

   void setNodeHidden(SceneNode& node, bool hide);

   std::vector<QueryResult> const& queryScene(Frustum const& frust, QueryTypes::List queryTypes);
   std::vector<QueryResult> const& subQuery(std::vector<QueryResult> const& queryResults, SceneNodeFlags::List ignoreFlags);
   std::vector<QueryResult> const& queryNode(SceneNode& node);
	
	NodeHandle addNode( SceneNode *node, SceneNode &parent );
	NodeHandle addNodes( SceneNode &parent, SceneGraphResource &sgRes );
	void removeNode( SceneNode &node );
	bool relocateNode( SceneNode &node, SceneNode &parent );
   void updateNodeInstanceKey(SceneNode& node);
	
	int findNodes( SceneNode &startNode, std::string const& name, int type );
	SceneNode *getFindResult( int index ) { return (unsigned)index < _findResults.size() ? _findResults[index] : 0x0; }
	
	int castRay( SceneNode &node, const Vec3f &rayOrig, const Vec3f &rayDir, int numNearest, int userFlags );
	bool getCastRayResult( int index, CastRayResult &crr );

	int checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion );

	SceneNode &getRootNode() { return *_nodes[_rootNodeId]; }
	
	SceneNode *resolveNodeHandle( NodeHandle handle );

   void clearQueryCache();

   void registerNodeTracker(SceneNode const* tracker, std::function<void(SceneNode const* updatedNode)>);
   void clearNodeTracker(SceneNode const* tracker);

   void shutdown();
   void initialize();
protected:
   NodeHandle nextNodeId();
	void _findNodes( SceneNode &startNode, std::string const& name, int type );
	NodeHandle parseNode( SceneNodeTpl &tpl, SceneNode *parent );
	void removeNodeRec( SceneNode &node );
	void castRayInternal( SceneNode &node, int userFlags );
   void fastCastRayInternal(int userFlags);
   void updateNodeTrackers(SceneNode const* n);
   void _queryNode(SceneNode& node, std::vector<QueryResult>& results);

protected:
   SceneId                        _sceneId;
   NodeHandle                     _rootNodeId;
   boost::container::flat_map<NodeHandle, SceneNode *>      _nodes;
	std::vector< SceneNode * >     _findResults;
	std::vector< CastRayResult >   _castRayResults;
	GridSpatialGraph*              _spatialGraph;

	std::map< int, NodeRegEntry >  _registry;  // Registry of node types

	Vec3f                          _rayOrigin;  // Don't put these values on the stack during recursive search
	Vec3f                          _rayDirection;  // Ditto
	int                            _rayNum;  // Ditto

   CachedQueryResult              _queryCache[QueryCacheSize];
   int                            _queryCacheCount;
   int                            _currentQuery;

   std::unordered_map<SceneNode const*, std::function<void(SceneNode const*)>> _nodeTrackers;

	friend class Renderer;
};

}
#endif // _egScene_H_
