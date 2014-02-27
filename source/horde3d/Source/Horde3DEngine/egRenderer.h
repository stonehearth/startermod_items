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
struct ShaderContext;

const uint32 MaxNumOverlayVerts = (1 << 16); // about 32k..
const uint32 ParticlesPerBatch = 64;	// Warning: The GPU must have enough registers
const uint32 QuadIndexBufCount = MaxNumOverlayVerts * 6;
const uint32 MaxVoxelInstanceCount = 10000;

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
   bool canDoShadows;
   bool canDoSsao;
};

struct EngineRendererCaps
{
   bool ShadowsSupported;
   bool SsaoSupported;
};

struct UniformType
{
   enum List
   {
      FLOAT = 1,
      VEC4  = 2,
      MAT44 = 3
   };
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
	bool setMaterial( MaterialResource *materialRes, const std::string &shaderContext );
	
	bool createShadowRB( uint32 width, uint32 height );
	void releaseShadowRB();

	int registerOccSet();
	void unregisterOccSet( int occSet );
	void drawOccProxies( uint32 list );
	void pushOccProxy( uint32 list, const Vec3f &bbMin, const Vec3f &bbMax, uint32 queryObj )
		{ _occProxies[list].push_back( OccProxy( bbMin, bbMax, queryObj ) ); }
	
	void showOverlays( const float *verts, uint32 vertCount, float *colRGBA,
	                   MaterialResource *matRes, int flags );
	void clearOverlays();
	
	static void drawMeshes( const std::string &shaderContext, const std::string &theClass, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet );
	static void drawVoxelMeshes( const std::string &shaderContext, const std::string &theClass, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet );
	static void drawVoxelMeshes_Instances( const std::string &shaderContext, const std::string &theClass, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet );
	static void drawParticles( const std::string &shaderContext, const std::string &theClass, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet );
   static void drawHudElements(const std::string &shaderContext, const std::string &theClass, bool debugView,
      const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);
   static void drawInstanceNode(const std::string &shaderContext, const std::string &theClass, bool debugView,
      const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet);

   void render( CameraNode *camNode, PipelineResource *pRes );
	void finalizeFrame();

	uint32 getFrameID() { return _frameID; }
	ShaderCombination *getCurShader() { return _curShader; }
	CameraNode *getCurCamera() const { return _curCamera; }
	uint32 getQuadIdxBuf() { return _quadIdxBuf; }
	uint32 getParticleVBO() { return _particleVBO; }

   uint32 getClipspaceLayout() { return _vlClipspace; }
   uint32 getPosColTexLayout() { return _vlPosColTex; }

   void setCurrentTime(float time) { _currentTime = time; }
   uint32 getShadowRendBuf() const { return _shadowRB; }

   void getEngineCapabilities(EngineRendererCaps* rendererCaps, EngineGpuCaps* gpuCaps) const;

   void setGlobalUniform(const char* uniName, UniformType::List kind, void* value);

protected:
   ShaderCombination* findShaderCombination(ShaderResource* r, ShaderContext* context) const;
   bool isShaderContextSwitch(const std::string &curContext, const MaterialResource *materialRes);

   void setupViewMatrices( const Matrix4f &viewMat, const Matrix4f &projMat );
	
	void createPrimitives();
	
	bool setMaterialRec( MaterialResource *materialRes, const std::string &shaderContext, ShaderResource *shaderRes );
	
	void setupShadowMap( bool noShadows );
	Matrix4f calcCropMatrix( const Frustum &frustSlice, const Vec3f lightPos, const Matrix4f &lightViewProjMat );
   Matrix4f calcDirectionalLightShadowProj(const BoundingBox& worldBounds, const Frustum& frustSlice, const Matrix4f& lightViewMat, int numShadowMaps);
   void computeLightFrustumNearFar(const BoundingBox& worldBounds, const Matrix4f& lightViewMat, const Vec3f& lightMin, const Vec3f& lightMax, float* nearV, float* farV);
   void computeTightCameraBounds(float* minDist, float* maxDist);
   Frustum computeDirectionalLightFrustum(float nearPlaneDist, float farPlaneDist);
   void quantizeShadowFrustum(const Frustum& frustSlice, int shadowMapSize, Vec3f* min, Vec3f* max);
   void updateShadowMap(const Frustum* lightFrus, float minDist, float maxDist);

	void drawOverlays( const std::string &shaderContext );

	void bindPipeBuffer( uint32 rbObj, const std::string &sampler, uint32 bufIndex );
	void clear( bool depth, bool buf0, bool buf1, bool buf2, bool buf3, float r, float g, float b, float a, int stencilVal );
	void drawFSQuad( Resource *matRes, const std::string &shaderContext );
	void drawGeometry( const std::string &shaderContext, const std::string &theClass,
	                   RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd );
   void drawProjections(const std::string &shaderContext, uint32 userFlags );
	void drawLightGeometry( const std::string &shaderContext, const std::string &theClass,
	                        bool noShadows, RenderingOrder::List order, int occSet, bool selectedOnly );
	void drawLightShapes( const std::string &shaderContext, bool noShadows, int occSet );
	
	void drawRenderables( const std::string &shaderContext, const std::string &theClass, bool debugView,
		const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order, int occSet );
   static void drawVoxelMesh_Instances_WithInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn);
   static void drawVoxelMesh_Instances_WithoutInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn);
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
   std::unordered_map<std::string, Matrix4f> _uniformMats;
	
	std::vector< OverlayBatch >        _overlayBatches;
	OverlayVert                        *_overlayVerts;
	uint32                             _overlayVB;
	
	uint32                             _shadowRB;
	uint32                             _frameID;
	uint32                             _defShadowMap;
	uint32                             _quadIdxBuf;
	uint32                             _particleVBO;

   PipelineResource                   *_curPipeline;
	MaterialResource                   *_curStageMatLink;
	CameraNode                         *_curCamera;
	LightNode                          *_curLight;
	ShaderCombination                  *_curShader;
	RenderTarget                       *_curRenderTarget;
	uint32                             _curShaderUpdateStamp;
	
	uint32                             _maxAnisoMask;
	float                              _smSize;
	float                              _splitPlanes[5];
	Matrix4f                           _lightMats[4];

	uint32                             _vlPosOnly, _vlOverlay, _vlModel, _vlParticle, _vlVoxelModel, _vlClipspace, _vlPosColTex, _vlInstanceVoxelModel;
	uint32                             _vbCube, _ibCube, _vbSphere, _ibSphere;
	uint32                             _vbCone, _ibCone, _vbFSPoly;
   uint32                             _vbFrust, _vbPoly, _ibPoly;

   static uint32                      _vbInstanceVoxelData;
   static float*                      _vbInstanceVoxelBuf;

   // Feature-level compatibility of the card, determined by GPU specifics.
   GpuCompatibility                    gpuCompatibility_;
public:
   // needed to draw debug shapes in extensions!
   glslopt_ctx*                       _glsl_opt_ctx;
	ShaderCombination                  _defColorShader;
	int                                _defColShader_color;  // Uniform location
};

}
#endif // _egRenderer_H_
