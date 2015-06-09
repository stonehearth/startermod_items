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

#ifndef _egRenderer_H_
#define _egRenderer_H_

#include "egPrerequisites.h"
#include "egRendererBase.h"
#include "egPrimitives.h"
#include "egModel.h"
#include <vector>
#include <algorithm>
#include <unordered_map>

struct glslopt_ctx;

namespace Horde3D {

class MaterialResource;
class LightNode;
class CameraNode;
class VoxelMeshNode;

const uint32 MaxNumOverlayVerts = (1 << 16); // about 32k..
const uint32 ParticlesPerBatch = 64;	// Warning: The GPU must have enough registers
const uint32 QuadIndexBufCount = MaxNumOverlayVerts * 6;
const uint32 MaxVoxelInstanceCount = 1024;

extern const char *vsDefColor;
extern const char *fsDefColor;
extern const char *vsOccBox;
extern const char *fsOccBox;	
	

// =================================================================================================
// Renderer
// =================================================================================================

struct OverlayBatch
{
	PMaterialResource  materialRes;
	uint32             firstVert, vertCount;
	float              colRGBA[4];
	int                flags;
	
	OverlayBatch() {}

	OverlayBatch( uint32 firstVert, uint32 vertCount, float *col, MaterialResource *materialRes, int flags ) :
		materialRes( materialRes ), firstVert( firstVert ), vertCount( vertCount ), flags( flags )
	{
		colRGBA[0] = col[0]; colRGBA[1] = col[1]; colRGBA[2] = col[2]; colRGBA[3] = col[3];
	}
 };

struct OverlayVert
{
   OverlayVert() {}
   OverlayVert(float x, float y, float u, float v) :
      x(x), y(y), u(u), v(v) {};

	float  x, y;  // Position
	float  u, v;  // Texture coordinates
};


struct ParticleVert
{
	float  u, v;         // Texture coordinates
	float  index;        // Index in property array

	ParticleVert() {}

	ParticleVert( float u, float v ):
		u( u ), v( v ), index( 0 )
	{
	}
};

struct CubeVert
{
   float x, y, z;       // Object-space location.

	CubeVert() {}

	CubeVert( float x, float y, float z ):
		x( x ), y( y ), z( z )
	{
	}
};

struct CubeBatchVert
{
   float x, y, z;       // Object-space location.
	float  index;        // Index in property array

	CubeBatchVert() {}

	CubeBatchVert( float x, float y, float z, float index ):
		x( x ), y( y ), z( z ), index(index)
	{
	}
};

// =================================================================================================

struct OccProxy
{
	Vec3f   bbMin, bbMax;
	uint32  queryObj;

	OccProxy() {}
	OccProxy( const Vec3f &bbMin, const Vec3f &bbMax, uint32 queryObj ) :
		bbMin( bbMin ), bbMax( bbMax ), queryObj( queryObj )
	{
	}
};

struct PipeSamplerBinding
{
	char    sampler[64];
	uint32  rbObj;
	uint32  bufIndex;
};

struct GpuCompatibility {
   bool canDoOmniShadows;
   bool canDoShadows;
};

struct EngineRendererCaps
{
   bool HighQualityRendererSupported;
   bool ShadowsSupported;
};

struct UniformType
{
   enum List
   {
      FLOAT = 1,
      VEC4  = 2,
      MAT44 = 3,
      VEC3 = 4,
      MAT44_ARRAY = 5
   };
};


struct SelectedNode
{
   SelectedNode(NodeHandle n, Vec3f c) : h(n), color(c) {}
   NodeHandle h;
   Vec3f color;
};


class Renderer
{
public:
	Renderer();
	~Renderer();
	
	unsigned char *useScratchBuf( uint32 minSize );
	
	bool init(int glMajor, int glMinor, bool msaaWindowSupported, bool enable_gl_logging);
	void initStates();

   void collectOneDebugFrame();
	void drawAABB( const Vec3f &bbMin, const Vec3f &bbMax );
	void drawSphere( const Vec3f &pos, float radius );
	void drawCone( float height, float fov, const Matrix4f &transMat );
   void drawFrustum(const Frustum& frust);
   void drawPoly(const std::vector<Vec3f>& poly);

	bool createShaderComb( const char* filename, const char *vertexShader, const char *fragmentShader, ShaderCombination &sc );
	void releaseShaderComb( ShaderCombination &sc );
	void setShaderComb( ShaderCombination *sc );
	void commitGeneralUniforms();
   void commitGlobalUniforms();
	bool setMaterial( MaterialResource *materialRes, std::string const& shaderContext );

   void addSelectedNode(NodeHandle n, float r, float g, float b);
   void removeSelectedNode(NodeHandle n);
	
   uint32 calculateShadowBufferSize(LightNode const* node) const;
   void reallocateShadowBuffers(int quality);

	int registerOccSet();
	void unregisterOccSet( int occSet );
	void drawOccProxies( uint32 list );
	void pushOccProxy( uint32 list, const Vec3f &bbMin, const Vec3f &bbMax, uint32 queryObj )
		{ _occProxies[list].push_back( OccProxy( bbMin, bbMax, queryObj ) ); }
	
	void showOverlays( const float *verts, uint32 vertCount, float *colRGBA,
	                   MaterialResource *matRes, int flags );
	void clearOverlays();
	
	static void drawMeshes(SceneId sceneId, std::string const& shaderContext, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
	static void drawVoxelMeshes(SceneId sceneId, std::string const& shaderContext, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
	static void drawVoxelMeshes_Instances(SceneId sceneId, std::string const& shaderContext, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
	static void drawParticles(SceneId sceneId, std::string const& shaderContext, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
   static void drawHudElements(SceneId sceneId, std::string const& shaderContext, bool debugView,
      const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
   static void drawInstanceNode(SceneId sceneId, std::string const& shaderContext, bool debugView,
      const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );

   void render( CameraNode *camNode, PipelineResource *pRes );
	void finalizeFrame();

	uint32 getFrameID() { return _frameID; }
	ShaderCombination *getCurShader() { return _curShader; }
	CameraNode *getCurCamera() const { return _curCamera; }
	uint32 getQuadIdxBuf() { return _quadIdxBuf; }
	uint32 getParticleVBO() { return _particleVBO; }
   uint32 getShadowCascadeBuffer() const { return _shadowCascadeBuffer; }

   uint32 getClipspaceLayout() { return _vlClipspace; }
   uint32 getPosColTexLayout() { return _vlPosColTex; }

   void setCurrentTime(float time) { _currentTime = time; }

   void getEngineCapabilities(EngineRendererCaps* rendererCaps, EngineGpuCaps* gpuCaps) const;

   void setGlobalUniform(const char* uniName, UniformType::List kind, void const* value, int num=1);

protected:
   ShaderCombination* findShaderCombination(ShaderResource* sr) const;
   bool isShaderContextSwitch(std::string const& curContext, MaterialResource *materialRes) const;

   void setupViewMatrices( const Matrix4f &viewMat, const Matrix4f &projMat );
	
	void createPrimitives();
	
	bool setMaterialRec(MaterialResource *materialRes, std::string const& shaderContext);
   void updateLodUniform(int lodLevel, float lodDist1, float lodDist2);
	
   void commitLightUniforms(LightNode const* light);
   void setupShadowMap(LightNode const* light, bool noShadows);
   Matrix4f calcCropMatrix(SceneId sceneId, const Frustum &frustSlice, Vec3f const& lightPos, const Matrix4f &lightViewProjMat );
   Matrix4f calcDirectionalLightShadowProj(LightNode const* light, BoundingBox const& worldBounds, Frustum const& frustSlice, Matrix4f const& lightViewMat) const;
   void computeLightFrustumNearFar(const BoundingBox& worldBounds, const Matrix4f& lightViewMat, const Vec3f& lightMin, const Vec3f& lightMax, float* nearV, float* farV) const;
   void computeTightCameraBounds(SceneId sceneId, float* minDist, float* maxDist);
   Frustum computeDirectionalLightFrustum(LightNode const* light, float nearPlaneDist, float farPlaneDist) const;
   void quantizeShadowFrustum(const Frustum& frustSlice, int shadowMapSize, Vec3f* min, Vec3f* max);
   void updateShadowMap(LightNode const* light, Frustum const* lightFrus, float minDist, float maxDist, int cubeFace=0);

	void drawOverlays( std::string const& shaderContext );

	void bindPipeBuffer( uint32 rbObj, std::string const& sampler, uint32 bufIndex );
	void clear( bool depth, bool buf0, bool buf1, bool buf2, bool buf3, float r, float g, float b, float a, int stencilVal );
	void drawFSQuad( Resource *matRes, std::string const& shaderContext );
	void drawMatSphere(Resource *matRes, std::string const& shaderContext, const Vec3f &pos, float radius);
	void drawQuad();
   void drawLodGeometry(SceneId sceneId, std::string const& shaderContext,
                         RenderingOrder::List order, int filterRequried, int occSet, float frustStart, float frustEnd, int lodLevel, Frustum const* lightFrus=0x0);
   void drawGeometry(SceneId sceneId, std::string const& shaderContext,
	                   RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd, int forceLodLevel=-1, Frustum const* lightFrus=0x0);
   void drawSelected(SceneId sceneId, std::string const& shaderContext,
	                   RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd, int forceLodLevel=-1, Frustum const* lightFrus=0x0);
   void drawProjections(SceneId sceneId, std::string const& shaderContext, uint32 userFlags );
   void prioritizeLights(SceneId sceneId, std::vector<LightNode*> *lights);
	void doForwardLightPass(SceneId sceneId, std::string const& contextSuffix,
	                        bool noShadows, RenderingOrder::List order, int occSet, bool selectedOnly, int lodLevel=-1 );
	void doDeferredLightPass(SceneId sceneId, bool noShadows, MaterialResource* deferredMaterial);
	
	void drawRenderables(SceneId sceneId, std::string const& shaderContext, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet, int lodLevel );
   static uint32 acquireInstanceAttributeBuffer();
   static void drawVoxelMesh_Instances_WithInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn, int lodLevel);
   static void drawVoxelMesh_Instances_WithoutInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn, int lodLevel);
	void renderDebugView();
	void finishRendering();
   void logPerformanceData();
   void setGpuCompatibility();

protected:
	unsigned char                      *_scratchBuf;
	uint32                             _scratchBufSize;

	Matrix4f                           _viewMat, _viewMatInv, _projMat, _viewProjMat, _viewProjMatInv, _projectorMat;
	float                              _currentTime;
   MaterialResource*                  _materialOverride;

	std::vector< PipeSamplerBinding >  _pipeSamplerBindings;
	std::vector< char >                _occSets;  // Actually bool
	std::vector< OccProxy >            _occProxies[2];  // 0: renderables, 1: lights

   std::unordered_map<std::string, float> _uniformFloats;
   std::unordered_map<std::string, Vec4f> _uniformVecs;
   std::unordered_map<std::string, Vec3f> _uniformVec3s;
   std::unordered_map<std::string, Matrix4f> _uniformMats;
   std::unordered_map<std::string, std::vector<float>> _uniformMatArrays;

   std::vector<SelectedNode>          _selectedNodes;
	
	std::vector< OverlayBatch >        _overlayBatches;
	OverlayVert                        *_overlayVerts;
	uint32                             _overlayVB;
	
	uint32                             _frameID;
	uint32                             _defShadowMap;
	uint32                             _quadIdxBuf;
	uint32                             _particleVBO;
   uint32                             _shadowCascadeBuffer;

   PipelineResource                   *_curPipeline;
	MaterialResource                   *_curStageMatLink;
	CameraNode                         *_curCamera;
	ShaderCombination                  *_curShader;
	RenderTarget                       *_curRenderTarget;
	uint32                             _curShaderUpdateStamp;
	
	uint32                             _maxAnisoMask;
	float                              _splitPlanes[5];
	Matrix4f                           _lightMats[4];
   Vec4f                              _lodValues;

	uint32                             _vlPosOnly, _vlOverlay, _vlModel, _vlParticle, _vlVoxelModel, _vlClipspace, _vlPosColTex, _vlInstanceVoxelModel;
	uint32                             _vbCube, _ibCube, _vbSphere, _ibSphere;
	uint32                             _vbCone, _ibCone, _vbFSPoly;
   uint32                             _vbFrust, _vbPoly, _ibPoly;

   static float*                      _vbInstanceVoxelBuf;
   static std::unordered_map<RenderableQueue const*, uint32> _instanceDataCache;
   static Matrix4f                    _defaultBoneMat;


   // Feature-level compatibility of the card, determined by GPU specifics.
   GpuCompatibility                    gpuCompatibility_;

   float                              _lod_polygon_offset_x;
   float                              _lod_polygon_offset_y;

public:
   // needed to draw debug shapes in extensions!
   glslopt_ctx*                       _glsl_opt_ctx;
	ShaderCombination                  _defColorShader;
	int                                _defColShader_color;  // Uniform location
};

}
#endif // _egRenderer_H_
