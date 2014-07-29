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


const int RootNode = 1;
const int QueryCacheSize = 32;

class InstanceKey {
public:
   Resource* geoResource;
   MaterialResource* matResource;
   size_t hash;

   InstanceKey() {
      geoResource = nullptr;
      matResource = nullptr;
   }

   void updateHash() {
      hash = (uint32)(geoResource) ^ (uint32)(matResource);
   }

   bool operator==(const InstanceKey& other) const {
      return geoResource == other.geoResource &&
         matResource == other.matResource;
   }
   bool operator!=(const InstanceKey& other) const {
      return !(other == *this);
   }
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
      Bounds = 0x1,
      Flags = 0x2,
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
      ProjectorNode
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
		Selected = 0x8,
      NoCull = 0x10
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
   inline int getRenderStamp() const { return _renderStamp; }
   inline void setRenderStamp(int s) const { _renderStamp = s; }
	NodeHandle getHandle() const { return _handle; }
	SceneNode *getParent() const { return _parent; }
	std::string const& getName() const { return _name; }
	std::vector< SceneNode * > &getChildren() { return _children; }
	Matrix4f &getRelTrans() { return _relTrans; }
	Matrix4f &getAbsTrans() { return _absTrans; }
	const BoundingBox &getBBox() const { return _bBox; }
   void updateBBox(const BoundingBox& bbox);

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
   // Because a reverse-lookup is going to be more expensive to maintain.
   mutable int                 _renderStamp;
	NodeHandle                  _handle;
	uint32                      _flags;
   uint32                      _userFlags;
   uint32                      _accumulatedFlags;
	float                       _sortKey;
   InstanceKey                 _instanceKey;
	bool                        _dirty;  // Does the node need to be updated?
	bool                        _transformed;
	bool                        _renderable;
   bool                        _noInstancing;

	BoundingBox                 _bBox;  // AABB in world space

	std::vector< SceneNode * >  _children;  // Child nodes
	std::string                 _name;
	std::string                 _attachment;  // User defined data


private:

   friend class SceneManager;
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
	friend class SceneManager;

protected:
	GroupNode( const GroupNodeTpl &groupTpl );
};


// =================================================================================================
// Spatial Graph
// =================================================================================================

struct SpatialQuery
{
  Frustum frustum;
  const Frustum* secondaryFrustum;
  Vec3f camPos;
  RenderingOrder::List order;
  uint32 filterIgnore; 
  uint32 filterRequired;
  bool useRenderableQueue;
  bool useLightQueue;
  bool forceNoInstancing;
  uint32 userFlags;
};

struct RendQueueItem
{
	SceneNode  const*node;
	int        type;  // Type is stored explicitly for better cache efficiency when iterating over list
	float      sortKey;

	RendQueueItem() {}
	RendQueueItem( int type, float sortKey, SceneNode const*node ) : node( node ), type( type ), sortKey( sortKey ) {}
};

typedef std::vector<RendQueueItem> RenderableQueue;
typedef boost::container::flat_map<int, RenderableQueue > RenderableQueues;
typedef std::unordered_map<InstanceKey, RenderableQueue, hash_InstanceKey > InstanceRenderableQueue;
typedef boost::container::flat_map<int, InstanceRenderableQueue > InstanceRenderableQueues;

class SpatialGraph
{
public:
	SpatialGraph();
	
	void addNode(SceneNode const& sceneNode);
	void removeNode(SceneNode const& sceneNode);
	void updateNode(SceneNode const& sceneNode);

   void query(const SpatialQuery& query, RenderableQueues& renderableQueues, InstanceRenderableQueues& instanceQueues,
      std::vector<SceneNode const*>& lightQueue);

protected:
	std::unordered_map<NodeHandle, SceneNode const*>      _nodes;  // Renderable nodes and lights
};


struct GridElement 
{
   BoundingBox bounds;
   boost::container::flat_set<SceneNode const*> _nodes;
};

class GridSpatialGraph
{
public:
   GridSpatialGraph();

   void addNode(SceneNode const& sceneNode);
	void removeNode(SceneNode const& sceneNode);
	void updateNode(SceneNode const& sceneNode);

   void query(const SpatialQuery& query, RenderableQueues& renderableQueues, InstanceRenderableQueues& instanceQueues,
      std::vector<SceneNode const*>& lightQueue);
   void castRay(const Vec3f& rayOrigin, const Vec3f& rayDirection, std::function<void(boost::container::flat_set<SceneNode const*> const& nodes)> cb);

protected:
   void boundingBoxToGrids(BoundingBox const& aabb, boost::container::flat_set<uint32>& gridElementList) const;
   inline uint32 hashGridPoint(int x, int y) const;
   inline void unhashGridHash(uint32 hash, int* x, int* y) const;

   boost::container::flat_map<NodeHandle, boost::container::flat_set<uint32> > _nodeGridLookup;
   std::unordered_map<uint32, GridElement> _gridElements;
   std::unordered_map<NodeHandle, SceneNode const*> _directionalLights;
   std::unordered_map<NodeHandle, SceneNode const*> _nocullNodes;
   int _renderStamp;

   boost::container::flat_set<uint32> newGrids;
   std::vector<uint32> toRemove;
};


// =================================================================================================
// Scene Manager
// =================================================================================================

typedef SceneNodeTpl *(*NodeTypeParsingFunc)( std::map< std::string, std::string > &attribs );
typedef SceneNode *(*NodeTypeFactoryFunc)( const SceneNodeTpl &tpl );
typedef void (*NodeTypeRenderFunc)( std::string const& shaderContext, std::string const& theClass, bool debugView,
                                    const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                                    int occSet, int lodLevel );

struct NodeRegEntry
{
	std::string          typeString;
	NodeTypeParsingFunc  parsingFunc;
	NodeTypeFactoryFunc  factoryFunc;
	NodeTypeRenderFunc   renderFunc;
   NodeTypeRenderFunc   instanceRenderFunc;
   std::unordered_map<NodeHandle, SceneNode*> nodes;
};

struct CastRayResult
{
	SceneNode const* node;
	float      distance;
	Vec3f      intersection;
   Vec3f      normal;
};

struct SpatialQueryResult
{
   SpatialQuery query;
   RenderableQueues renderableQueues;
   InstanceRenderableQueues instanceRenderableQueues;
   std::vector<SceneNode const*> lightQueue;
};

// =================================================================================================

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

        void reset();

	void registerType( int type, std::string const& typeString, NodeTypeParsingFunc pf,
	                   NodeTypeFactoryFunc ff, NodeTypeRenderFunc rf, NodeTypeRenderFunc irf );
	NodeRegEntry *findType( int type );
	NodeRegEntry *findType( std::string const& typeString );
	
	void updateNodes();
	void updateSpatialNode( SceneNode const& node ) { _spatialGraph->updateNode( node ); }
	void updateQueues( const char* reason, const Frustum &frustum1, const Frustum *frustum2,
	                   RenderingOrder::List order, uint32 filterIgnore, uint32 filterRequired, bool lightQueue, bool renderableQueue, bool forceNoInstancing=false, 
                      uint32 userFlags=0 );
	
	NodeHandle addNode( SceneNode *node, SceneNode &parent );
	NodeHandle addNodes( SceneNode &parent, SceneGraphResource &sgRes );
	void removeNode( SceneNode &node );
	bool relocateNode( SceneNode &node, SceneNode &parent );
	
	int findNodes( SceneNode &startNode, std::string const& name, int type );
	SceneNode *getFindResult( int index ) { return (unsigned)index < _findResults.size() ? _findResults[index] : 0x0; }
	
	int castRay( SceneNode &node, const Vec3f &rayOrig, const Vec3f &rayDir, int numNearest, int userFlags );
	bool getCastRayResult( int index, CastRayResult &crr );

	int checkNodeVisibility( SceneNode &node, CameraNode &cam, bool checkOcclusion );

	SceneNode &getRootNode() { return *_nodes[RootNode]; }
	std::vector<SceneNode const*> &getLightQueue();
	RenderableQueue& getRenderableQueue(int itemType);
   InstanceRenderableQueue& getInstanceRenderableQueue(int itemType);
   RenderableQueues& getRenderableQueues();
	
	SceneNode *resolveNodeHandle( NodeHandle handle );

   void clearQueryCache();

protected:
	void _findNodes( SceneNode &startNode, std::string const& name, int type );
   int _checkQueryCache(const SpatialQuery& query);
	NodeHandle parseNode( SceneNodeTpl &tpl, SceneNode *parent );
	void removeNodeRec( SceneNode &node );
        void shutdown();
        void initialize();
	void castRayInternal( SceneNode &node, int userFlags );
   void fastCastRayInternal(int userFlags);

protected:
        uint32                         _nextNodeHandle;
   boost::container::flat_map<NodeHandle, SceneNode *>      _nodes;  // _nodes[0] is root node
	std::vector< SceneNode * >     _findResults;
	std::vector< CastRayResult >   _castRayResults;
	GridSpatialGraph*              _spatialGraph;

	std::map< int, NodeRegEntry >  _registry;  // Registry of node types

	Vec3f                          _rayOrigin;  // Don't put these values on the stack during recursive search
	Vec3f                          _rayDirection;  // Ditto
	int                            _rayNum;  // Ditto

   SpatialQueryResult             _queryCache[QueryCacheSize];
   int                            _queryCacheCount;
   int                            _currentQuery;

	friend class Renderer;
};

}
#endif // _egScene_H_
