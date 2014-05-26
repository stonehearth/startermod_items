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


namespace Horde3D {

struct SceneNodeTpl;
class CameraNode;
class SceneGraphResource;


const int RootNode = 1;
const int QueryCacheSize = 32;

struct InstanceKey {
   Resource* geoResource;
   MaterialResource* matResource;

   bool operator==(const InstanceKey& other) const {
      return geoResource == other.geoResource &&
         matResource == other.matResource;
   }
   bool operator!=(const InstanceKey& other) const {
      return !(other == *this);
   }
};

struct hash_InstanceKey {
   size_t operator()(const InstanceKey& x) const {
      return (uint32)(x.geoResource) ^ (uint32)(x.matResource);
   }
};

// =================================================================================================
// Scene Node
// =================================================================================================

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

	int getFlags() { return _flags; }
   void setFlags( int flags, bool recursive );
   void twiddleFlags( int flags, bool on, bool recursive );

	virtual int getParamI( int param );
	virtual void setParamI( int param, int value );
	virtual float getParamF( int param, int compIdx );
	virtual void setParamF( int param, int compIdx, float value );
	virtual const char *getParamStr( int param );
	virtual void setParamStr( int param, const char* value );
   virtual void* mapParamV( int param );
   virtual void unmapParamV( int param, int mappedLength );

	virtual void setLodLevel( int lodLevel );

	virtual bool canAttach( SceneNode &parent );
	void markDirty();
	void update();
	virtual bool checkIntersection( const Vec3f &rayOrig, const Vec3f &rayDir, Vec3f &intsPos, Vec3f &intsNorm ) const;

   virtual const InstanceKey* getInstanceKey() { return 0x0; }
	int getType() { return _type; };
	NodeHandle getHandle() { return _handle; }
	SceneNode *getParent() { return _parent; }
	std::string const& getName() { return _name; }
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
	void markChildrenDirty();

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
	uint32                      _flags;
   uint32                      _userFlags;
	float                       _sortKey;
	bool                        _dirty;  // Does the node need to be updated?
	bool                        _transformed;
	bool                        _renderable;

	BoundingBox                 _bBox;  // AABB in world space

	std::vector< SceneNode * >  _children;  // Child nodes
	std::string                 _name;
	std::string                 _attachment;  // User defined data

	friend class SceneManager;
	friend class SpatialGraph;
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
	SceneNode  *node;
	int        type;  // Type is stored explicitly for better cache efficiency when iterating over list
	float      sortKey;

	RendQueueItem() {}
	RendQueueItem( int type, float sortKey, SceneNode *node ) : node( node ), type( type ), sortKey( sortKey ) {}
};

typedef std::vector<RendQueueItem> RenderableQueue;
typedef std::unordered_map<int, RenderableQueue > RenderableQueues;
typedef std::unordered_map<InstanceKey, RenderableQueue, hash_InstanceKey > InstanceRenderableQueue;
typedef std::unordered_map<int, InstanceRenderableQueue > InstanceRenderableQueues;

class SpatialGraph
{
public:
	SpatialGraph();
	
	void addNode( SceneNode &sceneNode );
	void removeNode( uint32 sgHandle );
	void updateNode( uint32 sgHandle );

   void query(const SpatialQuery& query, RenderableQueues& renderableQueues, InstanceRenderableQueues& instanceQueues,
      std::vector<SceneNode*>& lightQueue);

protected:
	std::map<NodeHandle, SceneNode *>      _nodes;  // Renderable nodes and lights
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
};

struct CastRayResult
{
	SceneNode  *node;
	float      distance;
	Vec3f      intersection;
   Vec3f      normal;
};

struct SpatialQueryResult
{
   SpatialQuery query;
   RenderableQueues renderableQueues;
   InstanceRenderableQueues instanceRenderableQueues;
   std::vector<SceneNode*> lightQueue;
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
	void updateSpatialNode( uint32 sgHandle ) { _spatialGraph->updateNode( sgHandle ); }
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
	std::vector< SceneNode * > &getLightQueue();
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

protected:
        uint32                         _nextNodeHandle;
	std::unordered_map<NodeHandle, SceneNode *>      _nodes;  // _nodes[0] is root node
	std::vector< SceneNode * >     _findResults;
	std::vector< CastRayResult >   _castRayResults;
	SpatialGraph                   *_spatialGraph;

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
