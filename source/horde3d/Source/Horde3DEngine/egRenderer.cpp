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
#include "lib/perfmon/perfmon.h"

#include "egRenderer.h"
#include "utOpenGL.h"
#include "egParticle.h"
#include "egLight.h"
#include "egCamera.h"
#include "egHudElement.h"
#include "egInstanceNode.h"
#include "egModules.h"
#include "egProjectorNode.h"
#include "egVoxelGeometry.h"
#include "egVoxelModel.h"
#include "egCom.h"
#include <cstring>
#include <sstream>
#include <regex>
#if defined(OPTIMIZE_GSLS)
#include <glsl/glsl_optimizer.h>
#endif
#include "utDebug.h"

#define R_LOG(level)       LOG(horde.renderer, level)
#define MAT_LOG(level)     LOG(horde.material, level)

namespace Horde3D {

using namespace std;


const char *vsDefColor =
	"uniform mat4 viewProjMat;\n"
	"uniform mat4 worldMat;\n"
   "uniform float modelScale;\n"
	"attribute vec3 vertPos;\n"
   "#ifdef DRAW_SKINNED\n"
   "attribute float boneIndex;\n"
   "uniform mat4 bones[48];\n"
   "#endif\n"

	"void main() {\n"

   "mat4 final;\n"
   "#ifdef DRAW_SKINNED\n"
	"int idx = int(boneIndex);\n"
	"final = worldMat * bones[idx];\n"
   "#else\n"
 	"final = worldMat;\n"
   "#endif\n"
	"gl_Position = viewProjMat * final * vec4( vertPos * modelScale, 1.0 );\n"
	"}\n";

const char *fsDefColor =
	"uniform vec4 color;\n"
	"void main() {\n"
	"	gl_FragColor = color;\n"
	"}\n";

float* Renderer::_vbInstanceVoxelBuf;
std::unordered_map<RenderableQueue const*, uint32> Renderer::_instanceDataCache;
Matrix4f Renderer::_defaultBoneMat;

Renderer::Renderer()
{
	_scratchBuf = 0x0;
	_overlayVerts = 0x0;
	_scratchBufSize = 0;
	_frameID = 1;
	_defShadowMap = 0;
	_quadIdxBuf = 0;
	_particleVBO = 0;
	_curCamera = 0x0;
	_curShader = 0x0;
	_curRenderTarget = 0x0;
	_curShaderUpdateStamp = 1;
	_maxAnisoMask = 0;
   _materialOverride = 0x0;
   _curPipeline = 0x0;
   _shadowCascadeBuffer = 0;

	_vlPosOnly = 0;
	_vlOverlay = 0;
	_vlModel = 0;
   _vlVoxelModel = 0;
   _vlInstanceVoxelModel = 0;
	_vlParticle = 0;
   bool openglES = false;
   _lod_polygon_offset_x = 0.0f;
   _lod_polygon_offset_y = 0.0f;
   _defaultBoneMat.toIdentity();
#if defined(OPTIMIZE_GSLS)
   _glsl_opt_ctx = glslopt_initialize(openglES);
#endif
}


Renderer::~Renderer()
{
   if (_shadowCascadeBuffer) {
      gRDI->destroyRenderBuffer(_shadowCascadeBuffer);
      _shadowCascadeBuffer = 0;
   }
	gRDI->destroyTexture( _defShadowMap );
	gRDI->destroyBuffer( _particleVBO );
   delete[] _vbInstanceVoxelBuf;
	releaseShaderComb( _defColorShader );
#if defined(OPTIMIZE_GSLS)
   glslopt_cleanup(_glsl_opt_ctx);
#endif
	delete[] _scratchBuf;
	delete[] _overlayVerts;
}


// =================================================================================================
// Basic Initialization and Setup
// =================================================================================================

unsigned char *Renderer::useScratchBuf( uint32 minSize )
{
	if( _scratchBufSize < minSize )
	{
		delete[] _scratchBuf;
		_scratchBuf = new unsigned char[minSize + 15];
		_scratchBufSize = minSize;
	}

	return _scratchBuf + (size_t)_scratchBuf % 16;  // 16 byte aligned
}


void Renderer::setGpuCompatibility()
{
   CardType gpu = gRDI->getCaps().cardType;
   int glVer = gRDI->getCaps().glVersion;

   // Shadows: we don't trust intel less than gl version 30.
   // TODO: Eventually, we get to a nice big matrix of manufacturer/driver.
   gpuCompatibility_.canDoShadows = !(gpu == CardType::INTEL && glVer < 30);

   gpuCompatibility_.canDoOmniShadows = gpuCompatibility_.canDoShadows && gRDI->getCaps().texFloat;
}

void Renderer::getEngineCapabilities(EngineRendererCaps* rendererCaps, EngineGpuCaps* gpuCaps) const
{
   if (rendererCaps) {
      // It is _possible_ (apparently?  According to Horde guys?  Harumph.) that, even with GL3.x support claimed, we won't get
      // the stencil buffer we deserve, so be doubly paranoid and check that that is our render target depth format.
      rendererCaps->HighQualityRendererSupported = gRDI->_depthFormat == GL_DEPTH24_STENCIL8 && gRDI->getCaps().glVersion >= 30;

      // The following vendor/driver combinations break some aspect of the HQ renderer.

      // These combos don't like const vec3 arrays in GLSL (supported in GLSL since 3.0!)
      if (strstr(gRDI->getCaps().vendor, "Intel") != nullptr) {
         if (gRDI->getCaps().glVersion < 41) {
            // If we're seeing an early OGL driver, don't even bother.
            rendererCaps->HighQualityRendererSupported = false;
         } else {
            std::cmatch cm;
            std::regex e("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
            std::regex_search(gRDI->getCaps().version, cm, e);

            if (cm.size() == 5) {
               const int lastBadDriver[] = {10, 18, 14, 4156};  // 10.18.14.4156
               rendererCaps->HighQualityRendererSupported = false;
               for (int i = 1; i < 5; i++) {
                  int v = atoi(cm[i].str().c_str());
                  if (v > lastBadDriver[i - 1]) {
                     rendererCaps->HighQualityRendererSupported = true;
                     break;
                  } else if (v < lastBadDriver[i - 1]) {
                     break;
                  }
               }
            }
         }
      }

      rendererCaps->ShadowsSupported = gpuCompatibility_.canDoShadows;
   }

   if (gpuCaps) {
      gpuCaps->MSAASupported = gRDI->getCaps().rtMultisampling;
   }
}


bool Renderer::init(int glMajor, int glMinor, bool msaaWindowSupported, bool enable_gl_logging)
{
	// Init Render Device Interface
	if( !gRDI->init(glMajor, glMinor, msaaWindowSupported, enable_gl_logging) ) return false;

   setGpuCompatibility();

   if (!gpuCompatibility_.canDoShadows)
   {
      Modules::log().writeWarning("Renderer: disabling shadows.");
   }

   if (!gpuCompatibility_.canDoOmniShadows)
   {
      Modules::log().writeWarning("Renderer: disabling omni-directional shadows.");
   }

   // Check capabilities
	if( !gRDI->getCaps().texFloat )
		Modules::log().writeWarning( "Renderer: No floating point texture support available" );
	if( !gRDI->getCaps().texNPOT )
		Modules::log().writeWarning( "Renderer: No non-Power-of-two texture support available" );
	if( !gRDI->getCaps().rtMultisampling )
		Modules::log().writeWarning( "Renderer: No multisampling for render targets available" );

   // TODO(klochek): expose flags as part of an interface; h3dGetSupportedShaderFlags(), or something?
   Modules::config().setGlobalShaderFlag("INSTANCE_SUPPORT", gRDI->getCaps().hasInstancing);
	
	// Create vertex layouts
	VertexLayoutAttrib attribsPosOnly[1] = {
		"vertPos", 0, 3, 0
	};
	_vlPosOnly = gRDI->registerVertexLayout( 1, attribsPosOnly );

	VertexLayoutAttrib attribsOverlay[2] = {
		"vertPos", 0, 2, 0,
		"texCoords0", 0, 2, 8
	};
	_vlOverlay = gRDI->registerVertexLayout( 2, attribsOverlay );
	
	VertexLayoutAttrib attribsModel[7] = {
		"vertPos", 0, 3, 0,
		"normal", 1, 3, 0,
		"tangent", 2, 4, 0,
		"joints", 3, 4, 8,
		"weights", 3, 4, 24,
		"texCoords0", 3, 2, 0,
		"texCoords1", 3, 2, 40
	};
	_vlModel = gRDI->registerVertexLayout( 7, attribsModel );

	VertexLayoutAttrib attribsVoxelModel[4] = {
      { "vertPos",     0, 3, 0 }, 
      { "boneIndex",   0, 1, 12 },
      { "normal",      0, 3, 16 },
      { "color",       0, 4, 28 },
	};
	_vlVoxelModel = gRDI->registerVertexLayout( 4, attribsVoxelModel );

	VertexLayoutAttrib attribsInstanceVoxelModel[5] = {
      { "vertPos",     0, 3, 0 }, 
      { "boneIndex",   0, 1, 12 },
      { "normal",      0, 3, 16 },
      { "color",       0, 4, 28 },
      { "transform",   1, 16, 0 }
	};

   VertexDivisorAttrib divisorsInstanceVoxelModel[5] = {
      0, 0, 0, 0, 1
   };
	_vlInstanceVoxelModel = gRDI->registerVertexLayout( 5, attribsInstanceVoxelModel, divisorsInstanceVoxelModel );

   _vbInstanceVoxelBuf = new float[MaxVoxelInstanceCount * (4 * 4)];

   VertexLayoutAttrib attribsParticle[2] = {
		"texCoords0", 0, 2, 0,
		"parIdx", 0, 1, 8
	};
	_vlParticle = gRDI->registerVertexLayout( 2, attribsParticle );
	
   VertexLayoutAttrib attribsClipspace[3] = {
      {"vertPos", 0, 4, 0},
      {"texCoords0", 0, 2, 16},
      {"color", 0, 4, 24}
   };
   _vlClipspace = gRDI->registerVertexLayout( 3, attribsClipspace);

   VertexLayoutAttrib attribsPosColTex[3] = {
      {"vertPos", 0, 3, 0},
      {"texCoords0", 0, 2, 12},
      {"color", 0, 4, 20}
   };
   _vlPosColTex = gRDI->registerVertexLayout( 3, attribsPosColTex);

   // Upload default shaders
	if( !createShaderComb( NULL, vsDefColor, fsDefColor, _defColorShader ) )
	{
		Modules::log().writeError( "Failed to compile default shaders" );
		return false;
	}

	// Cache common uniforms
	_defColShader_color = gRDI->getShaderConstLoc( _defColorShader.shaderObj, "color" );
	
	// Create default shadow map
	float shadowTex[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	_defShadowMap = gRDI->createTexture( TextureTypes::Tex2D, 4, 4, 1, TextureFormats::DEPTH, false, false, false, false );
	gRDI->uploadTextureData( _defShadowMap, 0, 0, shadowTex );

	// Create index buffer used for drawing quads
	uint16 *quadIndices = new uint16[QuadIndexBufCount];
	for( uint32 i = 0; i < QuadIndexBufCount / 6; ++i )
	{
		quadIndices[i*6+0] = i * 4 + 0; quadIndices[i*6+1] = i * 4 + 1; quadIndices[i*6+2] = i * 4 + 2;
		quadIndices[i*6+3] = i * 4 + 2; quadIndices[i*6+4] = i * 4 + 3; quadIndices[i*6+5] = i * 4 + 0;
	}
	_quadIdxBuf = gRDI->createIndexBuffer( QuadIndexBufCount * sizeof( uint16 ), quadIndices );
	delete[] quadIndices; quadIndices = 0x0;
	
	// Create particle geometry array
	ParticleVert v0( 0, 0 );
	ParticleVert v1( 1, 0 );
	ParticleVert v2( 1, 1 );
	ParticleVert v3( 0, 1 );
	
	ParticleVert *parVerts = new ParticleVert[ParticlesPerBatch * 4];
	for( uint32 i = 0; i < ParticlesPerBatch; ++i )
	{
		parVerts[i * 4 + 0] = v0; parVerts[i * 4 + 0].index = (float)i;
		parVerts[i * 4 + 1] = v1; parVerts[i * 4 + 1].index = (float)i;
		parVerts[i * 4 + 2] = v2; parVerts[i * 4 + 2].index = (float)i;
		parVerts[i * 4 + 3] = v3; parVerts[i * 4 + 3].index = (float)i;
	}
	_particleVBO = gRDI->createVertexBuffer( ParticlesPerBatch * 4 * sizeof( ParticleVert ), STREAM, (float *)parVerts );
	delete[] parVerts; parVerts = 0x0;

   // Create overlay geometry array;
   _overlayBatches.reserve( 64 );
	_overlayVerts = new OverlayVert[MaxNumOverlayVerts];
	_overlayVB = gRDI->createVertexBuffer( MaxNumOverlayVerts * sizeof( OverlayVert ), STREAM, 0x0 );

	// Create unit primitives
	createPrimitives();

	// Init scratch buffer with some default size
	useScratchBuf( 4 * 1024*1024 );

	// Reset states
	finishRendering();

	// Start frame timer
	radiant::perfmon::Timer *timer = Modules::stats().getTimer( EngineStats::FrameTime );
	ASSERT( timer != 0x0 );
	timer->Start();
	
	return true;
}


void Renderer::initStates()
{
	gRDI->initStates();
}


// =================================================================================================
// Misc Helper Functions
// =================================================================================================

void Renderer::setupViewMatrices( const Matrix4f &viewMat, const Matrix4f &projMat )
{
	// Note: The viewing matrices should be set before a material is set, otherwise the general
	//       uniforms need to be commited manually
	
	_viewMat = viewMat;
	_viewMatInv = viewMat.inverted();
	_projMat = projMat;
	_viewProjMat = projMat * viewMat;
	_viewProjMatInv = _viewProjMat.inverted();

	++_curShaderUpdateStamp;
}


// =================================================================================================
// Rendering Helper Functions
// =================================================================================================

void Renderer::createPrimitives()
{
	// Unit cube
	float cubeVerts[8 * 3] = {  // x, y, z
		0.f, 0.f, 1.f,   1.f, 0.f, 1.f,   1.f, 1.f, 1.f,   0.f, 1.f, 1.f,
		0.f, 0.f, 0.f,   1.f, 0.f, 0.f,   1.f, 1.f, 0.f,   0.f, 1.f, 0.f
	};
	uint16 cubeInds[36] = {
		0, 1, 2, 2, 3, 0,   1, 5, 6, 6, 2, 1,   5, 4, 7, 7, 6, 5,
		4, 0, 3, 3, 7, 4,   3, 2, 6, 6, 7, 3,   4, 5, 1, 1, 0, 4
	};
	_vbCube = gRDI->createVertexBuffer( 8 * 3 * sizeof( float ), STATIC, cubeVerts );
	_ibCube = gRDI->createIndexBuffer( 36 * sizeof( uint16 ), cubeInds );

   _vbFrust = gRDI->createVertexBuffer(8 * 3 * sizeof( float ), STREAM, nullptr);

   uint16 polyInds[96];
   for (int i = 0; i < 31; i++) {
      polyInds[i * 3] = 0;
      polyInds[i * 3 + 1] = i + 1;
      polyInds[i * 3 + 2] = i + 2;
   }
   _vbPoly = gRDI->createVertexBuffer(32 * 3 * sizeof( float ), STREAM, nullptr);
   _ibPoly = gRDI->createIndexBuffer(96 * sizeof(uint16), polyInds);

	// Unit (geodesic) sphere (created by recursively subdividing a base octahedron)
	Vec3f spVerts[126] = {  // x, y, z
		Vec3f( 0.f, 1.f, 0.f ),   Vec3f( 0.f, -1.f, 0.f ),
		Vec3f( -0.707f, 0.f, 0.707f ),   Vec3f( 0.707f, 0.f, 0.707f ),
		Vec3f( 0.707f, 0.f, -0.707f ),   Vec3f( -0.707f, 0.f, -0.707f )
	};
	uint16 spInds[128 * 3] = {  // Number of faces: (4 ^ iterations) * 8
		2, 3, 0,   3, 4, 0,   4, 5, 0,   5, 2, 0,   2, 1, 3,   3, 1, 4,   4, 1, 5,   5, 1, 2
	};
	for( uint32 i = 0, nv = 6, ni = 24; i < 2; ++i )  // Two iterations
	{
		// Subdivide each face into 4 tris by bisecting each edge and push vertices onto unit sphere
		for( uint32 j = 0, prevNumInds = ni; j < prevNumInds; j += 3 )
		{
			spVerts[nv++] = ((spVerts[spInds[j + 0]] + spVerts[spInds[j + 1]]) * 0.5f).normalized();
			spVerts[nv++] = ((spVerts[spInds[j + 1]] + spVerts[spInds[j + 2]]) * 0.5f).normalized();
			spVerts[nv++] = ((spVerts[spInds[j + 2]] + spVerts[spInds[j + 0]]) * 0.5f).normalized();

			spInds[ni++] = spInds[j + 0]; spInds[ni++] = nv - 3; spInds[ni++] = nv - 1;
			spInds[ni++] = nv - 3; spInds[ni++] = spInds[j + 1]; spInds[ni++] = nv - 2;
			spInds[ni++] = nv - 2; spInds[ni++] = spInds[j + 2]; spInds[ni++] = nv - 1;
			spInds[j + 0] = nv - 3; spInds[j + 1] = nv - 2; spInds[j + 2] = nv - 1;
		}
	}
	_vbSphere = gRDI->createVertexBuffer( 126 * sizeof( Vec3f ), STREAM, spVerts );
	_ibSphere = gRDI->createIndexBuffer( 128 * 3 * sizeof( uint16 ), spInds );
	
	// Unit cone
	float coneVerts[13 * 3] = {  // x, y, z
		0.f, 0.f, 0.f,
		0.f, 1.f, -1.f,   -0.5f, 0.866f, -1.f,   -0.866f, 0.5f, -1.f,
		-1.f, 0.f, -1.f,   -0.866f, -0.5f, -1.f,   -0.5f, -0.866f, -1.f,
		0.f, -1.f, -1.f,   0.5f, -0.866f, -1.f,   0.866f, -0.5f, -1.f,
		1.f, 0.f, -1.f,   0.866f, 0.5f, -1.f,   0.5f, 0.866f, -1.f,
	};
	uint16 coneInds[22 * 3] = {
		0, 1, 2,   0, 2, 3,   0, 3, 4,   0, 4, 5,   0, 5, 6,   0, 6, 7,
		0, 7, 8,   0, 8, 9,   0, 9, 10,   0, 10, 11,   0, 11, 12,   0, 12, 1,
		10, 6, 2,   10, 8, 6,   10, 9, 8,   8, 7, 6,   6, 4, 2,   6, 5, 4,   4, 3, 2,
		2, 12, 10,   2, 1, 12,   12, 11, 10
	};
	_vbCone = gRDI->createVertexBuffer( 13 * 3 * sizeof( float ), STREAM, coneVerts );
	_ibCone = gRDI->createIndexBuffer( 22 * 3 * sizeof( uint16 ), coneInds );

	// Fullscreen polygon
	float fsVerts[3 * 3] = {  // x, y, z
		0.f, 0.f, 1.f,   2.f, 0.f, 1.f,   0.f, 2.f, 1.f
	};
	_vbFSPoly = gRDI->createVertexBuffer( 3 * 3 * sizeof( float ), STREAM, fsVerts );
}

void Renderer::drawFrustum(const Frustum& frust)
{
   float *vs = (float *)gRDI->mapBuffer(_vbFrust);

   for (int i = 0; i < 8; i++)
   {
      vs[i * 3 + 0] = frust.getCorner(i).x;
      vs[i * 3 + 1] = frust.getCorner(i).y;
      vs[i * 3 + 2] = frust.getCorner(i).z;
   }

   gRDI->unmapBuffer(_vbFrust);

   Matrix4f mat = Matrix4f();
   mat.toIdentity();
   gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
   gRDI->setVertexBuffer( 0, _vbFrust, 0, 12 );
   gRDI->setIndexBuffer( _ibCube, IDXFMT_16 );
   gRDI->setVertexLayout( _vlPosOnly );

   gRDI->drawIndexed( PRIM_TRILIST, 0, 36, 0, 8 );
}


void Renderer::drawPoly(const std::vector<Vec3f>& poly)
{
   float *vs = (float *)gRDI->mapBuffer(_vbPoly);

   int i = 0;
   for (const auto& vec : poly)
   {
      vs[i++] = vec.x; vs[i++] = vec.y; vs[i++] = vec.z;
   }

   gRDI->unmapBuffer(_vbPoly);

   int polySize = (int)poly.size();
   Matrix4f mat = Matrix4f();
   mat.toIdentity();
   gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
   gRDI->setVertexBuffer( 0, _vbPoly, 0, 12 );
   gRDI->setIndexBuffer( _ibPoly, IDXFMT_16 );
   gRDI->setVertexLayout( _vlPosOnly );
   gRDI->drawIndexed( PRIM_TRILIST, 0, 3 + ((polySize - 3) * 3), 0, polySize );
}

void Renderer::drawAABB( const Vec3f &bbMin, const Vec3f &bbMax )
{
	ASSERT( _curShader != 0x0 );
	
	Matrix4f mat = Matrix4f::TransMat( bbMin.x, bbMin.y, bbMin.z ) *
		Matrix4f::ScaleMat( bbMax.x - bbMin.x, bbMax.y - bbMin.y, bbMax.z - bbMin.z );
	gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
	
	gRDI->setVertexBuffer( 0, _vbCube, 0, 12 );
	gRDI->setIndexBuffer( _ibCube, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->drawIndexed( PRIM_TRILIST, 0, 36, 0, 8 );
}


void Renderer::drawSphere( const Vec3f &pos, float radius )
{
	ASSERT( _curShader != 0x0 );

	Matrix4f mat = Matrix4f::TransMat( pos.x, pos.y, pos.z ) *
	               Matrix4f::ScaleMat( radius, radius, radius );
	gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
	
	gRDI->setVertexBuffer( 0, _vbSphere, 0, 12 );
	gRDI->setIndexBuffer( _ibSphere, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->drawIndexed( PRIM_TRILIST, 0, 128 * 3, 0, 126 );
}


void Renderer::drawCone( float height, float radius, const Matrix4f &transMat )
{
	ASSERT( _curShader != 0x0 );

	Matrix4f mat = transMat * Matrix4f::ScaleMat( radius, radius, height );
	gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
	
	gRDI->setVertexBuffer( 0, _vbCone, 0, 12 );
	gRDI->setIndexBuffer( _ibCone, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->drawIndexed( PRIM_TRILIST, 0, 22 * 3, 0, 13 );
}


// =================================================================================================
// Material System
// =================================================================================================

bool Renderer::createShaderComb( const char* filename, const char *vertexShader, const char *fragmentShader, ShaderCombination &sc )
{
#if defined(OPTIMIZE_GSLS)
   auto optimize = [=](const char* input, glslopt_shader_type type) -> std::string {
      std::string result;
	   glslopt_shader* shader = glslopt_optimize (_glsl_opt_ctx, type, input, 0);
      bool optimizeOk = glslopt_get_status(shader);
	   if (optimizeOk) {
         result = glslopt_get_output (shader);
         //R_LOG(1) << "using optimized shader " << result;
      } else {
         //R_LOG(1) << "shader optimization failed " << glslopt_get_log(shader);
         //R_LOG(1) << "original shader: " << std::endl << input;
         result = input;
      }
      glslopt_shader_delete(shader);
      return result;
   };
   std::string vopt = optimize(vertexShader, kGlslOptShaderVertex);
   std::string fopt = optimize(fragmentShader, kGlslOptShaderFragment);
   vertexShader = vopt.c_str();
   fragmentShader = fopt.c_str();
#endif

	// Create shader program
	uint32 shdObj = gRDI->createShader( filename, vertexShader, fragmentShader );
	if( shdObj == 0 ) return false;
	
	sc.shaderObj = shdObj;
	gRDI->bindShader( shdObj );
	
	// Set standard uniforms
	int loc =gRDI-> getShaderSamplerLoc( shdObj, "shadowMap" );
	if( loc >= 0 ) gRDI->setShaderSampler( loc, 12 );

	// Misc general uniforms
	sc.uni_currentTime = gRDI->getShaderConstLoc( shdObj, "currentTime" );
	sc.uni_frameBufSize = gRDI->getShaderConstLoc( shdObj, "frameBufSize" );
   sc.uni_lodLevels = gRDI->getShaderConstLoc( shdObj, "lodLevels" );
   sc.uni_viewPortSize = gRDI->getShaderConstLoc( shdObj, "viewPortSize" );
   sc.uni_viewPortPos = gRDI->getShaderConstLoc( shdObj, "viewPortPos" );
   sc.uni_halfTanFoV = gRDI->getShaderConstLoc( shdObj, "halfTanFoV" );
   sc.uni_nearPlane = gRDI->getShaderConstLoc( shdObj, "nearPlane" );
   sc.uni_farPlane = gRDI->getShaderConstLoc( shdObj, "farPlane" );
	
	// View/projection uniforms
	sc.uni_viewMat = gRDI->getShaderConstLoc( shdObj, "viewMat" );
	sc.uni_viewMatInv = gRDI->getShaderConstLoc( shdObj, "viewMatInv" );
	sc.uni_projMat = gRDI->getShaderConstLoc( shdObj, "projMat" );
	sc.uni_viewProjMat = gRDI->getShaderConstLoc( shdObj, "viewProjMat" );
	sc.uni_viewProjMatInv = gRDI->getShaderConstLoc( shdObj, "viewProjMatInv" );
	sc.uni_viewerPos = gRDI->getShaderConstLoc( shdObj, "viewerPos" );
   sc.uni_camViewProjMat = gRDI->getShaderConstLoc( shdObj, "camViewProjMat" );
   sc.uni_camViewProjMatInv = gRDI->getShaderConstLoc( shdObj, "camViewProjMatInv" );
   sc.uni_camProjMat = gRDI->getShaderConstLoc( shdObj, "camProjMat" );
   sc.uni_camViewMat = gRDI->getShaderConstLoc( shdObj, "camViewMat" );
   sc.uni_camViewMatInv = gRDI->getShaderConstLoc( shdObj, "camViewMatInv" );
   sc.uni_camViewerPos = gRDI->getShaderConstLoc( shdObj, "camViewerPos" );
   sc.uni_projectorMat = gRDI->getShaderConstLoc( shdObj, "projectorMat" );
	
	// Per-instance uniforms
	sc.uni_worldMat = gRDI->getShaderConstLoc( shdObj, "worldMat" );
	sc.uni_worldNormalMat = gRDI->getShaderConstLoc( shdObj, "worldNormalMat" );
	sc.uni_nodeId = gRDI->getShaderConstLoc( shdObj, "nodeId" );
	sc.uni_skinMatRows = gRDI->getShaderConstLoc( shdObj, "skinMatRows[0]" );
	
	// Lighting uniforms
	sc.uni_lightPos = gRDI->getShaderConstLoc( shdObj, "lightPos" );
	sc.uni_lightRadii = gRDI->getShaderConstLoc( shdObj, "lightRadii" );
	sc.uni_lightDir = gRDI->getShaderConstLoc( shdObj, "lightDir" );
	sc.uni_lightColor = gRDI->getShaderConstLoc( shdObj, "lightColor" );
	sc.uni_lightAmbientColor = gRDI->getShaderConstLoc( shdObj, "lightAmbientColor" );
	sc.uni_shadowSplitDists = gRDI->getShaderConstLoc( shdObj, "shadowSplitDists" );
	sc.uni_shadowMats = gRDI->getShaderConstLoc( shdObj, "shadowMats" );
	sc.uni_shadowMapSize = gRDI->getShaderConstLoc( shdObj, "shadowMapSize" );
	
	// Particle-specific uniforms
	sc.uni_parPosArray = gRDI->getShaderConstLoc( shdObj, "parPosArray" );
	sc.uni_parSizeAndRotArray = gRDI->getShaderConstLoc( shdObj, "parSizeAndRotArray" );
	sc.uni_parColorArray = gRDI->getShaderConstLoc( shdObj, "parColorArray" );

   sc.uni_cubeBatchTransformArray = gRDI->getShaderConstLoc( shdObj, "cubeBatchTransformArray" );
   sc.uni_cubeBatchColorArray = gRDI->getShaderConstLoc( shdObj, "cubeBatchColorArray" );
	
   sc.uni_bones = gRDI->getShaderConstLoc(shdObj, "bones");
   sc.uni_modelScale = gRDI->getShaderConstLoc(shdObj, "modelScale");

   // Overlay-specific uniforms
	sc.uni_olayColor = gRDI->getShaderConstLoc( shdObj, "olayColor" );

	return true;
}


void Renderer::releaseShaderComb( ShaderCombination &sc )
{
	gRDI->destroyShader( sc.shaderObj );
}


void Renderer::setShaderComb( ShaderCombination *sc )
{
	if (_curShader != sc) {
		if (sc == 0x0) {
         gRDI->bindShader(0);
      } else {
         gRDI->bindShader(sc->shaderObj);
      }
		_curShader = sc;
	}
}

void Renderer::addSelectedNode(NodeHandle n, float r, float g, float b) {
   for (SelectedNode& s : _selectedNodes) {
      if (s.h == n) {
         return;
      }
   }

   _selectedNodes.push_back(SelectedNode(n, Vec3f(r, g, b)));
}


void Renderer::removeSelectedNode(NodeHandle n) {
   int i = 0;
   for (SelectedNode& s : _selectedNodes) {
      if (s.h == n) {
         _selectedNodes.erase(_selectedNodes.begin() + i);
         return;
      }
      i++;
   }
}

void Renderer::commitGlobalUniforms()
{
   for (auto& e : _uniformFloats)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         gRDI->setShaderConst(loc, CONST_FLOAT, &e.second);
      }
   }

   for (auto& e : _uniformVecs)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         Vec4f& v = e.second;
         gRDI->setShaderConst(loc, CONST_FLOAT4, &v);
      }
   }

   for (auto& e : _uniformVec3s)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         Vec3f& v = e.second;
         gRDI->setShaderConst(loc, CONST_FLOAT3, &v);
      }
   }

   for (auto& e : _uniformMats)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         Matrix4f& m = e.second;
         gRDI->setShaderConst(loc, CONST_FLOAT44, m.x);
      }
   }

   for (auto& e : _uniformMatArrays)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         std::vector<float>& m = e.second;
         gRDI->setShaderConst(loc, CONST_FLOAT44, m.data(), (int)m.size() / 16);
      }
   }
}


void Renderer::commitLightUniforms(LightNode const* light)
{
   float data[4];
   if (light == nullptr) {
      return;
   }

   data[0] = light->_absPos.x; data[1] = light->_absPos.y; data[2] = light->_absPos.z;
   setGlobalUniform("lightPos", UniformType::VEC4, data);

   data[0] = light->_radius1; data[1] = light->_radius2; data[2] = 1.0f / (data[1] - data[0]);
   setGlobalUniform("lightRadii", UniformType::VEC4, data);

   data[0] = light->_spotDir.x; data[1] = light->_spotDir.y; data[2] = light->_spotDir.z; data[3] = cosf(degToRad(light->_fov / 2.0f));
   setGlobalUniform("lightDir", UniformType::VEC4, data);

   Vec3f col = light->_diffuseCol * light->_diffuseColMult;
   setGlobalUniform("lightColor", UniformType::VEC3, &col.x);

   setGlobalUniform("lightAmbientColor", UniformType::VEC3, &light->_ambientCol.x);

   setGlobalUniform("shadowSplitDists", UniformType::VEC4, &_splitPlanes[1]);

   setGlobalUniform("shadowMats", UniformType::MAT44_ARRAY, &_lightMats[0].x[0], 4);

   setGlobalUniform("shadowMapSize", UniformType::FLOAT, &light->_shadowMapSize);
}


void Renderer::commitGeneralUniforms()
{
	ASSERT( _curShader != 0x0 );

   commitGlobalUniforms();

	// Note: Make sure that all functions which modify one of the following params increase the stamp
	if( _curShader->lastUpdateStamp != _curShaderUpdateStamp )
	{
		if( _curShader->uni_frameBufSize >= 0 )
		{
         float dimensions[4] = { (float)gRDI->_fbWidth, (float)gRDI->_fbHeight, 1.0f / gRDI->_fbWidth, 1.0f / gRDI->_fbHeight };
			gRDI->setShaderConst( _curShader->uni_frameBufSize, CONST_FLOAT4, dimensions );
		}

      if( _curShader->uni_lodLevels >= 0 )
      {
         gRDI->setShaderConst( _curShader->uni_lodLevels, CONST_FLOAT4, &_lodValues );
      }

      if (_curShader->uni_viewPortSize >= 0)
      {
         float dimensions[2] = { (float)gRDI->_vpWidth, (float)gRDI->_vpHeight };
			gRDI->setShaderConst( _curShader->uni_viewPortSize, CONST_FLOAT2, dimensions );
      }

      if (_curShader->uni_viewPortPos >= 0)
      {
         float dimensions[2] = { (float)gRDI->_vpX, (float)gRDI->_vpY };
			gRDI->setShaderConst( _curShader->uni_viewPortPos, CONST_FLOAT2, dimensions );
      }

      if ( _curShader->uni_halfTanFoV >= 0 )
      {
         float topV = _curCamera->getParamF(CameraNodeParams::TopPlaneF, 0);
         float nearV = _curCamera->getParamF(CameraNodeParams::NearPlaneF, 0);
         float f = topV / nearV;
         gRDI->setShaderConst( _curShader->uni_halfTanFoV, CONST_FLOAT, &f);
      }

      if ( _curShader->uni_nearPlane >= 0 )
      {
         float nearV = _curCamera->getParamF(CameraNodeParams::NearPlaneF, 0);
         gRDI->setShaderConst( _curShader->uni_nearPlane, CONST_FLOAT, &nearV);
      }

      if ( _curShader->uni_farPlane >= 0 )
      {
         float farV = _curCamera->getParamF(CameraNodeParams::FarPlaneF, 0);
         gRDI->setShaderConst( _curShader->uni_farPlane, CONST_FLOAT, &farV);
      }

      if( _curShader->uni_currentTime >= 0 )
			gRDI->setShaderConst( _curShader->uni_currentTime, CONST_FLOAT, &_currentTime );
		
		// Viewer params
		if( _curShader->uni_viewMat >= 0 )
			gRDI->setShaderConst( _curShader->uni_viewMat, CONST_FLOAT44, _viewMat.x );
		
		if( _curShader->uni_viewMatInv >= 0 )
			gRDI->setShaderConst( _curShader->uni_viewMatInv, CONST_FLOAT44, _viewMatInv.x );
		
		if( _curShader->uni_projMat >= 0 )
			gRDI->setShaderConst( _curShader->uni_projMat, CONST_FLOAT44, _projMat.x );
		
		if( _curShader->uni_viewProjMat >= 0 )
			gRDI->setShaderConst( _curShader->uni_viewProjMat, CONST_FLOAT44, _viewProjMat.x );

		if( _curShader->uni_viewProjMatInv >= 0 )
			gRDI->setShaderConst( _curShader->uni_viewProjMatInv, CONST_FLOAT44, _viewProjMatInv.x );
		
		if( _curShader->uni_viewerPos >= 0 )
			gRDI->setShaderConst( _curShader->uni_viewerPos, CONST_FLOAT3, &_viewMatInv.x[12] );

      if( _curShader->uni_camProjMat >= 0 )
      {
         Matrix4f m = getCurCamera()->getProjMat();
			gRDI->setShaderConst( _curShader->uni_camProjMat, CONST_FLOAT44, m.x );
      }

      if( _curShader->uni_camViewProjMat >= 0)
      {
         Matrix4f m = getCurCamera()->getProjMat() * getCurCamera()->getViewMat();
			gRDI->setShaderConst( _curShader->uni_camViewProjMat, CONST_FLOAT44, m.x );
      }

      if( _curShader->uni_camViewProjMatInv >= 0)
      {
         Matrix4f m = (getCurCamera()->getProjMat() * getCurCamera()->getViewMat()).inverted();
			gRDI->setShaderConst( _curShader->uni_camViewProjMatInv, CONST_FLOAT44, m.x );
      }

      if( _curShader->uni_camViewMat >= 0)
      {
         Matrix4f m = getCurCamera()->getViewMat();
			gRDI->setShaderConst( _curShader->uni_camViewMat, CONST_FLOAT44, m.x );
      }

      if( _curShader->uni_camViewMatInv >= 0)
      {
         Matrix4f m = getCurCamera()->getViewMat().inverted();
			gRDI->setShaderConst( _curShader->uni_camViewMatInv, CONST_FLOAT44, m.x );
      }
		
		if( _curShader->uni_camViewerPos >= 0 ) 
      {
		  Vec3f v = getCurCamera()->getAbsPos();
		 float f[3] = {v.x, v.y, v.z };
		 gRDI->setShaderConst( _curShader->uni_camViewerPos, CONST_FLOAT3, f );
      }

      if( _curShader->uni_projectorMat >= 0 )
      {
         Matrix4f m = _projectorMat.inverted();
         gRDI->setShaderConst( _curShader->uni_projectorMat, CONST_FLOAT44, m.x);
      }

      if (_curShader->uni_modelScale >= 0)
      {
         // We set a default scale, in case a shader is used and the renderer doesn't specify the scale.
         // This is fairly cheap, I think....
         float f = 1.0;
         gRDI->setShaderConst(_curShader->uni_modelScale, CONST_FLOAT, &f);
      }

		_curShader->lastUpdateStamp = _curShaderUpdateStamp;
	}
}

ShaderCombination* Renderer::findShaderCombination(ShaderResource* sr) const
{
   ShaderCombination* sc = nullptr;
   // Find a shader combination to match, compiling if necessary.
   for (auto& comb : sr->shaderCombinations)
   {
      sc = &comb;

      // Check to see that every single flag in the engine has a corresponding flag set in the shader.
      for (const auto& flag : Modules().config().shaderFlags)
      {
         if (sc->engineFlags.find(flag) == sc->engineFlags.end()) {
            sc = nullptr;
            break;
         }
      }

      // Also!  Check to see that every flag in the shader has a corresponding flag in the engine.
      if (sc != nullptr) {
         if (sc->engineFlags.size() == Modules().config().shaderFlags.size()) {
            break;
         }
         sc = nullptr;
      }
   }

   // If this is a new combination of engine flags, compile a new combination.
   if (sc == 0x0)
   {
      sr->shaderCombinations.push_back(ShaderCombination());
      ShaderCombination& sco = sr->shaderCombinations.back();
      for (const auto& flag : Modules().config().shaderFlags) {
         sco.engineFlags.insert(flag);
      }

      sr->compileCombination(sco);
      sc = &sr->shaderCombinations.back();
   }
   return sc;
}

bool Renderer::isShaderContextSwitch(std::string const& newContext, MaterialResource* materialRes) const
{
   PShaderResource sr = materialRes->getShader(newContext);
   ShaderCombination* sc = nullptr;

   if (!sr.getPtr()) {
      return false;
   }

   return findShaderCombination(sr) != _curShader;
}


bool Renderer::setMaterialRec(MaterialResource *materialRes, std::string const& shaderContext)
{
   radiant::perfmon::TimelineCounterGuard smr("setMaterialRec");
   if( materialRes == 0x0 ) {
      MAT_LOG(7) << "ignoring null MaterialResource in setMaterialRec";
      return false;
   }

   PShaderResource shaderRes = materialRes->getShader(shaderContext);

   if (!shaderRes.getPtr()) {
      MAT_LOG(7) << materialRes->getName() << " has no suitable shader in setMaterialRec.";
      return false;
   }

   ShaderCombination* sc = findShaderCombination(shaderRes);
   ShaderStateResource* stateRes = materialRes->getShaderState(shaderContext);

   // Set shader combination
   setShaderComb(sc);

   // Setup standard shader uniforms
   commitGeneralUniforms();

   // Configure color write mask.
   glColorMask(stateRes->writeMask & 1 ? GL_TRUE : GL_FALSE, 
      stateRes->writeMask & 2 ? GL_TRUE : GL_FALSE, 
      stateRes->writeMask & 4 ? GL_TRUE : GL_FALSE, 
      stateRes->writeMask & 8 ? GL_TRUE : GL_FALSE);

   // Configure depth mask
   if (stateRes->writeDepth) {
      glDepthMask(GL_TRUE);
   } else {
      glDepthMask(GL_FALSE);
   }

   // Configure cull mode
   if(!Modules::config().wireframeMode)
   {
      switch(stateRes->cullMode)
      {
      case CullModes::Back:
         glEnable( GL_CULL_FACE );
         glCullFace( GL_BACK );
         break;
      case CullModes::Front:
         glEnable( GL_CULL_FACE );
         glCullFace( GL_FRONT );
         break;
      case CullModes::None:
         glDisable( GL_CULL_FACE );
         break;
      }
   }

   switch(stateRes->stencilOpModes) 
   {
   case StencilOpModes::Off:
      glDisable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      break;
   case StencilOpModes::Keep_Dec_Dec:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_DECR, GL_DECR);
      break;
   case StencilOpModes::Keep_Inc_Inc:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
      break;
   case StencilOpModes::Keep_Keep_Dec:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
      break;
   case StencilOpModes::Keep_Keep_Inc:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
      break;
   case StencilOpModes::Replace_Replace_Replace:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      break;
   case StencilOpModes::Keep_Keep_Keep:
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      break;
   }

   switch(stateRes->stencilFunc)
   {
   case TestModes::LessEqual:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_LEQUAL, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::Equal:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_EQUAL, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::Always:
      if (stateRes->stencilOpModes == StencilOpModes::Off) {
         glDisable(GL_STENCIL_TEST);
      } else {
         glEnable(GL_STENCIL_TEST);
      }
      glStencilFunc( GL_ALWAYS, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::Less:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_LESS, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::Greater:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_GREATER, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::GreaterEqual:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_GEQUAL, stateRes->stencilRef, 0xffffffff );
      break;
   case TestModes::NotEqual:
      glEnable(GL_STENCIL_TEST);
      glStencilFunc( GL_NOTEQUAL, stateRes->stencilRef, 0xffffffff );
      break;
   }

   // Configure blending
   switch(stateRes->blendMode)
   {
   case BlendModes::Replace:
      glDisable( GL_BLEND );
      break;
   case BlendModes::Blend:
      glEnable( GL_BLEND );
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
      break;
   case BlendModes::ReplaceByAlpha:
      glEnable( GL_BLEND );
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
      break;
   case BlendModes::Add:
      glEnable( GL_BLEND );
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc( GL_ONE, GL_ONE );
      break;
   case BlendModes::AddBlended:
      glEnable( GL_BLEND );
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc( GL_SRC_ALPHA, GL_ONE );
      break;
   case BlendModes::Sub:
      glEnable(GL_BLEND);
      glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
      glBlendFunc(GL_ONE, GL_ONE);
      break;
   case BlendModes::Mult:
      glEnable( GL_BLEND );
      glBlendEquation(GL_FUNC_ADD);
      glBlendFunc( GL_DST_COLOR, GL_ZERO );
      break;
   }

   // Configure depth test
   if(stateRes->depthTest)
   {
      glEnable(GL_DEPTH_TEST);

      switch(stateRes->depthFunc)
      {
      case TestModes::LessEqual:
         glDepthFunc( GL_LEQUAL );
         break;
      case TestModes::Equal:
         glDepthFunc( GL_EQUAL );
         break;
      case TestModes::Always:
         glDepthFunc( GL_ALWAYS );
         break;
      case TestModes::Less:
         glDepthFunc( GL_LESS );
         break;
      case TestModes::Greater:
         glDepthFunc( GL_GREATER );
         break;
      case TestModes::GreaterEqual:
         glDepthFunc( GL_GEQUAL );
         break;
      }
   }
   else
   {
      glDisable( GL_DEPTH_TEST );
   }

   // Configure alpha-to-coverage
   if(stateRes->alphaToCoverage && Modules::config().sampleCount > 0)
      glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
   else
      glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );

   // Setup texture samplers
   for( size_t i = 0, si = shaderRes->_samplers.size(); i < si; ++i )
   {
      if( _curShader->customSamplers[i] < 0 ) continue;

      ShaderSampler &sampler = shaderRes->_samplers[i];
      TextureResource *texRes = 0x0;

      // Use default texture
      texRes = sampler.defTex;

      // Find sampler in material
      for (auto& matSampler : materialRes->getSamplers()) {
         if (matSampler.name == sampler.id) {
            if (matSampler.texRes->isLoaded()) {
               texRes = matSampler.texRes;
               break;
            }
         }
      }

      uint32 sampState = shaderRes->_samplers[i].sampState;
      if( (sampState & SS_FILTER_TRILINEAR) && !Modules::config().trilinearFiltering )
         sampState = (sampState & ~SS_FILTER_TRILINEAR) | SS_FILTER_BILINEAR;
      if( (sampState & SS_ANISO_MASK) > _maxAnisoMask )
         sampState = (sampState & ~SS_ANISO_MASK) | _maxAnisoMask;

      // Bind texture
      if( texRes != 0x0 )
      {
         if( texRes->getTexType() != sampler.type ) break;  // Wrong type

         if( texRes->getTexType() == TextureTypes::Tex2D )
         {
            if( texRes->getRBObject() == 0 )
            {
               gRDI->setTexture( shaderRes->_samplers[i].texUnit, texRes->getTexObject(), sampState );
            }
            else if( texRes->getRBObject() != gRDI->_curRendBuf )
            {
               gRDI->setTexture( shaderRes->_samplers[i].texUnit,
                  gRDI->getRenderBufferTex( texRes->getRBObject(), 0 ), sampState );
            }
            else  // Trying to bind active render buffer as texture
            {
               gRDI->setTexture( shaderRes->_samplers[i].texUnit, TextureResource::defTex2DObject, 0 );
            }
         }
         else
         {
            gRDI->setTexture( shaderRes->_samplers[i].texUnit, texRes->getTexObject(), sampState );
         }
      }

      // Find sampler in pipeline
      for( size_t j = 0, sj = _pipeSamplerBindings.size(); j < sj; ++j )
      {
         if( strcmp( _pipeSamplerBindings[j].sampler, sampler.id.c_str() ) == 0 )
         {
            gRDI->setTexture( shaderRes->_samplers[i].texUnit, gRDI->getRenderBufferTex(
               _pipeSamplerBindings[j].rbObj, _pipeSamplerBindings[j].bufIndex ), sampState );

            break;
         }
      }
   }

   // Set custom uniforms
   for (size_t i = 0, si = shaderRes->_uniforms.size(); i < si; ++i) {
      if (_curShader->customUniforms[i] < 0) {
         continue;
      }

      float *unifData = 0x0;

      // Find uniform in material
      MatUniform *matUniform = materialRes->getUniform(shaderContext, shaderRes->_uniforms[i].id);
      if (matUniform) {
         if (shaderRes->_uniforms[i].arraySize > 1) {
            unifData = matUniform->arrayValues.data();
         } else {
            unifData = matUniform->values;
         }
      }

      // Use default values if not found
      if (unifData == 0x0) {
         unifData = shaderRes->_uniforms[i].defValues;
      }

      if (unifData) {
         switch (shaderRes->_uniforms[i].size)
         {
         case 1:
            gRDI->setShaderConst(_curShader->customUniforms[i], CONST_FLOAT, unifData, shaderRes->_uniforms[i].arraySize);
            break;
         case 4:
            gRDI->setShaderConst(_curShader->customUniforms[i], CONST_FLOAT4, unifData, shaderRes->_uniforms[i].arraySize);
            break;
         }
      }
   }
   return true;
}


bool Renderer::setMaterial( MaterialResource *materialRes, std::string const& shaderContext )
{
   if (materialRes == 0x0) {
      setShaderComb(0x0);
      glDisable(GL_BLEND);
      glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
      glDepthMask(GL_TRUE);
      return false;
   }

   MAT_LOG(5) << "installing material " << materialRes->getName() << " in setMaterial.";
   if (setMaterialRec(materialRes, shaderContext)) {
      return true;
   }

   MAT_LOG(5) << "nothing worked!  failed to install material";
   _curShader = 0x0;
   return false;
}


// =================================================================================================
// Shadowing
// =================================================================================================

void Renderer::setupShadowMap(LightNode const* light, bool noShadows)
{
	
   // Bind shadow map
   if (!noShadows && light->_shadowMapCount > 0)
   {
      if (light->_directional) {
         uint32 sampState = SS_FILTER_BILINEAR | SS_ANISO1 | SS_ADDR_CLAMPCOL | SS_COMP_LEQUAL;
         gRDI->setTexture(12, gRDI->getRenderBufferTex(light->getShadowBuffer(), 32), sampState);
      } else {
         // Cube-map shadows are light-distances stored in the color channel.
         uint32 sampState = SS_FILTER_BILINEAR | SS_ANISO1 | SS_ADDR_CLAMP;
         gRDI->setTexture(12, gRDI->getRenderBufferTex(light->getShadowBuffer(), 0), sampState);
      }
   } else {
      uint32 sampState = SS_FILTER_BILINEAR | SS_ANISO1 | SS_ADDR_CLAMPCOL | SS_COMP_LEQUAL;
      gRDI->setTexture(12, _defShadowMap, sampState);
   }
}


void Renderer::quantizeShadowFrustum(const Frustum& frustSlice, int shadowMapSize, Vec3f* min, Vec3f* max)
{
   // Adapted from Microsoft's Direct SDK Cascaded Shadows example.

   // The basic idea here is to get a bound on the size of the frustum (which, in order for quantization
   // to work, should be a constant).  Use this bound (plus a little border) to quantize the x-y min/max
   // values of the frustum, thus resulting in a constant-sized frustum for the light (until the light
   // moves, of course).

   // The bound is the length of the diagonal of the frustum slice--a strict over-estimate.  The bound
   // needs to be big enough to accommodate the maximum size of the frustum slice, which is the
   // diagonal.
   float sliceBound = (frustSlice.getCorner(0) - frustSlice.getCorner(6)).length();

   // Compute a small border offset, based on the differences between the slice and the current bounds,
   // and increase the shadow frustum, so that it's big enough to envelope the entire frustum slice.
   Vec3f boarderOffset = (Vec3f(sliceBound , sliceBound , sliceBound ) - (*max - *min)) * 0.5f;
   boarderOffset.z = 0;
   *max += boarderOffset;
   *min -= boarderOffset;

   // The world units per texel are used to snap the shadow the orthographic projection
   // to texel sized increments.  This keeps the edges of the shadows from shimmering.
   float worldUnitsPerTexel = sliceBound  / shadowMapSize;
   Vec3f quantizer(worldUnitsPerTexel, worldUnitsPerTexel, 0);
   min->quantize(quantizer);
   max->quantize(quantizer);
}


Matrix4f Renderer::calcDirectionalLightShadowProj(LightNode const* light, BoundingBox const& worldBounds, Frustum const& frustSlice, 
                                                  Matrix4f const& lightViewMat) const
{
   const int numShadowTiles = light->_shadowMapCount > 1 ? 2 : 1;
   const int shadowMapSize = light->_shadowMapSize / numShadowTiles;
   // Pull out the mins/maxes of the frustum's corners.  Those bounds become the new projection matrix
   // for our directional light's frustum.  We only want to save the 'x' and 'y' values.
   Vec3f min, max;
   min = max = lightViewMat * frustSlice.getCorner(0);
   for (int i = 1; i < 8; i++)
   {
      Vec3f pt = lightViewMat * frustSlice.getCorner(i);
      for (int j = 0; j < 3; j++)
      {
         min[j] = std::min(min[j], pt[j]);
         max[j] = std::max(max[j], pt[j]);
      }
   }

   // We get the min and max z-values from the scene itself.
   computeLightFrustumNearFar(worldBounds, lightViewMat, min, max, &min.z, &max.z);

   // If desired, quantize the shadow frustum, in order to prevent swimming shadow textures.
   // quantizeShadowFrustum(frustSlice, shadowMapSize, &min, &max);

   return Matrix4f::OrthoMat(min.x, max.x, min.y, max.y, -max.z, -min.z);
}

void Renderer::reallocateShadowBuffers(int quality)
{
   if (_shadowCascadeBuffer) {
      gRDI->destroyRenderBuffer(_shadowCascadeBuffer);
      _shadowCascadeBuffer = 0;
   }

   // Update the one-and-only shadow cascade buffer here
   if (Modules::config().enableShadows) {
      while (!_shadowCascadeBuffer && Modules::config().shadowMapQuality > 0) {
         uint32 cascadeBufferSize = (uint32)pow(2, Modules::config().shadowMapQuality + 8);
         _shadowCascadeBuffer = gRDI->createRenderBuffer(cascadeBufferSize, cascadeBufferSize, TextureFormats::BGRA8, true, 0, 0);

         if (!_shadowCascadeBuffer) {
            // Allocation failed, so let's try again, more modestly.
            Modules::log().writeError("Couldn't reallocate shadow cascade buffer (%d^2).  Trying with a lower resolution.", cascadeBufferSize);
            Modules::config().shadowMapQuality--;
         }
      }

      if (!_shadowCascadeBuffer) {
         Modules::log().writeError("No cascade buffer allocated!");
      }
   }

   for (auto& scene : Modules::sceneMan().getScenes()) {
      int numNodes = scene->findNodes(scene->getRootNode(), "", SceneNodeTypes::Light);

      for (int i = 0; i < numNodes; i++) {
         LightNode* l = (LightNode*)scene->getFindResult(i);

         l->reallocateShadowBuffer(calculateShadowBufferSize(l));
      }
   }
}

float isInFrustum(float val, float boundsVal, int side)
{
   if (side == 1)
   {
      return val >= boundsVal;
   }

   return val <= boundsVal;
}


float clipLineToAxis(float lineStart, float axisValue, float lDir)
{
   float v = (axisValue - lineStart);

   if (fabs(lDir) < 0.00001)
   {
      return -1.0;
   }
   return v / lDir;
}


int clipPolyToAxis(Vec3f clippedVerts[], int numClippedVerts, float boundsVal, int valIdx, int sign)
{
   Vec3f tempClippedVerts[16];
   int newNumClippedVerts = 0;
   for (int vertNum = 0; vertNum < numClippedVerts; vertNum++)
   {
      Vec3f lStart = clippedVerts[vertNum];
      Vec3f lEnd = clippedVerts[(vertNum + 1) % numClippedVerts];
      Vec3f dir = (lEnd - lStart);

      if (isInFrustum(lStart[valIdx], boundsVal, sign))
      {
         tempClippedVerts[newNumClippedVerts++] = lStart;
      }
   
      float t = clipLineToAxis(lStart[valIdx], boundsVal, dir[valIdx]);
      if (t > 0 && t < 1)
      {
         tempClippedVerts[newNumClippedVerts++] = lStart + (dir * t);
      }
   }
   for (int vertNum = 0; vertNum < newNumClippedVerts; vertNum++)
   {
      clippedVerts[vertNum] = tempClippedVerts[vertNum];
   }

   return newNumClippedVerts;
}


void Renderer::computeLightFrustumNearFar(const BoundingBox& worldBounds, const Matrix4f& lightViewMat, const Vec3f& lightMin, const Vec3f& lightMax, float* nearV, float* farV) const
{
   // In brief: we construct 6 quads, one for each of the sides of the bounding box of the entire world/scene, and then
   // clip each one successively against the side of the light's frustum (ignoring front/back clipping).
   // Find the min/max 'z' values between all the resulting polygons: those values are our min/max values for the
   // light frustum.  Note: do it all in light-space, which makes the clipping very simple.

   Vec3f startingVerts[8];
   Vec3f clippedVerts[16];
   const int quadIndices[] = {
      0,1,2,3,
      1,2,6,5,
      5,6,7,4,
      4,7,3,0,
      6,2,3,7,
      5,4,0,1
   };

   for (int i = 0; i < 8; i++)
   {
      startingVerts[i]  = lightViewMat * worldBounds.getCorner(i);
   }

   // Construct six quads from the transformed bounds, and clip them against the light's frustum
   // (just top/bottom, left/right).
   *nearV = 1000000.0f;
   *farV = -1000000.0f;
   for (int quadNum = 0; quadNum < 6; quadNum++)
   {
      // Load up the quad.
      int numClippedVerts = 4;
      for (int i = 0; i < 4; i++)
      {
         clippedVerts[i] = startingVerts[quadIndices[quadNum * 4 + i]];
      }

      // Clip the quad successively against each frustum bound.
      numClippedVerts = clipPolyToAxis(clippedVerts, numClippedVerts, lightMin.x, 0, 1);
      numClippedVerts = clipPolyToAxis(clippedVerts, numClippedVerts, lightMax.y, 1, -1);
      numClippedVerts = clipPolyToAxis(clippedVerts, numClippedVerts, lightMax.x, 0, -1);
      numClippedVerts = clipPolyToAxis(clippedVerts, numClippedVerts, lightMin.y, 1, 1);

      // Take the resulting quad and extract min/maxes from it.
      for (int vertNum = 0; vertNum < numClippedVerts; vertNum++)
      {
         *nearV = std::min(*nearV, clippedVerts[vertNum].z);
         *farV = std::max(*farV, clippedVerts[vertNum].z);
      }
   }
}

uint32 Renderer::calculateShadowBufferSize(LightNode const* node) const
{
   if (node->_directional) {
      return (uint32)pow(2, Modules::config().shadowMapQuality + 8);
   } else {
      if (!gpuCompatibility_.canDoOmniShadows) {
         return 0;
      }
      int quality = std::max(Modules::config().shadowMapQuality + 3, 6);
      return (uint32)pow(2, quality);
   }
}


void Renderer::updateShadowMap(LightNode const* light, Frustum const* lightFrus, float minDist, float maxDist, int cubeFace)
{
   radiant::perfmon::TimelineCounterGuard smr("updateShadowMap");
	if (light == 0x0) {
      return;
   }

   const SceneId sceneId = Modules::sceneMan().sceneIdFor(light->getHandle());
	
	uint32 prevRendBuf = gRDI->_curRendBuf;
	int prevVPX = gRDI->_vpX, prevVPY = gRDI->_vpY, prevVPWidth = gRDI->_vpWidth, prevVPHeight = gRDI->_vpHeight;
   RDIRenderBuffer &shadowRT = gRDI->_rendBufs.getRef(light->getShadowBuffer());
	gRDI->setViewport(0, 0, shadowRT.width, shadowRT.height);
   gRDI->setRenderBuffer(light->getShadowBuffer(), cubeFace + GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	gRDI->clear(CLR_DEPTH, 0x0, 1.f);
	
   // Calculate split distances using PSSM scheme
   const float nearDist = minDist;
   const float farDist = maxf( maxDist, minDist + 0.01f );
   const uint32 numMaps = light->_shadowMapCount;
   const float lambda = light->_shadowSplitLambda;
	
	_splitPlanes[0] = nearDist;
   _splitPlanes[numMaps] = farDist;
	
	for( uint32 i = 1; i < numMaps; ++i )
	{
		float f = (float)i / numMaps;
		float logDist = nearDist * powf( farDist / nearDist, f );
		float uniformDist = nearDist + (farDist - nearDist) * f;
		
		_splitPlanes[i] = (1 - lambda) * uniformDist + lambda * logDist;  // Lerp
	}

   BoundingBox litAabb;
	Matrix4f lightProjMat;
   Matrix4f lightViewMat;
   if (light->_directional) {
	   // Find AABB of lit geometry
      Vec3f lightAbsPos;

      std::vector<QueryResult> const& results = Modules::sceneMan().sceneForId(sceneId).queryScene(*lightFrus, QueryTypes::Renderables);

      for (const auto& result : results) {
         litAabb.makeUnion(result.bounds);
	   }

      lightAbsPos = (litAabb.min() + litAabb.max()) * 0.5f;
      lightViewMat = Matrix4f(light->getViewMat());
      lightViewMat.x[12] = lightAbsPos.x;
      lightViewMat.x[13] = lightAbsPos.y;
      lightViewMat.x[14] = lightAbsPos.z;
   }

	// Prepare shadow map rendering
	glEnable(GL_DEPTH_TEST);
   gRDI->setShadowOffsets(2.0f, 4.0f);
	//glCullFace( GL_FRONT );	// Front face culling reduces artefacts but produces more "peter-panning"
	// Split viewing frustum into slices and render shadow maps
	Frustum frustum;
	for (uint32 i = 0; i < numMaps; ++i)
	{
		// Create frustum slice
		if (_curCamera->_orthographic)
		{
			frustum.buildBoxFrustum( _curCamera->_absTrans, _curCamera->_frustLeft, _curCamera->_frustRight,
			                         _curCamera->_frustBottom, _curCamera->_frustTop,
			                         -_splitPlanes[i], -_splitPlanes[i + 1] );
		} else {
			float newLeft = _curCamera->_frustLeft * _splitPlanes[i] / _curCamera->_frustNear;
			float newRight = _curCamera->_frustRight * _splitPlanes[i] / _curCamera->_frustNear;
			float newBottom = _curCamera->_frustBottom * _splitPlanes[i] / _curCamera->_frustNear;
			float newTop = _curCamera->_frustTop * _splitPlanes[i] / _curCamera->_frustNear;
			frustum.buildViewFrustum( _curCamera->_absTrans, newLeft, newRight, newBottom, newTop,
			                          _splitPlanes[i], _splitPlanes[i + 1] );
		}

      //gRDI->_frameDebugInfo.addSplitFrustum_(frustum);
		
		// Get light projection matrix
      if (light->_directional) {
         lightProjMat = calcDirectionalLightShadowProj(light, litAabb, frustum, lightViewMat);
      } else {
		   float ymax = 0.1f * tanf(degToRad(45.0f));
		   float xmax = ymax * 1.0f;  // ymax * aspect
         lightProjMat = Matrix4f::PerspectiveMat(-xmax, xmax, -ymax, ymax, 0.1f, light->_radius2 );
         lightViewMat = light->getCubeViewMat((LightCubeFace::List)cubeFace);
      }

		// Generate render queue with shadow casters for current slice
	   // Build optimized light projection matrix
		frustum.buildViewFrustum(lightViewMat, lightProjMat);
      Modules::sceneMan().sceneForId(sceneId).updateQueues("rendering shadowmap", frustum, 0x0, RenderingOrder::None,
			SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, false, 0 );
		
      gRDI->_frameDebugInfo.addShadowCascadeFrustum_(frustum);

		// Create texture atlas if several splits are enabled
		if( numMaps > 1 )
		{
         const int hsm = light->_shadowMapSize / 2;
			const int scissorXY[8] = { 0, 0,  hsm, 0,  hsm, hsm,  0, hsm };
			const float transXY[8] = { -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f,  -0.5f, 0.5f };

			glEnable(GL_SCISSOR_TEST);

			// Select quadrant of shadow map
			lightProjMat.scale( 0.5f, 0.5f, 1.0f );
			lightProjMat.translate( transXY[i * 2], transXY[i * 2 + 1], 0.0f );
			gRDI->setScissorRect( scissorXY[i * 2], scissorXY[i * 2 + 1], hsm, hsm );
		}

      _lightMats[i] = lightProjMat * lightViewMat;
		setupViewMatrices(lightViewMat, lightProjMat);
		
      commitLightUniforms(light);
		// Render at lodlevel = 1 (don't need the higher poly count for shadows, yay!)
		drawRenderables(sceneId, light->_shadowContext, false, &frustum, 0x0, RenderingOrder::None, -1, 1);
      Modules().stats().incStat(EngineStats::ShadowPassCount, 1);
	}

	// Map from post-projective space [-1,1] to texture space [0,1]
	for( uint32 i = 0; i < numMaps; ++i )
	{
      float m[] = {
         0.5, 0.0, 0.0, 0.0,
         0.0, 0.5, 0.0, 0.0,
         0.0, 0.0, 0.5, 0.0,
         0.5, 0.5, 0.5, 1.0
      };
      Matrix4f biasMatrix(m);
      _lightMats[i] = biasMatrix * _lightMats[i];
	}

	// ********************************************************************************************

	glCullFace( GL_BACK );
	glDisable( GL_SCISSOR_TEST );

   gRDI->setShadowOffsets(0.0f, 0.0f);
	gRDI->setViewport( prevVPX, prevVPY, prevVPWidth, prevVPHeight );
	gRDI->setRenderBuffer( prevRendBuf );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
}


// =================================================================================================
// Occlusion Culling
// =================================================================================================

int Renderer::registerOccSet()
{
	for( int i = 0; i < (int)_occSets.size(); ++i )
	{
		if( _occSets[i] == 0 )
		{
			_occSets[i] = 1;
			return i;
		}
	}

	_occSets.push_back( 1 );
	return (int)_occSets.size() - 1;
}


void Renderer::unregisterOccSet( int occSet )
{
	if( occSet >= 0 && occSet < (int)_occSets.size() )
		_occSets[occSet] = 0;
}


void Renderer::drawOccProxies( uint32 list )
{
	ASSERT( list < 2 );
	
	GLboolean colMask[4], depthMask;
	glGetBooleanv( GL_COLOR_WRITEMASK, colMask );
	glGetBooleanv( GL_DEPTH_WRITEMASK, &depthMask );
	
	setMaterial( 0x0, "" );
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glDepthMask( GL_FALSE );
	
	setShaderComb( &Modules::renderer()._defColorShader );
	commitGeneralUniforms();
	gRDI->setVertexBuffer( 0, _vbCube, 0, 12 );
	gRDI->setIndexBuffer( _ibCube, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	// Draw occlusion proxies
	for( size_t i = 0, s = _occProxies[list].size(); i < s; ++i )
	{
		OccProxy &proxy = _occProxies[list][i];

		gRDI->beginQuery( proxy.queryObj );
		
		Matrix4f mat = Matrix4f::TransMat( proxy.bbMin.x, proxy.bbMin.y, proxy.bbMin.z ) *
			Matrix4f::ScaleMat( proxy.bbMax.x - proxy.bbMin.x, proxy.bbMax.y - proxy.bbMin.y, proxy.bbMax.z - proxy.bbMin.z );
		gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );

		// Draw AABB
		gRDI->drawIndexed( PRIM_TRILIST, 0, 36, 0, 8 );

		gRDI->endQuery( proxy.queryObj );
	}

	setShaderComb( 0x0 );
	glDepthMask( depthMask );
	glColorMask( colMask[0], colMask[1], colMask[2], colMask[3] );

	_occProxies[list].resize( 0 );
}


// =================================================================================================
// Overlays
// =================================================================================================

void Renderer::showOverlays( const float *verts, uint32 vertCount, float *colRGBA,
                             MaterialResource *matRes, int flags )
{
	uint32 numOverlayVerts = 0;
	if( !_overlayBatches.empty() )
		numOverlayVerts = _overlayBatches.back().firstVert + _overlayBatches.back().vertCount;
	
	if( numOverlayVerts + vertCount > MaxNumOverlayVerts ) return;

	memcpy( &_overlayVerts[numOverlayVerts], verts, vertCount * sizeof( OverlayVert ) );
	
	// Check if previous batch can be extended
	if( !_overlayBatches.empty() )
	{
		OverlayBatch &prevBatch = _overlayBatches.back();
		if( matRes == prevBatch.materialRes && flags == prevBatch.flags &&
			memcmp( colRGBA, prevBatch.colRGBA, 4 * sizeof( float ) ) == 0 )
		{
			prevBatch.vertCount += vertCount;
			return;
		}
	}
	
	// Create new batch
	_overlayBatches.push_back( OverlayBatch( numOverlayVerts, vertCount, colRGBA, matRes, flags ) );
}


void Renderer::clearOverlays()
{
	_overlayBatches.resize( 0 );
}


void Renderer::drawOverlays( std::string const& shaderContext )
{
	uint32 numOverlayVerts = 0;
   float desiredAspect = Modules::config().overlayAspect;
	if( !_overlayBatches.empty() )
		numOverlayVerts = _overlayBatches.back().firstVert + _overlayBatches.back().vertCount;
	
	if( numOverlayVerts == 0 ) return;
	
	// Upload overlay vertices
	gRDI->updateBufferData( _overlayVB, 0, numOverlayVerts * sizeof( OverlayVert ), _overlayVerts );

	gRDI->setVertexBuffer( 0, _overlayVB, 0, sizeof( OverlayVert ) );
	gRDI->setIndexBuffer( _quadIdxBuf, IDXFMT_16 );
	ASSERT( QuadIndexBufCount >= MaxNumOverlayVerts * 6 );

   setupViewMatrices( Matrix4f(), Matrix4f::OrthoMat(0, desiredAspect, 1.0, 0, -1, 1 ) );
	
	MaterialResource *curMatRes = 0x0;
	
	gRDI->setVertexLayout( _vlOverlay );
	for(auto& ob : _overlayBatches)
	{		
		if( curMatRes != ob.materialRes )
		{
			if( !setMaterial( ob.materialRes, shaderContext ) ) continue;
			curMatRes = ob.materialRes;
		}
		
		if( _curShader->uni_olayColor >= 0 )
			gRDI->setShaderConst( _curShader->uni_olayColor, CONST_FLOAT4, ob.colRGBA );
		
		// Draw batch
		gRDI->drawIndexed( PRIM_TRILIST, ob.firstVert * 6/4, ob.vertCount * 6/4, ob.firstVert, ob.vertCount );
	}
}


// =================================================================================================
// Pipeline Functions
// =================================================================================================

void Renderer::bindPipeBuffer( uint32 rbObj, std::string const& sampler, uint32 bufIndex )
{
	if( rbObj == 0 )
	{
		// Clear buffer bindings
		_pipeSamplerBindings.resize( 0 );
	}
	else
	{
		// Check if binding is already existing
		for( size_t i = 0, s = _pipeSamplerBindings.size(); i < s; ++i )
		{
			if( strcmp( _pipeSamplerBindings[i].sampler, sampler.c_str() ) == 0 )
			{
				_pipeSamplerBindings[i].rbObj = rbObj;
				_pipeSamplerBindings[i].bufIndex = bufIndex;
				return;
			}
		}
		
		// Add binding
		_pipeSamplerBindings.push_back( PipeSamplerBinding() );
		size_t len = std::min( sampler.length(), (size_t)63 );
		strncpy_s( _pipeSamplerBindings.back().sampler, 63, sampler.c_str(), len );
		_pipeSamplerBindings.back().sampler[len] = '\0';
		_pipeSamplerBindings.back().rbObj = rbObj;
		_pipeSamplerBindings.back().bufIndex = bufIndex;
	}
}


void Renderer::clear( bool depth, bool buf0, bool buf1, bool buf2, bool buf3,
                      float r, float g, float b, float a, int stencilVal )
{
	uint32 mask = 0;
	uint32 prevBuffers[4] = { 0 };
	float clrColor[] = { r, g, b, a };

	glDisable( GL_BLEND );	// Clearing floating point buffers causes problems when blending is enabled on Radeon 9600
	glDepthMask( GL_TRUE );

	if( gRDI->_curRendBuf != 0x0 )
	{
		// Store state of glDrawBuffers
		for( uint32 i = 0; i < 4; ++i ) {
         glGetIntegerv( GL_DRAW_BUFFER0 + i, (int *)&prevBuffers[i] );
      }
		
		RDIRenderBuffer &rb = gRDI->_rendBufs.getRef( gRDI->_curRendBuf );
		uint32 buffers[4], cnt = 0;

		if( depth && rb.depthTex != 0 ) mask |= CLR_DEPTH;
      if( stencilVal >= 0 && rb.depthTex != 0 ) mask |= CLR_STENCIL;

		if( buf0 && rb.colTexs[0] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT0_EXT;
		if( buf1 && rb.colTexs[1] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT1_EXT;
		if( buf2 && rb.colTexs[2] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT2_EXT;
		if( buf3 && rb.colTexs[3] != 0 ) buffers[cnt++] = GL_COLOR_ATTACHMENT3_EXT;

		if( cnt > 0 )
		{	
			mask |= CLR_COLOR;
			glDrawBuffers( cnt, buffers );
		}
	}
	else
	{
		if( depth ) mask |= CLR_DEPTH;
		if( buf0 ) mask |= CLR_COLOR;
      if( stencilVal >= 0 ) mask |= CLR_STENCIL;
		//gRDI->setScissorRect( _curCamera->_vpX, _curCamera->_vpY, _curCamera->_vpWidth, _curCamera->_vpHeight );
		//glEnable( GL_SCISSOR_TEST );
	}
	
	gRDI->clear( mask, clrColor, 1.f, stencilVal );
	//glDisable( GL_SCISSOR_TEST );
	
	// Restore state of glDrawBuffers
	if( gRDI->_curRendBuf != 0x0 ) {
		glDrawBuffers( 4, prevBuffers );
   }
}

void Renderer::drawMatSphere(Resource *matRes, std::string const& shaderContext, const Vec3f &pos, float radius)
{

   setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
   if( !setMaterial( (MaterialResource *)matRes, shaderContext ) ) return;
	Matrix4f mat = Matrix4f::TransMat( pos.x, pos.y, pos.z ) *
	               Matrix4f::ScaleMat( radius, radius, radius );
	gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
	
	gRDI->setVertexBuffer( 0, _vbSphere, 0, 12 );
	gRDI->setIndexBuffer( _ibSphere, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->drawIndexed( PRIM_TRILIST, 0, 128 * 3, 0, 126 );
}

void Renderer::drawFSQuad( Resource *matRes, std::string const& shaderContext )
{
	if( matRes == 0x0 || matRes->getType() != ResourceTypes::Material ) return;

	setupViewMatrices( _curCamera->getViewMat(), Matrix4f::OrthoMat( 0, 1, 0, 1, -1, 1 ) );
	
	if( !setMaterial( (MaterialResource *)matRes, shaderContext ) ) return;

	gRDI->setVertexBuffer( 0, _vbFSPoly, 0, 12 );
	gRDI->setIndexBuffer( 0, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->draw( PRIM_TRILIST, 0, 3 );
}

void Renderer::drawQuad()
{
	ASSERT( _curShader != 0x0 );	
	gRDI->setVertexBuffer( 0, _vbFSPoly, 0, 12 );
	gRDI->setIndexBuffer( 0, IDXFMT_16 );
	gRDI->setVertexLayout( _vlPosOnly );

	gRDI->draw( PRIM_TRILIST, 0, 3 );
}


void Renderer::updateLodUniform(int lodLevel, float lodDist1, float lodDist2)
{
   float nearV = _curCamera->getParamF(CameraNodeParams::NearPlaneF, 0);
   float farV = _curCamera->getParamF(CameraNodeParams::FarPlaneF, 0);
   _lodValues.x = (float)lodLevel;
   _lodValues.y = (1.0f - lodDist1) * nearV + (lodDist1 * farV);
   _lodValues.z = (1.0f - lodDist2) * nearV + (lodDist2 * farV);
   _lodValues.w = (_lodValues.y - _lodValues.z);
}

void Renderer::drawLodGeometry(SceneId sceneId, std::string const& shaderContext,
                             RenderingOrder::List order, int filterRequried, int occSet, float frustStart, float frustEnd, int lodLevel, Frustum const* lightFrus)
{
   // These two magic values represent the normalized distances to the end of the first LOD level,
   // and the beginning of the second LOD level.  A larger first value implies an overlap between
   // the two, which is desirable for blending smoothly between the two levels.
   updateLodUniform(lodLevel, 0.41f, 0.39f);

   Frustum f = _curCamera->getFrustum();

   if (frustStart != 0.0f || frustEnd != 1.0f) {
      float fStart = (1.0f - frustStart) * _curCamera->_frustNear + (frustStart * _curCamera->_frustFar);
      float fEnd = (1.0f - frustEnd) * _curCamera->_frustNear + (frustEnd * _curCamera->_frustFar);

      f.buildViewFrustum(_curCamera->getAbsTrans(), _curCamera->getParamF(CameraNodeParams::FOVf, 0), 
         _curCamera->_vpWidth / (float)_curCamera->_vpHeight, fStart, fEnd);
   }

   R_LOG(7) << "updating geometry queue";

   Modules::sceneMan().sceneForId(sceneId).updateQueues("drawing geometry", f, lightFrus, order, SceneNodeFlags::NoDraw, 
      filterRequried, false, true );
	
	setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
	drawRenderables(sceneId, shaderContext, false, &_curCamera->getFrustum(), 0x0, order, occSet, lodLevel );
}


void Renderer::drawGeometry(SceneId sceneId, std::string const& shaderContext,
                             RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd, int forceLodLevel, Frustum const* lightFrus)
{
   // TODO: all these 'forceLodLevel' settings look like a joke, but rationalizing this between the forward and deferred renderers
   // is tricky.  Sit and think and fix this.
   R_LOG(5) << "drawing geometry (shader:" << shaderContext << " lod:" << forceLodLevel << ")";

   if (forceLodLevel >= 0) {
      drawLodGeometry(sceneId, shaderContext, order, filterRequired, occSet, frustStart, frustEnd, forceLodLevel, lightFrus);
   } else {
      if (forceLodLevel == -1) {
         _lod_polygon_offset_x = 0.0;
         _lod_polygon_offset_y = -2.0;
      }

      float fStart = std::max(frustStart, 0.0f);
      float fEnd = std::min(0.41f, frustEnd);
      if (fStart < fEnd) {
         drawLodGeometry(sceneId, shaderContext, order, filterRequired, occSet, fStart, fEnd, 0, lightFrus);
      }

      if (forceLodLevel == -3) {
         _lod_polygon_offset_x = 0.0;
         _lod_polygon_offset_y = -2.0;
      }

      fStart = std::max(frustStart, 0.39f);
      fEnd = std::min(1.0f, frustEnd);
      if (fStart < fEnd) {
         drawLodGeometry(sceneId, shaderContext, order, filterRequired, occSet, fStart, fEnd, 1, lightFrus);
      }

      _lod_polygon_offset_x = 0.0;
      _lod_polygon_offset_y = 0.0;
   }
}

void Renderer::drawSelected(SceneId sceneId, std::string const& shaderContext,
                             RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd, int forceLodLevel, Frustum const* lightFrus)
{
   std::vector<NodeHandle> toRemove;
   _lod_polygon_offset_x = -1.0;
   _lod_polygon_offset_y = -4.0;
   Frustum f = _curCamera->getFrustum();
   for (SelectedNode &n : _selectedNodes) {
      SceneNode *np = Modules::sceneMan().resolveNodeHandle(n.h);
      if (np) {
         // We load up each selected node (and all descendents) and render them, one root at a time.
         Modules::sceneMan().sceneForId(sceneId).updateQueuesWithNode(*np, f);

         // Update every selected element's selection color.
         for (auto& q: Modules::sceneMan().sceneForId(sceneId).getRenderableQueues()) {
            for (auto &rn : q.second) {
               // Sigh.  The proper fix is: fix Horde.  Once culling is fixed, the result will be a list of queue items that expose,
               // amongst other necessities, a material, and then everything will Just Work.
               int matResHandle = rn.node->getParamI(VoxelMeshNodeParams::MatResI);
               if (matResHandle > 0) {
	               Resource *resObj = Modules::resMan().resolveResHandle(matResHandle);

                  ((MaterialResource *)resObj)->setUniform("selected_color", n.color.x, n.color.y, n.color.z, 1.0);
                  ((MaterialResource *)resObj)->setUniform("selected_color_fast", n.color.x * 0.5f, n.color.y * 0.5f, n.color.z * 0.5f, 1.0);
               }
            }
         }

         for (auto& q: Modules::sceneMan().sceneForId(sceneId).getInstanceRenderableQueues()) {
            for (auto &irq : q.second) {
               for (auto &rn : irq.second) {
                  // Sigh.  The proper fix is: fix Horde.  Once culling is fixed, the result will be a list of queue items that expose,
                  // amongst other necessities, a material, and then everything will Just Work.
                  int matResHandle = rn.node->getParamI(VoxelMeshNodeParams::MatResI);
                  if (matResHandle > 0) {
	                  Resource *resObj = Modules::resMan().resolveResHandle(matResHandle);

                     ((MaterialResource *)resObj)->setUniform("selected_color", n.color.x, n.color.y, n.color.z, 1.0);
                     ((MaterialResource *)resObj)->setUniform("selected_color_fast", n.color.x * 0.5f, n.color.y * 0.5f, n.color.z * 0.5f, 1.0);
                  }
               }
            }
         }
         updateLodUniform(0, 0.41f, 0.39f);

	      setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
	      drawRenderables(sceneId, shaderContext, false, &_curCamera->getFrustum(), 0x0, order, occSet, 1);
      } else {
         toRemove.push_back(n.h);
      }
   }

   for (NodeHandle nh : toRemove) {
      removeSelectedNode(nh);
   }
   _lod_polygon_offset_x = 0.0;
   _lod_polygon_offset_y = 0.0;
}


void Renderer::drawProjections(SceneId sceneId, std::string const& shaderContext, uint32 userFlags )
{
   Scene& scene = Modules::sceneMan().sceneForId(sceneId);
   int numProjectorNodes = scene.findNodes(scene.getRootNode(), "", SceneNodeTypes::ProjectorNode);

   setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );

   Matrix4f m;
   m.toIdentity();
   for (int i = 0; i < numProjectorNodes; i++)
   {
      ProjectorNode* n = (ProjectorNode*)scene.getFindResult(i);
      
      _materialOverride = n->getMaterialRes();
      Frustum f;
      const BoundingBox& b = n->getBBox();
      f.buildBoxFrustum(m, b.min().x, b.max().x, b.min().y, b.max().y, b.min().z, b.max().z);
      scene.updateQueues("drawing geometry", f, 0x0, RenderingOrder::None,
	                                    SceneNodeFlags::NoDraw, 0, false, true, false, userFlags);

      _projectorMat = n->getAbsTrans();

      // Don't need higher poly counts for projection geometry, so render at lod level 1.
      drawRenderables(sceneId, shaderContext, false, &_curCamera->getFrustum(), 0x0, RenderingOrder::None, -1, 1);
   }

   _materialOverride = 0x0;
}


void Renderer::computeTightCameraBounds(SceneId sceneId, float* minDist, float* maxDist)
{
   float defaultMax = *maxDist;
   float defaultMin = *minDist;
   *maxDist = defaultMin;
   *minDist = defaultMax;

   Scene& scene = Modules::sceneMan().sceneForId(sceneId);

   // First, get all the visible objects in the full camera's frustum.
   BoundingBox visibleAabb;
   scene.updateQueues("computing tight camera", _curCamera->getFrustum(), 0x0,
      RenderingOrder::None, SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, true);
   for( const auto& queue : scene.getRenderableQueues() )
   {
      for (const auto& entry : queue.second)
      {
         SceneNode const* n = entry.node;
	      visibleAabb.makeUnion(n->getBBox());
      }
   }

   if (!visibleAabb.isValid())
   {
      return;
   }

   // Tightly clip the resulting AABB of visible geometry against the frustum.
   std::vector<Polygon> clippedAabb;
   _curCamera->getFrustum().clipAABB(visibleAabb, &clippedAabb);

   // Extract the maximum distance of the clipped fragments.
   for (const auto& poly : clippedAabb)
   {
      gRDI->_frameDebugInfo.addPolygon_(poly);
      for (const auto& point : poly.points())
      {
         float dist = -(_curCamera->getViewMat() * point).z;
         if(dist > *maxDist)
         {
            *maxDist = dist;
         }

         if (dist < *minDist)
         {
            *minDist = dist;
         }
      }
   }

   if (*minDist < defaultMin) {
      *minDist = defaultMin;
      return;
   }

   // Check the near-plane--if it's even partially inside the visible AABB, then
   // just revert to the near-plane as the min-dist.
   for (int i = 0; i < 4; i++)
   {
      if (visibleAabb.contains(_curCamera->getFrustum().getCorner(i))) 
      {
         *minDist = defaultMin;
         break;
      }
   }
}


Frustum Renderer::computeDirectionalLightFrustum(LightNode const* light, float nearPlaneDist, float farPlaneDist) const
{
   Frustum result;
   // Construct a tighter camera frustum, given the supplied far plane value.
   Frustum tightCamFrust;
   float f = nearPlaneDist / _curCamera->_frustNear;
   tightCamFrust.buildViewFrustum(_curCamera->_absTrans, _curCamera->_frustLeft * f, _curCamera->_frustRight * f, 
      _curCamera->_frustBottom * f, _curCamera->_frustTop * f, nearPlaneDist, farPlaneDist);

   // Transform the camera frustum into light space, building a new AABB as we do so.
   BoundingBox bb;
   for (int i = 0; i < 8; i++)
   {
      bb.addPoint(light->getViewMat() * tightCamFrust.getCorner(i));
   }

   // Ten-thousand units ought to be enough for anyone.  (This value is how far our light is from
   // it's center-point; ideally, we'll always over-estimate here, and then dynamically adjust during
   // shadow-map construction to a tight bound over the light-visible geometry.)
   bb.addPoint(Vec3f(bb.min().x, bb.min().y, 10000));

   result.buildBoxFrustum(light->getViewMat().inverted(), bb.min().x, bb.max().x, bb.min().y, bb.max().y, bb.max().z, bb.min().z);

   return result;
}

struct LightImportanceSortPred
{
   Matrix4f const& clipMat;
   Vec3f const& camPos;
   LightImportanceSortPred(Matrix4f const& m, Vec3f const& cp) : clipMat(m), camPos(cp) {
   }

	bool operator()( LightNode*const& a, LightNode*const& b ) const
   {
      float x, y, w, h;
      float aArea = 1.0, bArea = 1.0;
      
      if (!a->getParamI(LightNodeParams::DirectionalI)) {
         a->calcScreenSpaceAABB(clipMat, x, y, w, h);
         aArea = w * h;
      }

      if (!b->getParamI(LightNodeParams::DirectionalI)) {
         b->calcScreenSpaceAABB(clipMat, x, y, w, h);
         bArea = w * h;
      }

      if (aArea == bArea) {
         Vec3f l1 = (camPos - a->getAbsPos());
         float d1 = l1.dot(l1);

         Vec3f l2 = (camPos - b->getAbsPos());
         float d2 = l2.dot(l2);

         return d1 <= d2;
      }

      return aArea > bArea;
   }
};


void Renderer::prioritizeLights(SceneId sceneId, std::vector<LightNode*>* lights)
{
   std::vector<LightNode*> high, low;

   unsigned int maxLights = (unsigned int)Modules::config().maxLights;
   // Light prioritization:
   // All required lights must be included, regardless of their size/distance from the viewer.
   // As many high-importance lights as can fit, in order of f(screen_area, distance_from_viewer).
   // As many low-importance lights as can fit, in order of f(screen_area, distance_from_viewer).
   // Right now, we don't worry about distance to the viewer; let's see how it looks....

	for(auto const& entry : Modules::sceneMan().sceneForId(sceneId).getLightQueue())
	{
		LightNode* curLight = (LightNode*)entry;
      if (curLight->_importance == LightNodeImportance::Required) {
         // Just add all required lights, immediately, regardless of the maxLight cap.
         lights->push_back(curLight);
      } else if (curLight->_importance == LightNodeImportance::High) {
         high.push_back(curLight);
      } else {
         low.push_back(curLight);
      }
   }

   Matrix4f clipMat = _curCamera->getProjMat() * _curCamera->getViewMat();
   
   // Sort high importance lights.
   std::sort(high.begin(), high.end(), LightImportanceSortPred(clipMat, _curCamera->getAbsPos()));
   for (auto const& entry : high) {
      if (lights->size() >= maxLights) {
         return;
      }
      lights->push_back(entry);
   }


   // Sort low importance lights.
   std::sort(low.begin(), low.end(), LightImportanceSortPred(clipMat, _curCamera->getAbsPos()));
   for (auto const& entry : low) {
      if (lights->size() >= maxLights) {
         return;
      }
      lights->push_back(entry);
   }
}

void Renderer::doForwardLightPass(SceneId sceneId, std::string const& contextSuffix,
                                  bool noShadows, RenderingOrder::List order, int occSet, bool selectedOnly, int lodLevel)
{
   Modules::sceneMan().sceneForId(sceneId).updateQueues("drawing light geometry", _curCamera->getFrustum(), 0x0, order,
      SceneNodeFlags::NoDraw, 0, true, false);

   std::vector<LightNode*> prioritizedLights;
   prioritizeLights(sceneId, &prioritizedLights);

   // For shadows, knowing the tightest extents (surrounding geometry) of the front and back of the frustum can 
   // help us greatly with increasing shadow precision, so compute them if needed.
   float maxDist = _curCamera->_frustFar;
   float minDist = _curCamera->_frustNear;
   if (!noShadows) 
   {
      computeTightCameraBounds(sceneId, &minDist, &maxDist);
   }

   for (const auto& curLight : prioritizedLights)
   {
      Frustum const* lightFrus;
      Frustum dirLightFrus;

      if (curLight->_directional)
      {
         if (noShadows)
         {
            // If we're a directional light, and no shadows are to be cast, then only light
            // what is visible.
            lightFrus = nullptr;
         } else {
            dirLightFrus = computeDirectionalLightFrustum(curLight, minDist, maxDist);
            lightFrus = &dirLightFrus;
         }
      } else {
         lightFrus = &(curLight->getFrustum());
      }

      if (lightFrus && _curCamera->getFrustum().cullFrustum(*lightFrus)) {
         continue;
      }

      // Calculate light screen space position
      float bbx, bby, bbw, bbh;
      curLight->calcScreenSpaceAABB(_curCamera->getProjMat() * _curCamera->getViewMat(), bbx, bby, bbw, bbh);

      if (noShadows || curLight->_shadowMapCount == 0)
      {
         // Bind the trivial shadow map.
         // TODO: this is actually pointless, since in-shader, we should never actually reference
         // the shadow map in an un-shadowed pass.
         setupShadowMap(curLight, true);

         commitLightUniforms(curLight);
         // Set scissor rectangle
         if (bbx != 0 || bby != 0 || bbw != 1 || bbh != 1)
         {
            gRDI->setScissorRect(ftoi_r(bbx * gRDI->_fbWidth), ftoi_r(bby * gRDI->_fbHeight), ftoi_r(bbw * gRDI->_fbWidth), ftoi_r(bbh * gRDI->_fbHeight));
            glEnable(GL_SCISSOR_TEST);
         }

         drawGeometry(sceneId, curLight->_lightingContext + "_" + contextSuffix, order, 0, occSet, 0.0, 1.0, lodLevel, lightFrus);
      } else {
         if (curLight->_shadowMapBuffer) {
            if (curLight->_directional && curLight->_shadowMapBuffer) {
               updateShadowMap(curLight, lightFrus, minDist, maxDist);
               setupShadowMap(curLight, false);
               commitLightUniforms(curLight);
               // Set scissor rectangle
               if (bbx != 0 || bby != 0 || bbw != 1 || bbh != 1)
               {
                  gRDI->setScissorRect(ftoi_r(bbx * gRDI->_fbWidth), ftoi_r(bby * gRDI->_fbHeight), ftoi_r(bbw * gRDI->_fbWidth), ftoi_r(bbh * gRDI->_fbHeight));
                  glEnable(GL_SCISSOR_TEST);
               }

               drawGeometry(sceneId, curLight->_lightingContext + "_" + contextSuffix, order, 0, occSet, 0.0, 1.0, lodLevel, lightFrus);
            } else {
               // Omni lights require a pass/binding for each side of the cubemap into which they render.
               for (int i = 0; i < 6; i++) {
                  if (curLight->_dirtyShadows[i]) {
                     curLight->_dirtyShadows[i] = false;
                     updateShadowMap(curLight, lightFrus, minDist, maxDist, i);
                  }
               }

               setupShadowMap(curLight, false);
               commitLightUniforms(curLight);
               // Set scissor rectangle
               if (bbx != 0 || bby != 0 || bbw != 1 || bbh != 1)
               {
                  gRDI->setScissorRect(ftoi_r(bbx * gRDI->_fbWidth), ftoi_r(bby * gRDI->_fbHeight), ftoi_r(bbw * gRDI->_fbWidth), ftoi_r(bbh * gRDI->_fbHeight));
                  glEnable(GL_SCISSOR_TEST);
               }

               // Luckily, we only need one pass to actually apply the shadowing.
               drawGeometry(sceneId, curLight->_lightingContext + "_" + contextSuffix, order, 0, occSet, 0.0, 1.0, lodLevel, lightFrus);
            }
         }
      }
		
      Modules().stats().incStat(EngineStats::LightPassCount, 1);

      // Reset
      glDisable(GL_SCISSOR_TEST);
   }
}


void Renderer::doDeferredLightPass(SceneId sceneId, bool noShadows, MaterialResource *deferredMaterial)
{
	Modules::sceneMan().sceneForId(sceneId).updateQueues( "drawing light shapes", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
	                                  SceneNodeFlags::NoDraw, 0, true, false );
   std::vector<LightNode*> prioritizedLights;
   prioritizeLights(sceneId, &prioritizedLights);

   // For shadows, knowing the tightest extents (surrounding geometry) of the front and back of the frustum can 
   // help us greatly with increasing shadow precision, so compute them if needed.
   float maxDist = _curCamera->_frustFar;
   float minDist = _curCamera->_frustNear;
   if (!noShadows) 
   {
      computeTightCameraBounds(sceneId, &minDist, &maxDist);
   }

   for (const auto& curLight : prioritizedLights)
   {
      Frustum const* lightFrus;
      Frustum dirLightFrus;

      if (curLight->_directional)
      {
         if (noShadows)
         {
            // If we're a directional light, and no shadows are to be cast, then only light
            // what is visible.
            lightFrus = nullptr;
         } else {
            dirLightFrus = computeDirectionalLightFrustum(curLight, minDist, maxDist);
            lightFrus = &dirLightFrus;
         }
      } else {
         lightFrus = &(curLight->getFrustum());
      }

      if (lightFrus && _curCamera->getFrustum().cullFrustum(*lightFrus)) {
         continue;
      }

      if (noShadows || curLight->_shadowMapCount == 0)
      {
         // Bind the trivial shadow map.
         // TODO: this is actually pointless, since in-shader, we should never actually reference
         // the shadow map in an un-shadowed pass.
         setupShadowMap(curLight, true);
         commitLightUniforms(curLight);         
      } else {
         if (curLight->_shadowMapBuffer) {
            if (curLight->_directional && curLight->_shadowMapBuffer) {
               updateShadowMap(curLight, lightFrus, minDist, maxDist);
               setupShadowMap(curLight, false);
               commitLightUniforms(curLight);
            } else {
               // Omni lights require a pass/binding for each side of the cubemap into which they render.
               for (int i = 0; i < 6; i++) {
                  if (curLight->_dirtyShadows[i]) {
                     curLight->_dirtyShadows[i] = false;
                     updateShadowMap(curLight, lightFrus, minDist, maxDist, i);
                  }
               }

               setupShadowMap(curLight, false);
               commitLightUniforms(curLight);
            }
         }
      }

      if (curLight->_directional) {
         drawFSQuad(deferredMaterial, curLight->_lightingContext + "_" + _curPipeline->_pipelineName);
      } else {
         // Calculate light screen space position
         float bbx, bby, bbw, bbh;
         curLight->calcScreenSpaceAABB(_curCamera->getProjMat() * _curCamera->getViewMat(), bbx, bby, bbw, bbh);
         // Set scissor rectangle
         if (bbw == 1.0 && bbh == 1.0)
         {
            drawMatSphere(deferredMaterial, curLight->_lightingContext + "_" + _curPipeline->_pipelineName + "_INSIDE", curLight->getAbsPos(), curLight->getRadius() + 1.0f);
         } else {
            drawMatSphere(deferredMaterial, curLight->_lightingContext + "_" + _curPipeline->_pipelineName, curLight->getAbsPos(), curLight->getRadius() + 1.0f);
         }
      }

      Modules().stats().incStat(EngineStats::LightPassCount, 1);
	}
}


// =================================================================================================
// Scene Node Rendering Functions
// =================================================================================================

void Renderer::setGlobalUniform(const char* str, UniformType::List kind, void const* value, int num)
{
   float *f = (float*)value;
   if (kind == UniformType::FLOAT) {
      _uniformFloats[std::string(str)] = *f;
   } else if (kind == UniformType::VEC4) {
      _uniformVecs[std::string(str)] = Vec4f(f[0], f[1], f[2], f[3]);
   } else if (kind == UniformType::MAT44) {
      _uniformMats[std::string(str)] = Matrix4f(f);
   } else if (kind == UniformType::VEC3) {
      _uniformVec3s[std::string(str)] = Vec3f(f[0], f[1], f[2]);
   } else if (kind == UniformType::MAT44_ARRAY) {
      std::vector<float>& vs = _uniformMatArrays[std::string(str)];
      vs.clear();
      vs.reserve(num * 16);
      for (int i = 0; i < 16 * num; i++) {
         vs.push_back(f[i]);
      }
   }
}


void Renderer::drawRenderables(SceneId sceneId, std::string const& shaderContext, bool debugView,
                                const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                                int occSet, int lodLevel )
{
	ASSERT( _curCamera != 0x0 );
	
   R_LOG(7) << "drawing renderables (shader:" << shaderContext << " lod:" << lodLevel;

	if( Modules::config().wireframeMode && !Modules::config().debugViewMode )
	{
		glDisable( GL_CULL_FACE );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}
	else
	{
		glEnable( GL_CULL_FACE );
	}
	
   for (auto& regType : Modules::sceneMan().sceneForId(sceneId)._registry)
	{
		if( regType.second.renderFunc != 0x0 ) {
			(*regType.second.renderFunc)(sceneId, shaderContext, debugView, frust1, frust2, order, occSet, lodLevel );
      }

      if (regType.second.instanceRenderFunc != 0x0) {
         (*regType.second.instanceRenderFunc)(sceneId, shaderContext, debugView, frust1, frust2, order, occSet, lodLevel );
      }
	}

	// Reset states
	if( Modules::config().wireframeMode && !Modules::config().debugViewMode )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}


void Renderer::drawMeshes(SceneId sceneId, std::string const& shaderContext, bool debugView,
                           const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                           int occSet, int lodLevel )
{
	if( frust1 == 0x0 ) return;
	
	GeometryResource *curGeoRes = 0x0;
	MaterialResource *curMatRes = 0x0;

   R_LOG(9) << "drawing meshes (shader:" << shaderContext << " lod:" << lodLevel << ")";


   Modules::config().setGlobalShaderFlag("DRAW_WITH_INSTANCING", false);
   Modules::config().setGlobalShaderFlag("DRAW_SKINNED", false);
	// Loop over mesh queue
	for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::Mesh) )
	{
		MeshNode *meshNode = (MeshNode *)entry.node;
		ModelNode *modelNode = meshNode->getParentModel();

      #define RENDER_LOG() R_LOG(9) << " mesh " << meshNode->_handle << ": " 
		
		// Check that mesh is valid
		if( modelNode->getGeometryResource() == 0x0 ) {
                        RENDER_LOG() << "geometry not loaded.  ignoring.";
			continue;
      }
		if( meshNode->getBatchStart() + meshNode->getBatchCount() > modelNode->getGeometryResource()->_indexCount ) {
         RENDER_LOG() << "geometry not loaded for lod level " << lodLevel << ".  ignoring.";
			continue;
      }
		
      bool modelChanged = true;
		uint32 queryObj = 0;
		
		// Bind geometry
      RENDER_LOG() << "binding geometry...";
		if( curGeoRes != modelNode->getGeometryResource() )
		{
			curGeoRes = modelNode->getGeometryResource();
			ASSERT( curGeoRes != 0x0 );
		
			// Indices
         gRDI->setIndexBuffer(curGeoRes->getIndexBuf(), curGeoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

			// Vertices
			uint32 posVBuf = curGeoRes->getPosVBuf();
			uint32 tanVBuf = curGeoRes->getTanVBuf();
			uint32 staticVBuf = curGeoRes->getStaticVBuf();
			
			gRDI->setVertexBuffer( 0, posVBuf, 0, sizeof( Vec3f ) );
			gRDI->setVertexBuffer( 1, tanVBuf, 0, sizeof( VertexDataTan ) );
			gRDI->setVertexBuffer( 2, tanVBuf, sizeof( Vec3f ), sizeof( VertexDataTan ) );
			gRDI->setVertexBuffer( 3, staticVBuf, 0, sizeof( VertexDataStatic ) );
		}

		gRDI->setVertexLayout( Modules::renderer()._vlModel );
		
		ShaderCombination *prevShader = Modules::renderer().getCurShader();
		
		if( !debugView )
		{
			// Set material
			if( curMatRes != meshNode->getMaterialRes() )
			{
				if( !Modules::renderer().setMaterial( meshNode->getMaterialRes(), shaderContext ) )
				{	
               RENDER_LOG() << "no material for context " << shaderContext << ".  ignoring.";
					curMatRes = 0x0;
					continue;
				}
				curMatRes = meshNode->getMaterialRes();
			}
		}
		else
		{
			Modules::renderer().setShaderComb( &Modules::renderer()._defColorShader );
			Modules::renderer().commitGeneralUniforms();
			
			uint32 curLod = meshNode->getLodLevel();
			Vec4f color;
			if( curLod == 0 ) color = Vec4f( 0.5f, 0.75f, 1, 1 );
			else if( curLod == 1 ) color = Vec4f( 0.25f, 0.75, 0.75f, 1 );
			else if( curLod == 2 ) color = Vec4f( 0.25f, 0.75, 0.5f, 1 );
			else if( curLod == 3 ) color = Vec4f( 0.5f, 0.5f, 0.25f, 1 );
			else color = Vec4f( 0.75f, 0.5, 0.25f, 1 );

			// Darken models with skeleton so that bones are more noticable
			if( !modelNode->_jointList.empty() ) color = color * 0.3f;

			gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color.x );
		}

		ShaderCombination *curShader = Modules::renderer().getCurShader();

		if( modelChanged || curShader != prevShader )
		{
			// Skeleton
			if( curShader->uni_skinMatRows >= 0 && !modelNode->_skinMatRows.empty() )
			{
				// Note:	OpenGL 2.1 supports mat4x3 but it is internally realized as mat4 on most
				//			hardware so it would require 4 instead of 3 uniform slots per joint
				
				gRDI->setShaderConst( curShader->uni_skinMatRows, CONST_FLOAT4,
				                      &modelNode->_skinMatRows[0], (int)modelNode->_skinMatRows.size() );
			}

			modelChanged = false;
		}
		
		// World transformation
		if( curShader->uni_worldMat >= 0 )
		{
			gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &meshNode->_absTrans.x[0] );
		}
		if( curShader->uni_worldNormalMat >= 0 )
		{
			// TODO: Optimize this
			Matrix4f normalMat4 = meshNode->_absTrans.inverted().transposed();
			float normalMat[9] = { normalMat4.x[0], normalMat4.x[1], normalMat4.x[2],
			                       normalMat4.x[4], normalMat4.x[5], normalMat4.x[6],
			                       normalMat4.x[8], normalMat4.x[9], normalMat4.x[10] };
			gRDI->setShaderConst( curShader->uni_worldNormalMat, CONST_FLOAT33, normalMat );
		}
		if( curShader->uni_nodeId >= 0 )
		{
			float id = (float)meshNode->getHandle();
			gRDI->setShaderConst( curShader->uni_nodeId, CONST_FLOAT, &id );
		}

		if( queryObj )
			gRDI->beginQuery( queryObj );
		
		// Render
      RENDER_LOG() << "rendering...";
		gRDI->drawIndexed( PRIM_TRILIST, meshNode->getBatchStart(), meshNode->getBatchCount(),
		                   meshNode->getVertRStart(), meshNode->getVertREnd() - meshNode->getVertRStart() + 1 );
		Modules::stats().incStat( EngineStats::BatchCount, 1 );
		Modules::stats().incStat( EngineStats::TriCount, meshNode->getBatchCount() / 3.0f );

		if( queryObj )
			gRDI->endQuery( queryObj );

#undef RENDER_LOG
	}

	gRDI->setVertexLayout( 0 );
}


void Renderer::drawHudElements(SceneId sceneId, std::string const& shaderContext, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   radiant::perfmon::TimelineCounterGuard dvm("drawHudElements");
	if( frust1 == 0x0 ) return;
	
	for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::HudElement) )
	{
      HudElementNode* hudNode = (HudElementNode*) entry.node;
      
      for (const auto& hudElement : hudNode->getSubElements())
      {
         hudElement->draw(shaderContext, hudNode->_absTrans);
      }
   }
	gRDI->setVertexLayout( 0 );
}


void Renderer::drawVoxelMeshes(SceneId sceneId, std::string const& shaderContext, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   radiant::perfmon::TimelineCounterGuard dvm("drawVoxelMeshes");
   if( frust1 == 0x0 ) return;

   VoxelGeometryResource *curVoxelGeoRes = 0x0;
   MaterialResource *curMatRes = 0x0;

   Modules::config().setGlobalShaderFlag("DRAW_SKINNED", true);

   R_LOG(9) << "drawing voxel meshes (shader:" << shaderContext << " lod:" << lodLevel << ")";

   // Loop over mesh queue
   for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::VoxelMesh) )
   {
      VoxelMeshNode *meshNode = (VoxelMeshNode *)entry.node;
      VoxelModelNode *modelNode = meshNode->getParentModel();

#define RENDER_LOG() R_LOG(9) << "  mesh " << meshNode->_handle << " " << modelNode->getParent()->getName() << ": " 

      RENDER_LOG() << "starting";

      // Check that mesh is valid
      if( meshNode->getVoxelGeometryResource() == 0x0 ) {
         RENDER_LOG() << "geometry not loaded.  ignoring.";
         continue;
      }

      if( meshNode->getBatchStart(lodLevel) + meshNode->getBatchCount(lodLevel) > meshNode->getVoxelGeometryResource()->_indexCount ) {
         RENDER_LOG() << "geometry not loaded for lod level " << lodLevel << ".  ignoring.";
         continue;
      }

      // Bind geometry
      RENDER_LOG() << "binding geometry...";
      if( curVoxelGeoRes != meshNode->getVoxelGeometryResource() )
      {
         curVoxelGeoRes = meshNode->getVoxelGeometryResource();
         ASSERT( curVoxelGeoRes != 0x0 );

         // Indices
         gRDI->setIndexBuffer(curVoxelGeoRes->getIndexBuf(), IDXFMT_32);

         // Vertices
         gRDI->setVertexBuffer( 0, curVoxelGeoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );
      }

      gRDI->setVertexLayout( Modules::renderer()._vlVoxelModel );

      ShaderCombination *prevShader = Modules::renderer().getCurShader();

      if( !debugView )
      {
         if (Modules::renderer()._materialOverride != 0x0) {
            if( !Modules::renderer().setMaterial( Modules::renderer()._materialOverride, shaderContext ) )
            {	
               RENDER_LOG() << "no override material for context " << shaderContext << ".  RETURNING!";
               return;
            }
            curMatRes = Modules::renderer()._materialOverride;
         } else {
            // Set material
            if( curMatRes != meshNode->getMaterialRes() )
            {
               if( !Modules::renderer().setMaterial( meshNode->getMaterialRes(), shaderContext ) )
               {	
                  RENDER_LOG() << "no material for context " << shaderContext << ".  ignoring.";
                  curMatRes = 0x0;
                  continue;
               }
               curMatRes = meshNode->getMaterialRes();
            }
         }
      }
      else
      {
         Modules::renderer().setShaderComb( &Modules::renderer()._defColorShader );
         Modules::renderer().commitGeneralUniforms();

         uint32 curLod = lodLevel;
         Vec4f color;
         if( curLod == 0 ) color = Vec4f( 0.5f, 0.75f, 1, 1 );
         else if( curLod == 1 ) color = Vec4f( 0.25f, 0.75, 0.75f, 1 );
         else if( curLod == 2 ) color = Vec4f( 0.25f, 0.75, 0.5f, 1 );
         else if( curLod == 3 ) color = Vec4f( 0.5f, 0.5f, 0.25f, 1 );
         else color = Vec4f( 0.75f, 0.5, 0.25f, 1 );

         gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color.x );
      }

      ShaderCombination *curShader = Modules::renderer().getCurShader();

      // World transformation
      if( curShader->uni_worldMat >= 0 )
      {
         RENDER_LOG() << "setting world matrix to " << "(" << meshNode->_absTrans.c[3][0] << ", " << meshNode->_absTrans.c[3][1] << ", " << meshNode->_absTrans.c[3][2] << ")";

         gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &modelNode->_absTrans.x[0] );
      }
      if( curShader->uni_worldNormalMat >= 0 )
      {
         // TODO: Optimize this
         Matrix4f normalMat4 = meshNode->_absTrans.inverted().transposed();
         float normalMat[9] = { normalMat4.x[0], normalMat4.x[1], normalMat4.x[2],
            normalMat4.x[4], normalMat4.x[5], normalMat4.x[6],
            normalMat4.x[8], normalMat4.x[9], normalMat4.x[10] };
         gRDI->setShaderConst( curShader->uni_worldNormalMat, CONST_FLOAT33, normalMat );
      }
      if( curShader->uni_nodeId >= 0 )
      {
         float id = (float)meshNode->getHandle();
         gRDI->setShaderConst( curShader->uni_nodeId, CONST_FLOAT, &id );
      }

      float lodOffsetX = Modules::renderer()._lod_polygon_offset_x;
      float lodOffsetY = Modules::renderer()._lod_polygon_offset_y;
      // Shadow offsets will always win against the custom model offsets (which we don't care about
      // during a shadow pass.)
      float offset_x, offset_y;
      if (gRDI->getShadowOffsets(&offset_x, &offset_y) || modelNode->getPolygonOffset(offset_x, offset_y))
      {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(
            lodOffsetX + offset_x, 
            lodOffsetY + offset_y);
      } else if (lodOffsetX != 0 || lodOffsetY != 0) {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(lodOffsetX, lodOffsetY);
      } else {
         glDisable(GL_POLYGON_OFFSET_FILL);
      }

      Matrix4f* boneMats;
      if (curShader->uni_bones >= 0) {
         int numBones = (int)modelNode->_boneLookup.size();

         if (numBones == 0) {
            numBones = 1;
            boneMats = &_defaultBoneMat;
         } else {
            boneMats = modelNode->_boneRelTransLookup.data();
         }
         gRDI->setShaderConst(curShader->uni_bones, CONST_FLOAT44, boneMats, numBones);
      }
      if (curShader->uni_modelScale >= 0) {
         gRDI->setShaderConst(curShader->uni_modelScale, CONST_FLOAT, &modelNode->_modelScale, 1);
      }

      // Render
      RENDER_LOG() << "rendering...";
      gRDI->drawIndexed( PRIM_TRILIST, meshNode->getBatchStart(lodLevel), meshNode->getBatchCount(lodLevel),
         meshNode->getVertRStart(lodLevel), meshNode->getVertREnd(lodLevel) - meshNode->getVertRStart(lodLevel) + 1 );
      Modules::stats().incStat( EngineStats::BatchCount, 1 );
      Modules::stats().incStat( EngineStats::TriCount, meshNode->getBatchCount(lodLevel) / 3.0f );

#undef RENDER_LOG
   }

   // Draw occlusion proxies
   if( occSet >= 0 )
      Modules::renderer().drawOccProxies( 0 );

   gRDI->setVertexLayout( 0 );
   glDisable(GL_POLYGON_OFFSET_FILL);
}


void Renderer::drawVoxelMeshes_Instances(SceneId sceneId, std::string const& shaderContext, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   const unsigned int VoxelInstanceCutoff = 2;
   radiant::perfmon::TimelineCounterGuard dvm("drawVoxelMeshes_Instances");
	if( frust1 == 0x0 ) return;
	
	MaterialResource *curMatRes = 0x0;

   R_LOG(9) << "drawVoxelMeshes_Instances (shader:" << shaderContext << " lod:" << lodLevel << ")";
   #define RENDER_LOG() R_LOG(9) << " instance (" \
                                 << "geo:" << curVoxelGeoRes->getHandle() \
                                 << " name:" << curVoxelGeoRes->getName() \
                                 << " vmn:" << (vmn ? vmn->getParentModel()->getParent()->getName() : std::string("?")) \
                                 << ") "

   // Loop over mesh queue
   for( const auto& instanceKind : Modules::sceneMan().sceneForId(sceneId).getInstanceRenderableQueue(SceneNodeTypes::VoxelMesh) )
	{
      const InstanceKey& instanceKey = instanceKind.first;
      const VoxelGeometryResource *curVoxelGeoRes = (VoxelGeometryResource*) instanceKind.first.geoResource;
      VoxelMeshNode* vmn = nullptr;
		// Check that mesh is valid
		if (curVoxelGeoRes == 0x0) {
         R_LOG(9) << "geometry not loaded.  ignoring.";
			continue;
      }

      if (instanceKind.second.size() <= 0) {
         R_LOG(9) << "no instances created for " << curVoxelGeoRes->getName() << " .  ignoring.";
         continue;
      }

      bool useInstancing = instanceKind.second.size() >= VoxelInstanceCutoff && gRDI->getCaps().hasInstancing;
      Modules::config().setGlobalShaderFlag("DRAW_WITH_INSTANCING", useInstancing);
      Modules::config().setGlobalShaderFlag("DRAW_SKINNED", true);

      // TODO(klochek): awful--but how to fix?  We can keep cramming stuff into the InstanceKey, but to what end?
      vmn = (VoxelMeshNode*)instanceKind.second.front().node;
      const VoxelModelNode* voxelModel = vmn->getParentModel();

		// Bind geometry
		// Indices
		gRDI->setIndexBuffer(curVoxelGeoRes->getIndexBuf(), IDXFMT_32);

		// Vertices
      gRDI->setVertexBuffer( 0, curVoxelGeoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );

				
		if( !debugView )
		{
         if (Modules::renderer()._materialOverride != 0x0) {
				if( !Modules::renderer().setMaterial(Modules::renderer()._materialOverride, shaderContext ) )
				{	
               RENDER_LOG() << "no material override for context " << shaderContext << ".  RETURNING!";
               return;
				}
				curMatRes = Modules::renderer()._materialOverride;
         } else {
			   // Set material
			   //if( curMatRes != instanceKey.matResource )
			   //{
               if( !Modules::renderer().setMaterial( instanceKey.matResource, shaderContext ) )
				   {	
                  RENDER_LOG() << "no material for context " << shaderContext << ".  ignoring.";
					   curMatRes = 0x0;
					   continue;
				   }
               curMatRes = instanceKey.matResource;
			   //}
         }
		} else {
			Modules::renderer().setShaderComb( &Modules::renderer()._defColorShader );
			Modules::renderer().commitGeneralUniforms();
			Vec4f color( 0.5f, 0.75f, 1, 1 );
			gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color.x );
		}

		ShaderCombination *curShader = Modules::renderer().getCurShader();

      float lodOffsetX = Modules::renderer()._lod_polygon_offset_x;
      float lodOffsetY = Modules::renderer()._lod_polygon_offset_y;
      // Shadow offsets will always win against the custom model offsets (which we don't care about
      // during a shadow pass.)
      float offset_x, offset_y;
      if (gRDI->getShadowOffsets(&offset_x, &offset_y) || vmn->getParentModel()->getPolygonOffset(offset_x, offset_y))
      {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(
            lodOffsetX + offset_x, 
            lodOffsetY + offset_y);
      } else if (lodOffsetX != 0 || lodOffsetY != 0) {
         glEnable(GL_POLYGON_OFFSET_FILL);
         glPolygonOffset(lodOffsetX, lodOffsetY);
      } else {
         glDisable(GL_POLYGON_OFFSET_FILL);
      }

      Matrix4f* boneMats;
      if (curShader->uni_bones >= 0) {
         int numBones = (int)vmn->getParentModel()->_boneLookup.size();

         if (numBones == 0) {
            numBones = 1;
            boneMats = &_defaultBoneMat;
         } else {
            boneMats = vmn->getParentModel()->_boneRelTransLookup.data();
         }
         gRDI->setShaderConst(curShader->uni_bones, CONST_FLOAT44, boneMats, numBones);
      }

      if (curShader->uni_modelScale >= 0) {
         gRDI->setShaderConst(curShader->uni_modelScale, CONST_FLOAT, &voxelModel->_modelScale, 1);
      }


      RENDER_LOG() << "rendering (use instancing: " << useInstancing << ")";
      if (useInstancing) {
         drawVoxelMesh_Instances_WithInstancing(instanceKind.second, vmn, lodLevel);
      } else {
         drawVoxelMesh_Instances_WithoutInstancing(instanceKind.second, vmn, lodLevel);
      }
#undef RENDER_LOG
   }

	gRDI->setVertexLayout( 0 );
   glDisable(GL_POLYGON_OFFSET_FILL);
}


void Renderer::drawVoxelMesh_Instances_WithInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn, int lodLevel)
{
   // Collect transform data for every node of this mesh/material kind.
   radiant::perfmon::SwitchToCounter("copy mesh instances");
   // Set vertex layout
	gRDI->setVertexLayout( Modules::renderer()._vlInstanceVoxelModel );

   #define RENDER_LOG() R_LOG(9) << " drawing with instancing (" \
                                 << " name:" << vmn->getParentModel()->getParent()->getName() \
                                 << ") "

   unsigned int numQueued = 0;
   uint32 vbInstanceData = 0;
   auto idcIt = _instanceDataCache.find(&renderableQueue);
   if (idcIt != _instanceDataCache.end()) {
      vbInstanceData = idcIt->second;
      numQueued = (unsigned int)renderableQueue.size();
      RENDER_LOG() << "using cached instance data " << vbInstanceData;
   } else {
      vbInstanceData = gRDI->acquireBuffer((int)(sizeof(float) * 16 * renderableQueue.size()));
      RENDER_LOG() << "creating new instance data " << vbInstanceData;
      float* transformBuffer = _vbInstanceVoxelBuf;
      for (const auto& node : renderableQueue) {
         VoxelMeshNode const* meshNode = (VoxelMeshNode*)node.node;
         const SceneNode *transNode = meshNode->getParentModel();
		
         memcpy(transformBuffer, &transNode->_absTrans.x[0], sizeof(float) * 16);
         RENDER_LOG() << "adding world matrix (" << transNode->_absTrans.c[3][0] << ", " << transNode->_absTrans.c[3][1] << ", " << transNode->_absTrans.c[3][2] << ")";
         transformBuffer += 16;
         numQueued++;

         if (numQueued == MaxVoxelInstanceCount) {
            gRDI->updateBufferData(vbInstanceData, 0, (int)((transformBuffer - _vbInstanceVoxelBuf) * sizeof(float)), _vbInstanceVoxelBuf);
            gRDI->setVertexBuffer(1, vbInstanceData, 0, sizeof(float) * 16);
            gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, vmn->getBatchCount(lodLevel), vmn->getBatchStart(lodLevel), MaxVoxelInstanceCount);
            numQueued = 0;
            transformBuffer = _vbInstanceVoxelBuf;
            vbInstanceData = gRDI->acquireBuffer((int)(sizeof(float) * 16 * renderableQueue.size()));
         }
      }

      if (numQueued > 0) {
         gRDI->updateBufferData(vbInstanceData, 0, (int)((transformBuffer - _vbInstanceVoxelBuf) * sizeof(float)), _vbInstanceVoxelBuf);
      }

      if (numQueued == renderableQueue.size()) {
         _instanceDataCache[&renderableQueue] = vbInstanceData;
      }
   }

   if (numQueued > 0) {
      radiant::perfmon::SwitchToCounter("draw mesh instances");
      // Draw instanced meshes.
      gRDI->setVertexBuffer(1, vbInstanceData, 0, sizeof(float) * 16);
      gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, vmn->getBatchCount(lodLevel), vmn->getBatchStart(lodLevel), numQueued);
   }
   radiant::perfmon::SwitchToCounter("drawVoxelMeshes_Instances");

	// Render
	Modules::stats().incStat( EngineStats::BatchCount, 1 );
   Modules::stats().incStat( EngineStats::TriCount, (vmn->getBatchCount(lodLevel) / 3.0f) * renderableQueue.size() );

   #undef RENDER_LOG
}


void Renderer::drawVoxelMesh_Instances_WithoutInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn, int lodLevel)
{
   // Collect transform data for every node of this mesh/material kind.
   radiant::perfmon::SwitchToCounter("draw mesh instances");
   // Set vertex layout
   gRDI->setVertexLayout( Modules::renderer()._vlVoxelModel );
   ShaderCombination* curShader = Modules::renderer().getCurShader();

   #define RENDER_LOG() R_LOG(9) << " drawing without instancing (" \
                                 << " handle:" << vmn->getParent()->getHandle() \
                                 << " name:" << vmn->getParent()->getName() \
                                 << ") "

   for (const auto& node : renderableQueue) {
		VoxelMeshNode *meshNode = (VoxelMeshNode *)node.node;
      const SceneNode *transNode = meshNode->getParentModel();

      RENDER_LOG() << "setting (mesh handle:" << meshNode->getHandle() << " mesh name:" << meshNode->getName() << ") world matrix to " 
                   << "(" << transNode->_absTrans.c[3][0] << ", " << transNode->_absTrans.c[3][1] << ", " << transNode->_absTrans.c[3][2] << ") matrix addr:" << (void*)&transNode->_absTrans.c;

      gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &transNode->_absTrans.x[0] );
      gRDI->drawIndexed(RDIPrimType::PRIM_TRILIST, vmn->getBatchStart(lodLevel), vmn->getBatchCount(lodLevel),
         vmn->getVertRStart(lodLevel), vmn->getVertREnd(lodLevel) - vmn->getVertRStart(lodLevel) + 1);

      Modules::stats().incStat( EngineStats::BatchCount, 1 );
   }
   Modules::stats().incStat( EngineStats::TriCount, (vmn->getBatchCount(lodLevel) / 3.0f) * renderableQueue.size() );

   #undef RENDER_LOG
}


void Renderer::drawInstanceNode(SceneId sceneId, std::string const& shaderContext, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
	if( frust1 == 0x0 || Modules::renderer().getCurCamera() == 0x0 ) return;
	if( debugView ) return;  // Don't render in debug view

   bool useInstancing = gRDI->getCaps().hasInstancing;

   Modules::config().setGlobalShaderFlag("DRAW_WITH_INSTANCING", useInstancing);
   Modules::config().setGlobalShaderFlag("DRAW_SKINNED", false);
	
   R_LOG(9) << "drawing instance nodes (useInstancing:" << useInstancing << " lod:" << lodLevel << ")";

   #define RENDER_LOG() R_LOG(9) << " instance " << in->_handle << ": " 

   if (useInstancing) {   

      gRDI->setVertexLayout( Modules::renderer()._vlInstanceVoxelModel );
	   MaterialResource *curMatRes = 0x0;
      for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::InstanceNode) )
	   {
		   InstanceNode* in = (InstanceNode *)entry.node;

         gRDI->setVertexBuffer( 0, in->_geoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );
         gRDI->setVertexBuffer( 1, in->_instanceBufObj, 0, 16 * sizeof(float) );
         gRDI->setIndexBuffer( in->_geoRes->getIndexBuf(), IDXFMT_32 );

         if( curMatRes != in->_matRes )
		   {
            if( !Modules::renderer().setMaterial( in->_matRes, shaderContext ) ) {
               RENDER_LOG() << "no material for context " << shaderContext << ".  ignoring.";
               continue;
            }
			   curMatRes = in->_matRes;
		   }

         RENDER_LOG() << "rendering...";
         gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, 
            in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoIndexCountI), 
            0, in->_usedInstances);
	   }
   } else {
      gRDI->setVertexLayout( Modules::renderer()._vlVoxelModel );
	   MaterialResource *curMatRes = 0x0;
      for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::InstanceNode) )
	   {
		   InstanceNode* in = (InstanceNode *)entry.node;

         gRDI->setVertexBuffer( 0, in->_geoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );
         gRDI->setIndexBuffer( in->_geoRes->getIndexBuf(), IDXFMT_32 );

         if( curMatRes != in->_matRes )
		   {
            if( !Modules::renderer().setMaterial( in->_matRes, shaderContext ) ) {
               RENDER_LOG() << "no material for context " << shaderContext << ".  ignoring.";
               continue;
            }
			   curMatRes = in->_matRes;
		   }
         ShaderCombination* curShader = Modules::renderer().getCurShader();

         RENDER_LOG() << "rendering " << in->_usedInstances << " instances...";
         for (int i = 0; i < in->_usedInstances; i++) {
            gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &in->_instanceBuf[i * 16]);
            gRDI->drawIndexed(RDIPrimType::PRIM_TRILIST, 0, 
               in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoIndexCountI), 
               0, in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoVertexCountI) - 1);
         }
	   }
   }
#undef RENDER_LOG

	gRDI->setVertexLayout( 0 );
}


void Renderer::drawParticles(SceneId sceneId, std::string const& shaderContext, bool debugView,
                              const Frustum *frust1, const Frustum * /*frust2*/, RenderingOrder::List /*order*/,
                              int occSet, int lodLevel )
{
	if( frust1 == 0x0 || Modules::renderer().getCurCamera() == 0x0 ) return;
	if( debugView ) return;  // Don't render particles in debug view

	MaterialResource *curMatRes = 0x0;

	// Bind particle geometry
	gRDI->setVertexBuffer( 0, Modules::renderer().getParticleVBO(), 0, sizeof( ParticleVert ) );
	gRDI->setIndexBuffer( Modules::renderer().getQuadIdxBuf(), IDXFMT_16 );
	ASSERT( QuadIndexBufCount >= ParticlesPerBatch * 6 );

	// Loop through emitter queue
	for( const auto& entry : Modules::sceneMan().sceneForId(sceneId).getRenderableQueue(SceneNodeTypes::Emitter) )
	{
		EmitterNode *emitter = (EmitterNode *)entry.node;
		
		if( emitter->_particleCount == 0 ) continue;
		
		// Occlusion culling
		uint32 queryObj = 0;
		if( occSet >= 0 )
		{
			if( occSet > (int)emitter->_occQueries.size() - 1 )
			{
				emitter->_occQueries.resize( occSet + 1, 0 );
				emitter->_lastVisited.resize( occSet + 1, 0 );
			}
			if( emitter->_occQueries[occSet] == 0 )
			{
				queryObj = gRDI->createOcclusionQuery();
				emitter->_occQueries[occSet] = queryObj;
				emitter->_lastVisited[occSet] = 0;
			}
			else
			{
				if( emitter->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					emitter->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					// Check query result (viewer must be outside of bounding box)
					if( nearestDistToAABB( frust1->getOrigin(), emitter->getBBox().min(),
					                       emitter->getBBox().max() ) > 0 &&
						gRDI->getQueryResult( emitter->_occQueries[occSet] ) < 1 )
					{
						Modules::renderer().pushOccProxy( 0, emitter->getBBox().min(),
							emitter->getBBox().max(), emitter->_occQueries[occSet] );
						continue;
					}
					else
						queryObj = emitter->_occQueries[occSet];
				}
			}
		}
		
		// Set material
		if( curMatRes != emitter->_materialRes )
		{
			if( !Modules::renderer().setMaterial( emitter->_materialRes, shaderContext ) ) continue;
			curMatRes = emitter->_materialRes;
		}

		// Set vertex layout
		gRDI->setVertexLayout( Modules::renderer()._vlParticle );
		
		if( queryObj )
			gRDI->beginQuery( queryObj );
		
		// Shader uniforms
		ShaderCombination *curShader = Modules::renderer().getCurShader();
		if( curShader->uni_nodeId >= 0 )
		{
			float id = (float)emitter->getHandle();
			gRDI->setShaderConst( curShader->uni_nodeId, CONST_FLOAT, &id );
		}

		// Divide particles in batches and render them
		for( uint32 j = 0; j < emitter->_particleCount / ParticlesPerBatch; ++j )
		{
			// Check if batch needs to be rendered
			bool allDead = true;
			for( uint32 k = 0; k < ParticlesPerBatch; ++k )
			{
				if( emitter->_particles[j*ParticlesPerBatch + k].life > 0 )
				{
					allDead = false;
					break;
				}
			}
			if( allDead ) continue;

			// Render batch
			if( curShader->uni_parPosArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parPosArray, CONST_FLOAT3,
				                      (float *)emitter->_parPositions + j*ParticlesPerBatch*3, ParticlesPerBatch );
			if( curShader->uni_parSizeAndRotArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parSizeAndRotArray, CONST_FLOAT2,
				                      (float *)emitter->_parSizesANDRotations + j*ParticlesPerBatch*2, ParticlesPerBatch );
			if( curShader->uni_parColorArray >= 0 )
				gRDI->setShaderConst( curShader->uni_parColorArray, CONST_FLOAT4,
				                      (float *)emitter->_parColors + j*ParticlesPerBatch*4, ParticlesPerBatch );

			gRDI->drawIndexed( PRIM_TRILIST, 0, ParticlesPerBatch * 6, 0, ParticlesPerBatch * 4 );
			Modules::stats().incStat( EngineStats::BatchCount, 1 );
			Modules::stats().incStat( EngineStats::TriCount, ParticlesPerBatch * 2.0f );
		}

		uint32 count = emitter->_particleCount % ParticlesPerBatch;
		if( count > 0 )
		{
			uint32 offset = (emitter->_particleCount / ParticlesPerBatch) * ParticlesPerBatch;
			
			// Check if batch needs to be rendered
			bool allDead = true;
			for( uint32 k = 0; k < count; ++k )
			{
				if( emitter->_particles[offset + k].life > 0 )
				{
					allDead = false;
					break;
				}
			}
			
			if( !allDead )
			{
				// Render batch
				if( curShader->uni_parPosArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parPosArray, CONST_FLOAT3,
					                      (float *)emitter->_parPositions + offset*3, count );
				if( curShader->uni_parSizeAndRotArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parSizeAndRotArray, CONST_FLOAT2,
					                      (float *)emitter->_parSizesANDRotations + offset*2, count );
				if( curShader->uni_parColorArray >= 0 )
					gRDI->setShaderConst( curShader->uni_parColorArray, CONST_FLOAT4,
					                      (float *)emitter->_parColors + offset*4, count );
				
				gRDI->drawIndexed( PRIM_TRILIST, 0, count * 6, 0, count * 4 );
				Modules::stats().incStat( EngineStats::BatchCount, 1 );
				Modules::stats().incStat( EngineStats::TriCount, count * 2.0f );
			}
		}

		if( queryObj )
			gRDI->endQuery( queryObj );
	}

	// Draw occlusion proxies
	if( occSet >= 0 )
		Modules::renderer().drawOccProxies( 0 );
	
	gRDI->setVertexLayout( 0 );
}


// =================================================================================================
// Main Rendering Functions
// =================================================================================================

void Renderer::render( CameraNode *camNode, PipelineResource* pRes )
{
   radiant::perfmon::SwitchToCounter("render scene");
	_curCamera = camNode;
   _curPipeline = pRes;
	if( _curCamera == 0x0 ) return;

   // Make sure all the nodes have been transformed appropriately (in particular, the camera may have been
   // moved, and since we touch it directly for rendering, lets make sure its transforms are updated before
   // using it.
   Modules::sceneMan().sceneForNode(camNode->getHandle()).updateNodes();

   const SceneId sceneId = Modules::sceneMan().sceneIdFor(camNode->getHandle());
   gRDI->_frameDebugInfo.setViewerFrustum_(camNode->getFrustum());
   gRDI->_frameDebugInfo.addShadowCascadeFrustum_(camNode->getFrustum());
   

	// Build sampler anisotropy mask from anisotropy value
	int maxAniso = Modules::config().maxAnisotropy;
	if( maxAniso <= 1 ) _maxAnisoMask = SS_ANISO1;
	else if( maxAniso <= 2 ) _maxAnisoMask = SS_ANISO2;
	else if( maxAniso <= 4 ) _maxAnisoMask = SS_ANISO4;
	else if( maxAniso <= 8 ) _maxAnisoMask = SS_ANISO8;
	else _maxAnisoMask = SS_ANISO16;

	gRDI->setViewport( _curCamera->_vpX, _curCamera->_vpY, _curCamera->_vpWidth, _curCamera->_vpHeight );
	if( Modules::config().debugViewMode || _curPipeline == 0x0 )
	{
		renderDebugView();
		finishRendering();
		return;
	}

   bool drawShadows = Modules::config().getOption(EngineOptions::EnableShadows) == 1.0f;
	
	// Initialize
	gRDI->_outputBufferIndex = _curCamera->_outputBufferIndex;
	if( _curCamera->_outputTex != 0x0 )
		gRDI->setRenderBuffer( _curCamera->_outputTex->getRBObject() );
	else 
		gRDI->setRenderBuffer( 0 );

	// Process pipeline commands
	for( uint32 i = 0; i < _curPipeline->_stages.size(); ++i )
	{
		PipelineStagePtr &stage = _curPipeline->_stages[i];
		if( !stage->enabled ) continue;
		_curStageMatLink = stage->matLink;

      radiant::perfmon::SwitchToCounter (stage->debug_name.c_str());
		R_LOG(5) << "running pipeline stage " << stage->id;
		for( uint32 j = 0; j < stage->commands.size(); ++j )
		{
			PipelineCommand &pc = stage->commands[j];
			RenderTarget *rt;

			switch( pc.command )
			{
			case PipelineCommands::SwitchTarget:
				// Unbind all textures
				bindPipeBuffer( 0x0, "", 0 );
				
				// Bind new render target
				rt = (RenderTarget *)pc.params[0].getPtr();
				_curRenderTarget = rt;

				if( rt != 0x0 )
				{
               RDIRenderBuffer &rendBuf = gRDI->_rendBufs.getRef( rt->rendBuf );
					gRDI->_outputBufferIndex = _curCamera->_outputBufferIndex;
					gRDI->setViewport( 0, 0, rendBuf.width, rendBuf.height );
               gRDI->setRenderBuffer( rt->rendBuf );
				}
				else
				{
					gRDI->setViewport( _curCamera->_vpX, _curCamera->_vpY, _curCamera->_vpWidth, _curCamera->_vpHeight );
					gRDI->setRenderBuffer( _curCamera->_outputTex != 0x0 ?
					                       _curCamera->_outputTex->getRBObject() : 0 );
				}
				break;

         case PipelineCommands::BuildMipmap: 
            {
               rt = (RenderTarget *)pc.params[0].getPtr();
               int index = pc.params[1].getInt();

               RDIRenderBuffer &rendBuf = gRDI->_rendBufs.getRef( rt->rendBuf );
               uint32 texGlObj = gRDI->_textures.getRef(rendBuf.colTexs[index]).glObj;
               glActiveTexture(GL_TEXTURE15);
               glEnable(GL_TEXTURE_2D);
               glBindTexture(GL_TEXTURE_2D, texGlObj);
               glGenerateMipmapEXT(GL_TEXTURE_2D);
               glBindTexture(GL_TEXTURE_2D, 0);
               glDisable(GL_TEXTURE_2D);
            }
            break;

			case PipelineCommands::BindBuffer:
            {
               uint32 rendBuf = 0;
   				RenderTarget* rt = (RenderTarget *)pc.params[0].getPtr();
               if (rt) {
                  rendBuf = rt->rendBuf;
               } else {
                  rendBuf = (uint32)pc.params[3].getInt();
               }
               // unbind the shadow texture, if any.
               setupShadowMap(nullptr, true);
				   bindPipeBuffer(rendBuf, pc.params[1].getString(), (uint32)pc.params[2].getInt());
            }
				break;

			case PipelineCommands::UnbindBuffers:
				bindPipeBuffer( 0x0, "", 0 );
				break;

			case PipelineCommands::ClearTarget:
				clear( pc.params[0].getBool(), pc.params[1].getBool(), pc.params[2].getBool(),
				       pc.params[3].getBool(), pc.params[4].getBool(), pc.params[5].getFloat(),
				       pc.params[6].getFloat(), pc.params[7].getFloat(), pc.params[8].getFloat(), pc.params[9].getInt() );
				break;

			case PipelineCommands::DrawGeometry:
				drawGeometry(sceneId, pc.params[0].getString(), (RenderingOrder::List)pc.params[1].getInt(),
                          pc.params[2].getInt(), _curCamera->_occSet, pc.params[3].getFloat(), pc.params[4].getFloat(), pc.params[5].getInt() );
				break;

			case PipelineCommands::DrawSelected:
				drawSelected(sceneId, pc.params[0].getString(), (RenderingOrder::List)pc.params[1].getInt(),
                          pc.params[2].getInt(), _curCamera->_occSet, pc.params[3].getFloat(), pc.params[4].getFloat(), pc.params[5].getInt() );
				break;

         case PipelineCommands::DrawProjections:
            drawProjections(sceneId,pc.params[0].getString(), pc.params[1].getInt());
            break;

			case PipelineCommands::DrawOverlays:
            drawOverlays( pc.params[0].getString() );
				break;

			case PipelineCommands::DrawQuad:
				drawFSQuad( pc.params[0].getResource(), pc.params[1].getString() );
			break;

			case PipelineCommands::DoForwardLightLoop:
				doForwardLightPass(sceneId, pc.params[0].getString(), 
               pc.params[1].getBool() || !drawShadows, (RenderingOrder::List)pc.params[2].getInt(),
                               _curCamera->_occSet, pc.params[3].getBool(), pc.params[4].getInt() );
				break;

			case PipelineCommands::DoDeferredLightLoop:
				doDeferredLightPass(sceneId, pc.params[0].getBool() || !drawShadows, 
               (MaterialResource*)pc.params[1].getResource());
				break;

			case PipelineCommands::SetUniform:
				if( pc.params[0].getResource() && pc.params[0].getResource()->getType() == ResourceTypes::Material )
				{
					((MaterialResource *)pc.params[0].getResource())->setUniform( pc.params[1].getString(),
						pc.params[2].getFloat(), pc.params[3].getFloat(),
						pc.params[4].getFloat(), pc.params[5].getFloat() );
				}
				break;
			}
		}
	}
	
	finishRendering();
}

void Renderer::finalizeFrame()
{
	++_frameID;
	// Reset frame timer
	radiant::perfmon::Timer *timer = Modules::stats().getTimer( EngineStats::FrameTime );
	ASSERT( timer != 0x0 );

   _instanceDataCache.clear();
   gRDI->clearBufferCache();
   Modules::stats().getStat( EngineStats::FrameTime, true );  // Reset
   Modules::stats().incStat( EngineStats::FrameTime, (float)radiant::perfmon::CounterToMilliseconds(timer->GetElapsed()));
   logPerformanceData();
	timer->Restart();
   Modules::sceneMan().clearQueryCaches();
   gRDI->_frameDebugInfo.endFrame();
}

void Renderer::logPerformanceData()
{
   float curFrameTime = Modules::stats().getStat(EngineStats::FrameTime, false);
	float curFPS = 1000.0f / curFrameTime;
	
   float triCount = Modules::stats().getStat(EngineStats::TriCount, false);
   float numBatches = Modules::stats().getStat(EngineStats::BatchCount, false);

   Modules::log().writePerfInfo("%d %d %d", (int)curFPS, (int)triCount, (int)numBatches);
}


void Renderer::collectOneDebugFrame()
{
   gRDI->_frameDebugInfo.sampleFrame();
}

void Renderer::renderDebugView()
{
	float color[4] = { 0 };
   float frustCol[16] = {
      1,0,0,1,
      0,1,0,1,
      0,0,1,1,
      1,1,0,1
   };
	
	gRDI->setRenderBuffer( 0 );
	setMaterial( 0x0, "" );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	gRDI->clear( CLR_DEPTH | CLR_COLOR );


	// Draw renderable nodes as wireframe
	setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );

	// Draw bounding boxes
	glDisable( GL_CULL_FACE );
	setMaterial( 0x0, "" );
	setShaderComb( &_defColorShader );
	commitGeneralUniforms();
	gRDI->setShaderConst( _defColorShader.uni_worldMat, CONST_FLOAT44, &Matrix4f().x[0] );
	color[0] = 0.4f; color[1] = 0.4f; color[2] = 0.4f; color[3] = 1;
   int frustNum = 0;

   Scene& defaultScene = Modules::sceneMan().sceneForId(0);

   defaultScene.updateQueues( "rendering debug view", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
      SceneNodeFlags::NoDraw | SceneNodeFlags::NoCull, 0, true, true, true );
	gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color[0] );
	drawRenderables(0, "", true, &_curCamera->getFrustum(), 0x0, RenderingOrder::None, -1, 0 );
   for (const auto& queue : defaultScene.getRenderableQueues())
   {
      for( const auto& entry : queue.second )
	   {
		   const SceneNode *sn = entry.node;
		   //drawAABB( sn->_bBox.min(), sn->_bBox.max() );
	   }
   }

   float tempscale = 1.0f;
	setShaderComb( &_defColorShader );
	commitGeneralUniforms();
   gRDI->setShaderConst(_defColorShader.uni_modelScale, CONST_FLOAT, &tempscale);
	defaultScene.updateQueues( "rendering debug view", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
	                                  SceneNodeFlags::NoDraw, 0, true, true );
   for (const auto& frust : gRDI->_frameDebugInfo.getShadowCascadeFrustums())
   {
	   gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &frustCol[frustNum * 4] );
      drawFrustum(frust);
      frustNum = (frustNum + 1) % 4;
   }

   frustNum = 0;
   for (const auto& frust : gRDI->_frameDebugInfo.getSplitFrustums())
   {
	   gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &frustCol[frustNum * 4] );
      drawFrustum(frust);
      frustNum = (frustNum + 1) % 4;
   }

   for (const auto& poly: gRDI->_frameDebugInfo.getPolygons())
   {
	   color[0] = 1.0f; color[1] = 0.0f; color[2] = 1.0f; color[3] = 1;
	   gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, color );
      drawPoly(poly.points());
   }

   glEnable( GL_CULL_FACE );
	/*// Draw skeleton
	setShaderConst( _defColorShader.uni_worldMat, CONST_FLOAT44, &Matrix4f().x[0] );
	color[0] = 1; color[1] = 0; color[2] = 0; color[3] = 1;
	setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT, color );
	glLineWidth( 2.0f );
	glPointSize( 5.0f );
	for( uint32 i = 0, si = (uint32)Modules::sceneMan().getRenderableQueue().size(); i < si; ++i )
	{
		SceneNode *sn = Modules::sceneMan().getRenderableQueue()[i].node;
		
		if( sn->getType() == SceneNodeTypes::Model )
		{
			ModelNode *model = (ModelNode *)sn;

			for( uint32 j = 0, sj = (uint32)model->_jointList.size(); j < sj; ++j )
			{
				if( model->_jointList[j]->_parent->getType() != SceneNodeTypes::Model )
				{
					Vec3f pos1 = model->_jointList[j]->_absTrans * Vec3f( 0, 0, 0 );
					Vec3f pos2 = model->_jointList[j]->_parent->_absTrans * Vec3f( 0, 0, 0 );

					glBegin( GL_LINES );
					glVertex3fv( (float *)&pos1.x );
					glVertex3fv( (float *)&pos2.x );
					glEnd();

					glBegin( GL_POINTS );
					glVertex3fv( (float *)&pos1.x );
					glEnd();
				}
			}
		}
	}
	glLineWidth( 1.0f );
	glPointSize( 1.0f );*/

	// Draw light volumes
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	glCullFace( GL_FRONT );
	color[0] = 1; color[1] = 1; color[2] = 0; color[3] = 0.25f;
	gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, color );
	for( size_t i = 0, s = defaultScene.getLightQueue().size(); i < s; ++i )
	{
		LightNode *lightNode = (LightNode *)defaultScene.getLightQueue()[i];
		
		if( lightNode->_fov < 180 )
		{
			float r = lightNode->_radius2 * tanf( degToRad( lightNode->_fov / 2 ) );
			drawCone( lightNode->_radius2, r, lightNode->_absTrans );
		}
      else if (!lightNode->getParamI(LightNodeParams::DirectionalI))
		{
			drawSphere( lightNode->_absPos, lightNode->_radius2 );
		}
	}
	glCullFace( GL_BACK );
	glDisable( GL_BLEND );
}


void Renderer::finishRendering()
{
	gRDI->setRenderBuffer( 0 );
	setMaterial( 0x0, "" );
	gRDI->resetStates();
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
}

}  // namespace
