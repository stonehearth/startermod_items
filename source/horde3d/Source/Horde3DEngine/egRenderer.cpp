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
#if defined(OPTIMIZE_GSLS)
#include <glsl/glsl_optimizer.h>
#endif
#include "utDebug.h"

#define R_LOG(level)    LOG(horde.renderer, level)

namespace Horde3D {

using namespace std;


const char *vsDefColor =
	"uniform mat4 viewProjMat;\n"
	"uniform mat4 worldMat;\n"
	"attribute vec3 vertPos;\n"
	"void main() {\n"
	"	gl_Position = viewProjMat * worldMat * vec4( vertPos, 1.0 );\n"
	"}\n";

const char *fsDefColor =
	"uniform vec4 color;\n"
	"void main() {\n"
	"	gl_FragColor = color;\n"
	"}\n";

uint32 Renderer::_vbInstanceVoxelData;
float* Renderer::_vbInstanceVoxelBuf;

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
	_curLight = 0x0;
	_curShader = 0x0;
	_curRenderTarget = 0x0;
	_curShaderUpdateStamp = 1;
	_maxAnisoMask = 0;
	_smSize = 0;
   _materialOverride = 0x0;
   _curPipeline = 0x0;

	_shadowRB = 0;
	_vlPosOnly = 0;
	_vlOverlay = 0;
	_vlModel = 0;
   _vlVoxelModel = 0;
   _vlInstanceVoxelModel = 0;
	_vlParticle = 0;
   bool openglES = false;
   _lod_polygon_offset_x = 0.0f;
   _lod_polygon_offset_y = 0.0f;
#if defined(OPTIMIZE_GSLS)
   _glsl_opt_ctx = glslopt_initialize(openglES);
#endif
}


Renderer::~Renderer()
{
	releaseShadowRB();
	gRDI->destroyTexture( _defShadowMap );
	gRDI->destroyBuffer( _particleVBO );
   gRDI->destroyBuffer(_vbInstanceVoxelData);
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

   // SSAO: arbitrarily gate SSAO with gl version >= 4.0
   gpuCompatibility_.canDoSsao = glVer >= 40;
}

void Renderer::getEngineCapabilities(EngineRendererCaps* rendererCaps, EngineGpuCaps* gpuCaps) const
{
   if (rendererCaps) {
      rendererCaps->ShadowsSupported = gpuCompatibility_.canDoShadows;
      rendererCaps->SsaoSupported = gpuCompatibility_.canDoSsao;
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

	VertexLayoutAttrib attribsVoxelModel[3] = {
      { "vertPos",     0, 3, 0 }, 
      { "normal",      0, 3, 12 },
      { "color",       0, 4, 24 },
	};
	_vlVoxelModel = gRDI->registerVertexLayout( 3, attribsVoxelModel );

	VertexLayoutAttrib attribsInstanceVoxelModel[4] = {
      { "vertPos",     0, 3, 0 }, 
      { "normal",      0, 3, 12 },
      { "color",       0, 4, 24 },
      { "transform",   1, 16, 0 }
	};

   VertexDivisorAttrib divisorsInstanceVoxelModel[4] = {
      0, 0, 0, 1
   };
	_vlInstanceVoxelModel = gRDI->registerVertexLayout( 4, attribsInstanceVoxelModel, divisorsInstanceVoxelModel );

   _vbInstanceVoxelData = gRDI->createVertexBuffer(MaxVoxelInstanceCount * (4 * 4) * sizeof(float), DYNAMIC, nullptr);
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
	
	// Create shadow map render target
   if (Modules::config().getOption(EngineOptions::EnableShadows)) {
      if(!createShadowRB( Modules::config().shadowMapSize, Modules::config().shadowMapSize ) )
	   {
		   Modules::log().writeError( "Failed to create shadow map" );
		   return false;
	   }
   }
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
	Timer *timer = Modules::stats().getTimer( EngineStats::FrameTime );
	ASSERT( timer != 0x0 );
	timer->setEnabled( true );
	
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

   Matrix4f mat = Matrix4f();
   mat.toIdentity();
   gRDI->setShaderConst( _curShader->uni_worldMat, CONST_FLOAT44, &mat.x[0] );
   gRDI->setVertexBuffer( 0, _vbPoly, 0, 12 );
   gRDI->setIndexBuffer( _ibPoly, IDXFMT_16 );
   gRDI->setVertexLayout( _vlPosOnly );
   gRDI->drawIndexed( PRIM_TRILIST, 0, 3 + ((poly.size() - 3) * 3), 0, poly.size() );
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
	sc.uni_lightDir = gRDI->getShaderConstLoc( shdObj, "lightDir" );
	sc.uni_lightColor = gRDI->getShaderConstLoc( shdObj, "lightColor" );
	sc.uni_lightAmbientColor = gRDI->getShaderConstLoc( shdObj, "lightAmbientColor" );
	sc.uni_shadowSplitDists = gRDI->getShaderConstLoc( shdObj, "shadowSplitDists" );
	sc.uni_shadowMats = gRDI->getShaderConstLoc( shdObj, "shadowMats" );
	sc.uni_shadowMapSize = gRDI->getShaderConstLoc( shdObj, "shadowMapSize" );
	sc.uni_shadowBias = gRDI->getShaderConstLoc( shdObj, "shadowBias" );
	
	// Particle-specific uniforms
	sc.uni_parPosArray = gRDI->getShaderConstLoc( shdObj, "parPosArray" );
	sc.uni_parSizeAndRotArray = gRDI->getShaderConstLoc( shdObj, "parSizeAndRotArray" );
	sc.uni_parColorArray = gRDI->getShaderConstLoc( shdObj, "parColorArray" );

   sc.uni_cubeBatchTransformArray = gRDI->getShaderConstLoc( shdObj, "cubeBatchTransformArray" );
   sc.uni_cubeBatchColorArray = gRDI->getShaderConstLoc( shdObj, "cubeBatchColorArray" );
	
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
	if( _curShader != sc )
	{
		if( sc == 0x0 ) gRDI->bindShader( 0 );
		else gRDI->bindShader( sc->shaderObj );

		_curShader = sc;
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

   for (auto& e : _uniformMats)
   {
      int loc = gRDI->getShaderConstLoc(_curShader->shaderObj, e.first.c_str());
      if (loc >= 0) {
         Matrix4f& m = e.second;
         gRDI->setShaderConst(loc, CONST_FLOAT44, m.x);
      }
   }
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
			float dimensions[2] = { (float)gRDI->_fbWidth, (float)gRDI->_fbHeight };
			gRDI->setShaderConst( _curShader->uni_frameBufSize, CONST_FLOAT2, dimensions );
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
         Matrix4f m = getCurCamera()->getViewMat();
			gRDI->setShaderConst( _curShader->uni_camViewMatInv, CONST_FLOAT44, m.x );
      }
		
		if( _curShader->uni_camViewerPos >= 0 ) 
      {
         Matrix4f m = getCurCamera()->getViewMat();
			gRDI->setShaderConst( _curShader->uni_camViewerPos, CONST_FLOAT3, &m.x[12] );
      }

      if( _curShader->uni_projectorMat >= 0 )
      {
         Matrix4f m = _projectorMat.inverted();
         gRDI->setShaderConst( _curShader->uni_projectorMat, CONST_FLOAT44, m.x);
      }
		// Light params
		if( _curLight != 0x0 )
		{
			if( _curShader->uni_lightPos >= 0 )
			{
				float data[4] = { _curLight->_absPos.x, _curLight->_absPos.y,
				                  _curLight->_absPos.z, _curLight->_radius };
				gRDI->setShaderConst( _curShader->uni_lightPos, CONST_FLOAT4, data );
			}
			
			if( _curShader->uni_lightDir >= 0 )
			{
				float data[4] = { _curLight->_spotDir.x, _curLight->_spotDir.y,
				                  _curLight->_spotDir.z, cosf( degToRad( _curLight->_fov / 2.0f ) ) };
				gRDI->setShaderConst( _curShader->uni_lightDir, CONST_FLOAT4, data );
			}
			
			if( _curShader->uni_lightColor >= 0 )
			{
				Vec3f col = _curLight->_diffuseCol * _curLight->_diffuseColMult;
				gRDI->setShaderConst( _curShader->uni_lightColor, CONST_FLOAT3, &col.x );
			}
			
			if( _curShader->uni_lightAmbientColor >= 0 )
			{
				Vec3f col = _curLight->_ambientCol;
				gRDI->setShaderConst( _curShader->uni_lightAmbientColor, CONST_FLOAT3, &col.x );
			}

         if( _curShader->uni_shadowSplitDists >= 0 )
				gRDI->setShaderConst( _curShader->uni_shadowSplitDists, CONST_FLOAT4, &_splitPlanes[1] );

			if( _curShader->uni_shadowMats >= 0 )
				gRDI->setShaderConst( _curShader->uni_shadowMats, CONST_FLOAT44, &_lightMats[0].x[0], 4 );
			
			if( _curShader->uni_shadowMapSize >= 0 )
				gRDI->setShaderConst( _curShader->uni_shadowMapSize, CONST_FLOAT, &_smSize );
			
			if( _curShader->uni_shadowBias >= 0 )
				gRDI->setShaderConst( _curShader->uni_shadowBias, CONST_FLOAT, &_curLight->_shadowMapBias );
		}

		_curShader->lastUpdateStamp = _curShaderUpdateStamp;
	}
}

ShaderCombination* Renderer::findShaderCombination(ShaderResource* r, ShaderContext* context) const
{
   // Look for a shader with the right combination of engine flags.
   ShaderCombination* result = 0x0;
   for (auto& sc : context->shaderCombinations)
   {
      result = &sc;
      for (const auto& flag : Modules().config().shaderFlags)
      {
         if (sc.engineFlags.find(flag) == sc.engineFlags.end()) {
            result = 0x0;
            break;
         }
      }

      if (result != 0x0) {
         if (sc.engineFlags.size() == Modules().config().shaderFlags.size()) {
            break;
         }
         result = 0x0;
      }
   }

   // If this is a new combination of engine flags, compile a new combination.
   if (result == 0x0)
   {
      context->shaderCombinations.push_back(ShaderCombination());
      ShaderCombination& sc = context->shaderCombinations.back();
      for (const auto& flag : Modules().config().shaderFlags) {
         sc.engineFlags.insert(flag);
      }

      r->compileCombination(*context, sc);
      result = &context->shaderCombinations.back();
   }

   return result;
}

bool Renderer::isShaderContextSwitch(std::string const& newContext, const MaterialResource *materialRes)
{
   ShaderResource *sr = materialRes->_shaderRes;
   if (sr == 0x0) {
      return false;
   }

   ShaderContext *sc = sr->findContext(newContext);
   if (sc == 0x0) {
      return false;
   }

   ShaderCombination *scc = findShaderCombination(sr, sc);
	return scc != _curShader;
}


bool Renderer::setMaterialRec( MaterialResource *materialRes, std::string const& shaderContext,
                               ShaderResource *shaderRes )
{
   radiant::perfmon::TimelineCounterGuard smr("setMaterialRec");
	if( materialRes == 0x0 ) return false;
	
	bool firstRec = (shaderRes == 0x0);
	bool result = true;
	
	// Set shader in first recursion step
	if( firstRec )
	{	
		shaderRes = materialRes->_shaderRes;
		if( shaderRes == 0x0 ) return false;	
	
		// Find context
		ShaderContext *context = shaderRes->findContext( shaderContext );
		if( context == 0x0 ) return false;
		
		// Set shader combination
      ShaderCombination *sc = findShaderCombination(shaderRes, context);
		if( sc != _curShader ) setShaderComb( sc );
		if( _curShader == 0x0 || gRDI->_curShaderId == 0 ) return false;

		// Setup standard shader uniforms
		commitGeneralUniforms();

      // Configure color write mask.
      glColorMask(context->writeMask & 1 ? GL_TRUE : GL_FALSE, 
         context->writeMask & 2 ? GL_TRUE : GL_FALSE, 
         context->writeMask & 4 ? GL_TRUE : GL_FALSE, 
         context->writeMask & 8 ? GL_TRUE : GL_FALSE);

		// Configure depth mask
		if( context->writeDepth ) glDepthMask( GL_TRUE );
		else glDepthMask( GL_FALSE );

		// Configure cull mode
		if( !Modules::config().wireframeMode )
		{
			switch( context->cullMode )
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

      switch( context->stencilOpModes) 
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
      }

      switch( context->stencilFunc )
		{
		case TestModes::LessEqual:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_LEQUAL, context->stencilRef, 0xffffffff );
			break;
		case TestModes::Equal:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_EQUAL, context->stencilRef, 0xffffffff );
			break;
		case TestModes::Always:
         if (context->stencilOpModes == StencilOpModes::Off) {
            glDisable(GL_STENCIL_TEST);
         } else {
            glEnable(GL_STENCIL_TEST);
         }
         glStencilFunc( GL_ALWAYS, context->stencilRef, 0xffffffff );
			break;
		case TestModes::Less:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_LESS, context->stencilRef, 0xffffffff );
			break;
		case TestModes::Greater:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_GREATER, context->stencilRef, 0xffffffff );
			break;
		case TestModes::GreaterEqual:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_GEQUAL, context->stencilRef, 0xffffffff );
			break;
      case TestModes::NotEqual:
         glEnable(GL_STENCIL_TEST);
         glStencilFunc( GL_NOTEQUAL, context->stencilRef, 0xffffffff );
			break;
		}
		
		// Configure blending
		switch( context->blendMode )
		{
		case BlendModes::Replace:
			glDisable( GL_BLEND );
			break;
		case BlendModes::Blend:
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			break;
		case BlendModes::Add:
			glEnable( GL_BLEND );
			glBlendFunc( GL_ONE, GL_ONE );
			break;
		case BlendModes::AddBlended:
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			break;
      case BlendModes::Whateva:
         glEnable( GL_BLEND );
         glBlendFunc( GL_DST_ALPHA, GL_ONE );
         break;
		case BlendModes::Mult:
			glEnable( GL_BLEND );
			glBlendFunc( GL_DST_COLOR, GL_ZERO );
			break;
		}

		// Configure depth test
		if( context->depthTest )
		{
			glEnable( GL_DEPTH_TEST );
			
			switch( context->depthFunc )
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
		if( context->alphaToCoverage && Modules::config().sampleCount > 0 )
			glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
		else
			glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
	}

	// Setup texture samplers
	for( size_t i = 0, si = shaderRes->_samplers.size(); i < si; ++i )
	{
		if( _curShader->customSamplers[i] < 0 ) continue;
		
		ShaderSampler &sampler = shaderRes->_samplers[i];
		TextureResource *texRes = 0x0;

		// Use default texture
		if( firstRec) texRes = sampler.defTex;
		
		// Find sampler in material
		for( size_t j = 0, sj = materialRes->_samplers.size(); j < sj; ++j )
		{
			if( materialRes->_samplers[j].name == sampler.id )
			{
				if( materialRes->_samplers[j].texRes->isLoaded() )
					texRes = materialRes->_samplers[j].texRes;
				break;
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
		if( firstRec )
		{
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
	}

	// Set custom uniforms
	for( size_t i = 0, si = shaderRes->_uniforms.size(); i < si; ++i )
	{
		if( _curShader->customUniforms[i] < 0 ) continue;
		
		float *unifData = 0x0;

		// Find uniform in material
		for( size_t j = 0, sj = materialRes->_uniforms.size(); j < sj; ++j )
		{
			MatUniform &matUniform = materialRes->_uniforms[j];
			
			if( matUniform.name == shaderRes->_uniforms[i].id )
			{
            if (shaderRes->_uniforms[i].arraySize > 1) {
               unifData = matUniform.arrayValues.data();
            } else {
   				unifData = matUniform.values;
            }
				break;
			}
		}

		// Use default values if not found
		if( unifData == 0x0 && firstRec )
			unifData = shaderRes->_uniforms[i].defValues;

		if( unifData )
		{
			switch( shaderRes->_uniforms[i].size )
			{
			case 1:
				gRDI->setShaderConst( _curShader->customUniforms[i], CONST_FLOAT, unifData, shaderRes->_uniforms[i].arraySize );
				break;
			case 4:
            gRDI->setShaderConst( _curShader->customUniforms[i], CONST_FLOAT4, unifData, shaderRes->_uniforms[i].arraySize );
				break;
			}
		}
	}

	if( firstRec )
	{
		// Handle link of stage
		if( _curStageMatLink != 0x0 && _curStageMatLink != materialRes )
			result &= setMaterialRec( _curStageMatLink, shaderContext, shaderRes );
	}

	// Handle link of material resource
	if( materialRes->_matLink != 0x0 )
		result &= setMaterialRec( materialRes->_matLink, shaderContext, shaderRes );

	return result;
}


bool Renderer::setMaterial( MaterialResource *materialRes, std::string const& shaderContext )
{
	if( materialRes == 0x0 )
	{	
		setShaderComb( 0x0 );
		glDisable( GL_BLEND );
		glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_TRUE );
		return false;
	}

   // First, see if the model's material has what we're looking for.
	if (setMaterialRec(materialRes, shaderContext, 0x0)) {
      return true;
   }

   // Next, try everything in the model material's chain.
   MaterialResource* mr = materialRes->_parentMaterial;
   while (mr != 0x0) {
      if (setMaterialRec(mr, shaderContext, 0x0)) {
         return true;
      }
      mr = mr->_parentMaterial;
   }

	_curShader = 0x0;
	return false;
}


// =================================================================================================
// Shadowing
// =================================================================================================

bool Renderer::createShadowRB( uint32 width, uint32 height )
{
	return (_shadowRB = gRDI->createRenderBuffer( width, height, TextureFormats::BGRA8, true, 0, 0 )) != 0;
}


void Renderer::releaseShadowRB()
{
	if( _shadowRB ) gRDI->destroyRenderBuffer( _shadowRB );
}


void Renderer::setupShadowMap( bool noShadows )
{
	uint32 sampState = SS_FILTER_BILINEAR | SS_ANISO1 | SS_ADDR_CLAMPCOL | SS_COMP_LEQUAL;
	
	// Bind shadow map
	if( !noShadows && _curLight->_shadowMapCount > 0 )
	{
      gRDI->setTexture( 12, gRDI->getRenderBufferTex( _shadowRB, 32 ), sampState );
		_smSize = (float)Modules::config().shadowMapSize;
	}
	else
	{
		gRDI->setTexture( 12, _defShadowMap, sampState );
		_smSize = 4;
	}
}


Matrix4f Renderer::calcCropMatrix( const Frustum &frustSlice, const Vec3f lightPos, const Matrix4f &lightViewProjMat )
{
	float frustMinX =  Math::MaxFloat, bbMinX =  Math::MaxFloat;
	float frustMinY =  Math::MaxFloat, bbMinY =  Math::MaxFloat;
	float frustMinZ =  Math::MaxFloat, bbMinZ =  Math::MaxFloat;
	float frustMaxX = -Math::MaxFloat, bbMaxX = -Math::MaxFloat;
	float frustMaxY = -Math::MaxFloat, bbMaxY = -Math::MaxFloat;
	float frustMaxZ = -Math::MaxFloat, bbMaxZ = -Math::MaxFloat;
	
	// Find post-projective space AABB of all objects in frustum
	Modules::sceneMan().updateQueues("calculating crop matrix", frustSlice, 0x0, RenderingOrder::None,
		SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, true );
	
   for (const auto& queue : Modules::sceneMan().getRenderableQueues())
   {
      for( const auto& entry : queue.second )
	   {
		   const BoundingBox &aabb = entry.node->getBBox();
		
		   // Check if light is inside AABB
		   if( lightPos.x >= aabb.min().x && lightPos.y >= aabb.min().y && lightPos.z >= aabb.min().z &&
			   lightPos.x <= aabb.max().x && lightPos.y <= aabb.max().y && lightPos.z <= aabb.max().z )
		   {
			   bbMinX = bbMinY = bbMinZ = -1;
			   bbMaxX = bbMaxY = bbMaxZ = 1;
			   break;
		   }
		
		   for( uint32 j = 0; j < 8; ++j )
		   {
			   Vec4f v1 = lightViewProjMat * Vec4f( aabb.getCorner( j ) );
			   v1.w = 1.f / fabsf( v1.w );
			   v1.x *= v1.w; v1.y *= v1.w; v1.z *= v1.w;
			
			   if( v1.x < bbMinX ) bbMinX = v1.x;
			   if( v1.y < bbMinY ) bbMinY = v1.y;
			   if( v1.z < bbMinZ ) bbMinZ = v1.z;
			   if( v1.x > bbMaxX ) bbMaxX = v1.x;
			   if( v1.y > bbMaxY ) bbMaxY = v1.y;
			   if( v1.z > bbMaxZ ) bbMaxZ = v1.z;
		   }
	   }
   }

	// Find post-projective space AABB of frustum slice if light is not inside
	if( frustSlice.cullSphere( _curLight->_absPos, 0 ) )
	{
		// Get frustum in post-projective space
		for( uint32 i = 0; i < 8; ++i )
		{
			// Frustum slice
			Vec4f v1 = lightViewProjMat * Vec4f( frustSlice.getCorner( i ) );
			v1.w = 1.f / fabsf( v1.w );  // Use absolute value to reduce problems with back projection when v1.w < 0
			v1.x *= v1.w; v1.y *= v1.w; v1.z *= v1.w;

			if( v1.x < frustMinX ) frustMinX = v1.x;
			if( v1.y < frustMinY ) frustMinY = v1.y;
			if( v1.z < frustMinZ ) frustMinZ = v1.z;
			if( v1.x > frustMaxX ) frustMaxX = v1.x;
			if( v1.y > frustMaxY ) frustMaxY = v1.y;
			if( v1.z > frustMaxZ ) frustMaxZ = v1.z;
		}
	}
	else
	{
		frustMinX = frustMinY = frustMinZ = -1;
		frustMaxX = frustMaxY = frustMaxZ = 1;
	}

	// Merge frustum and AABB bounds and clamp to post-projective range [-1, 1]
	float minX = clamp( maxf( frustMinX, bbMinX ), -1, 1 );
	float minY = clamp( maxf( frustMinY, bbMinY ), -1, 1 );
	float minZ = clamp( minf( frustMinZ, bbMinZ ), -1, 1 );
	float maxX = clamp( minf( frustMaxX, bbMaxX ), -1, 1 );
	float maxY = clamp( minf( frustMaxY, bbMaxY ), -1, 1 );
	float maxZ = clamp( minf( frustMaxZ, bbMaxZ ), -1, 1 );

	// Zoom-in slice to make better use of available shadow map space
	float scaleX = 2.0f / (maxX - minX);
	float scaleY = 2.0f / (maxY - minY);
	float scaleZ = 2.0f / (maxZ - minZ);

	float offsetX = -0.5f * (maxX + minX) * scaleX;
	float offsetY = -0.5f * (maxY + minY) * scaleY;
	float offsetZ = -0.5f * (maxZ + minZ) * scaleZ;

	// Build final matrix
	float cropMat[16] = { scaleX, 0, 0, 0,
	                      0, scaleY, 0, 0,
	                      0, 0, scaleZ, 0,
	                      offsetX, offsetY, offsetZ, 1 };

	return Matrix4f( cropMat );
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


Matrix4f Renderer::calcDirectionalLightShadowProj(const BoundingBox& worldBounds, const Frustum& frustSlice, 
                                                  const Matrix4f& lightViewMat, int numShadowMaps)
{
   int numShadowTiles = numShadowMaps > 1 ? 2 : 1;
   int shadowMapSize = Modules::config().shadowMapSize / numShadowTiles;
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


void Renderer::computeLightFrustumNearFar(const BoundingBox& worldBounds, const Matrix4f& lightViewMat, const Vec3f& lightMin, const Vec3f& lightMax, float* nearV, float* farV)
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


void Renderer::updateShadowMap(const Frustum* lightFrus, float minDist, float maxDist)
{
	if( _curLight == 0x0 ) return;
	
	uint32 prevRendBuf = gRDI->_curRendBuf;
	int prevVPX = gRDI->_vpX, prevVPY = gRDI->_vpY, prevVPWidth = gRDI->_vpWidth, prevVPHeight = gRDI->_vpHeight;
   RDIRenderBuffer &shadowRT = gRDI->_rendBufs.getRef( _shadowRB );
	gRDI->setViewport( 0, 0, shadowRT.width, shadowRT.height );
	gRDI->setRenderBuffer( _shadowRB );
	
	glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
	glDepthMask( GL_TRUE );
	gRDI->clear( CLR_DEPTH, 0x0, 1.f );

	// ********************************************************************************************
	// Cascaded Shadow Maps
	// ********************************************************************************************
	
	// Find AABB of lit geometry
	BoundingBox litAabb;

   std::ostringstream reason;
   reason << "update shadowmap for light " << _curLight->getName();

	Modules::sceneMan().updateQueues(reason.str().c_str(), *lightFrus, 0x0,
		RenderingOrder::None, SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, true );
	for( const auto& queue : Modules::sceneMan().getRenderableQueues() )
	{
      for (const auto& entry : queue.second) 
      {
         SceneNode* n = entry.node;
         litAabb.makeUnion(n->getBBox());
      }
	}

   // Calculate split distances using PSSM scheme
   const float nearDist = minDist;
   const float farDist = maxf( maxDist, minDist + 0.01f );
   const uint32 numMaps = _curLight->_shadowMapCount;
   const float lambda = _curLight->_shadowSplitLambda;
	
	_splitPlanes[0] = nearDist;
   _splitPlanes[numMaps] = farDist;
	
	for( uint32 i = 1; i < numMaps; ++i )
	{
		float f = (float)i / numMaps;
		float logDist = nearDist * powf( farDist / nearDist, f );
		float uniformDist = nearDist + (farDist - nearDist) * f;
		
		_splitPlanes[i] = (1 - lambda) * uniformDist + lambda * logDist;  // Lerp
	}

	// Prepare shadow map rendering
	glEnable( GL_DEPTH_TEST );
   gRDI->setShadowOffsets(2.0f, 4.0f);
	//glCullFace( GL_FRONT );	// Front face culling reduces artefacts but produces more "peter-panning"
	// Split viewing frustum into slices and render shadow maps
	Frustum frustum;
	for( uint32 i = 0; i < numMaps; ++i )
	{
		// Create frustum slice
		if( !_curCamera->_orthographic )
		{
			float newLeft = _curCamera->_frustLeft * _splitPlanes[i] / _curCamera->_frustNear;
			float newRight = _curCamera->_frustRight * _splitPlanes[i] / _curCamera->_frustNear;
			float newBottom = _curCamera->_frustBottom * _splitPlanes[i] / _curCamera->_frustNear;
			float newTop = _curCamera->_frustTop * _splitPlanes[i] / _curCamera->_frustNear;
			frustum.buildViewFrustum( _curCamera->_absTrans, newLeft, newRight, newBottom, newTop,
			                          _splitPlanes[i], _splitPlanes[i + 1] );
		}
		else
		{
			frustum.buildBoxFrustum( _curCamera->_absTrans, _curCamera->_frustLeft, _curCamera->_frustRight,
			                         _curCamera->_frustBottom, _curCamera->_frustTop,
			                         -_splitPlanes[i], -_splitPlanes[i + 1] );
		}

      //gRDI->_frameDebugInfo.addSplitFrustum_(frustum);
		
		// Get light projection matrix
		Matrix4f lightProjMat;
      Matrix4f lightViewMat = _curLight->getViewMat();
      Vec3f lightAbsPos;
      if ( !_curLight->_directional ) {
		   float ymax = _curCamera->_frustNear * tanf( degToRad( _curLight->_fov / 2 ) );
		   float xmax = ymax * 1.0f;  // ymax * aspect
         lightProjMat = Matrix4f::PerspectiveMat(-xmax, xmax, -ymax, ymax, _curCamera->_frustNear, _curLight->_radius );
         lightAbsPos = _curLight->_absPos;

         Matrix4f lightViewProjMat = lightProjMat * lightViewMat;
		   lightProjMat = calcCropMatrix( frustum, lightAbsPos, lightViewProjMat ) * lightProjMat;
      } else {
         lightAbsPos = (litAabb.min() + litAabb.max()) * 0.5f;
         lightViewMat = Matrix4f(_curLight->getViewMat());
         lightViewMat.x[12] = lightAbsPos.x;
         lightViewMat.x[13] = lightAbsPos.y;
         lightViewMat.x[14] = lightAbsPos.z;

         lightProjMat = calcDirectionalLightShadowProj(litAabb, frustum, lightViewMat, numMaps);
      }

		// Generate render queue with shadow casters for current slice
	   // Build optimized light projection matrix
		frustum.buildViewFrustum( lightViewMat, lightProjMat );
		Modules::sceneMan().updateQueues("rendering shadowmap", frustum, 0x0, RenderingOrder::None,
			SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, false, 0 );
		
      //gRDI->_frameDebugInfo.addShadowCascadeFrustum_(frustum);

		// Create texture atlas if several splits are enabled
		if( numMaps > 1 )
		{
			const int hsm = Modules::config().shadowMapSize / 2;
			const int scissorXY[8] = { 0, 0,  hsm, 0,  hsm, hsm,  0, hsm };
			const float transXY[8] = { -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f,  -0.5f, 0.5f };
			
			glEnable( GL_SCISSOR_TEST );

			// Select quadrant of shadow map
			lightProjMat.scale( 0.5f, 0.5f, 1.0f );
			lightProjMat.translate( transXY[i * 2], transXY[i * 2 + 1], 0.0f );
			gRDI->setScissorRect( scissorXY[i * 2], scissorXY[i * 2 + 1], hsm, hsm );
		}
	
      _lightMats[i] = lightProjMat * lightViewMat;
		setupViewMatrices( lightViewMat, lightProjMat );
		
		// Render at lodlevel = 1 (don't need the higher poly count for shadows, yay!)
		drawRenderables( _curLight->_shadowContext, "", false, &frustum, 0x0, RenderingOrder::None, -1, 1 );
	}

	// Map from post-projective space [-1,1] to texture space [0,1]
	for( uint32 i = 0; i < numMaps; ++i )
	{
      if (!_curLight->_directional) {
		   _lightMats[i].scale( 0.5f, 0.5f, 1.0f );
		   _lightMats[i].translate( 0.5f, 0.5f, 0.0f );
      } else {
         float m[] = {
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0
         };
         Matrix4f biasMatrix(m);
         _lightMats[i] = biasMatrix * _lightMats[i];
      }
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

void Renderer::updateLodUniform(int lodLevel, float lodDist1, float lodDist2)
{
   float nearV = _curCamera->getParamF(CameraNodeParams::NearPlaneF, 0);
   float farV = _curCamera->getParamF(CameraNodeParams::FarPlaneF, 0);
   _lodValues.x = (float)lodLevel;
   _lodValues.y = (1.0f - lodDist1) * nearV + (lodDist1 * farV);
   _lodValues.z = (1.0f - lodDist2) * nearV + (lodDist2 * farV);
   _lodValues.w = (_lodValues.y - _lodValues.z);
}

void Renderer::drawLodGeometry(std::string const& shaderContext, std::string const& theClass,
                             RenderingOrder::List order, int filterRequried, int occSet, float frustStart, float frustEnd, int lodLevel)
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
   Modules::sceneMan().updateQueues("drawing geometry", f, 0x0, order, SceneNodeFlags::NoDraw, 
      filterRequried, false, true );
	
	setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
	drawRenderables( shaderContext, theClass, false, &_curCamera->getFrustum(), 0x0, order, occSet, lodLevel );
}


void Renderer::drawGeometry( std::string const& shaderContext, std::string const& theClass,
                             RenderingOrder::List order, int filterRequired, int occSet, float frustStart, float frustEnd, int forceLodLevel )
{
   if (forceLodLevel >= 0) {
      drawLodGeometry(shaderContext, theClass, order, filterRequired, occSet, frustStart, frustEnd, forceLodLevel);
   } else {
      if (forceLodLevel == -1) {
         _lod_polygon_offset_x = 0.0;
         _lod_polygon_offset_y = -2.0;
      }

      float fStart = std::max(frustStart, 0.0f);
      float fEnd = std::min(0.41f, frustEnd);
      if (fStart < fEnd) {
         drawLodGeometry(shaderContext, theClass, order, filterRequired, occSet, fStart, fEnd, 0);
      }

      fStart = std::max(frustStart, 0.39f);
      fEnd = std::min(1.0f, frustEnd);
      if (fStart < fEnd) {
         drawLodGeometry(shaderContext, theClass, order, filterRequired, occSet, fStart, fEnd, 1);
      }

      _lod_polygon_offset_x = 0.0;
      _lod_polygon_offset_y = 0.0;
   }
}


void Renderer::drawProjections( std::string const& shaderContext, uint32 userFlags )
{
   int numProjectorNodes = Modules::sceneMan().findNodes(Modules::sceneMan().getRootNode(), "", SceneNodeTypes::ProjectorNode);

   setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );

   Matrix4f m;
   m.toIdentity();
   for (int i = 0; i < numProjectorNodes; i++)
   {
      ProjectorNode* n = (ProjectorNode*)Modules::sceneMan().getFindResult(i);
      
      _materialOverride = n->getMaterialRes();
      Frustum f;
      const BoundingBox& b = n->getBBox();
      f.buildBoxFrustum(m, b.min().x, b.max().x, b.min().y, b.max().y, b.min().z, b.max().z);
      Modules::sceneMan().updateQueues("drawing geometry", f, 0x0, RenderingOrder::None,
	                                    SceneNodeFlags::NoDraw, 0, false, true, false, userFlags);

      _projectorMat = n->getAbsTrans();

      // Don't need higher poly counts for projection geometry, so render at lod level 1.
      drawRenderables( shaderContext, "", false, &_curCamera->getFrustum(), 0x0, RenderingOrder::None, -1, 1);
   }

   _materialOverride = 0x0;
}


void Renderer::computeTightCameraBounds(float* minDist, float* maxDist)
{
   float defaultMax = *maxDist;
   float defaultMin = *minDist;
   *maxDist = defaultMin;
   *minDist = defaultMax;

   // First, get all the visible objects in the full camera's frustum.
   BoundingBox visibleAabb;
   Modules::sceneMan().updateQueues("computing tight camera", _curCamera->getFrustum(), 0x0,
      RenderingOrder::None, SceneNodeFlags::NoDraw | SceneNodeFlags::NoCastShadow, 0, false, true, true);
   for( const auto& queue : Modules::sceneMan().getRenderableQueues() )
   {
      for (const auto& entry : queue.second)
      {
         SceneNode* n = entry.node;
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


Frustum Renderer::computeDirectionalLightFrustum(float nearPlaneDist, float farPlaneDist)
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
      bb.addPoint(_curLight->getViewMat() * tightCamFrust.getCorner(i));
   }

   // Ten-thousand units ought to be enough for anyone.  (This value is how far our light is from
   // it's center-point; ideally, we'll always over-estimate here, and then dynamically adjust during
   // shadow-map construction to a tight bound over the light-visible geometry.)
   bb.addPoint(Vec3f(bb.min().x, bb.min().y, 10000));

   result.buildBoxFrustum(_curLight->getViewMat().inverted(), bb.min().x, bb.max().x, bb.min().y, bb.max().y, bb.max().z, bb.min().z);

   return result;
}


void Renderer::drawLightGeometry( std::string const& shaderContext, std::string const& theClass,
                                  bool noShadows, RenderingOrder::List order, int occSet, bool selectedOnly )
{
	Modules::sceneMan().updateQueues("drawing light geometry", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
	                                 SceneNodeFlags::NoDraw, 0, true, false );
	
	for( const auto& entry : Modules::sceneMan().getLightQueue() )
	{
		_curLight = (LightNode *)entry;
      const Frustum* lightFrus;
      Frustum dirLightFrus;

      // Save the far plane distance in case we have a directional light we want to cast shadows with.
      float maxDist = _curCamera->_frustFar;
      float minDist = _curCamera->_frustNear;

      if (!noShadows && _curLight->_shadowMapCount > 0)
      {
         computeTightCameraBounds(&minDist, &maxDist);
      }

      if (_curLight->_directional)
      {
         if (!noShadows)
         {
            dirLightFrus = computeDirectionalLightFrustum(minDist, maxDist);
            lightFrus = &dirLightFrus;
         } else {
            // If we're a directional light, and no shadows are to be cast, then just light
            // strictly only what is visible.
            lightFrus = 0x0;
         }
      } else {
         lightFrus = &(_curLight->getFrustum());
      }

		// Check if light is not visible
		if( lightFrus && _curCamera->getFrustum().cullFrustum( *lightFrus ) ) continue;

		// Check if light is occluded
		if( occSet >= 0 )
		{
			if( occSet > (int)_curLight->_occQueries.size() - 1 )
			{
				_curLight->_occQueries.resize( occSet + 1, 0 );
				_curLight->_lastVisited.resize( occSet + 1, 0 );
			}
			if( _curLight->_occQueries[occSet] == 0 )
			{
				_curLight->_occQueries[occSet] = gRDI->createOcclusionQuery();
				_curLight->_lastVisited[occSet] = 0;
			}
			else
			{
				if( _curLight->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					_curLight->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					Vec3f bbMin, bbMax;
					lightFrus->calcAABB( bbMin, bbMax );
					
					// Check that viewer is outside light bounds
					if( nearestDistToAABB( _curCamera->getFrustum().getOrigin(), bbMin, bbMax ) > 0 )
					{
						Modules::renderer().pushOccProxy( 1, bbMin, bbMax, _curLight->_occQueries[occSet] );

						// Check query result from previous frame
						if( gRDI->getQueryResult( _curLight->_occQueries[occSet] ) < 1 )
						{
							continue;
						}
					}
				}
			}
		}
	
		// Update shadow map
		if( !noShadows && _curLight->_shadowMapCount > 0 )
		{
			updateShadowMap(lightFrus, minDist, maxDist);
			setupShadowMap( false );
		}
		else
		{
			setupShadowMap( true );
		}
		
		// Calculate light screen space position
		float bbx, bby, bbw, bbh;
		_curLight->calcScreenSpaceAABB( _curCamera->getProjMat() * _curCamera->getViewMat(),
		                                bbx, bby, bbw, bbh );

		// Set scissor rectangle
		if( bbx != 0 || bby != 0 || bbw != 1 || bbh != 1 )
		{
			gRDI->setScissorRect( ftoi_r( bbx * gRDI->_fbWidth ), ftoi_r( bby * gRDI->_fbHeight ),
			                      ftoi_r( bbw * gRDI->_fbWidth ), ftoi_r( bbh * gRDI->_fbHeight ) );
			glEnable( GL_SCISSOR_TEST );
		}
		// Render
      std::ostringstream reason;
      reason << "drawing light geometry for light " << _curLight->getName();

      drawGeometry(_curLight->_lightingContext + "_" + _curPipeline->_pipelineName, theClass, order, 0, occSet, 0.0, 1.0);

		Modules().stats().incStat( EngineStats::LightPassCount, 1 );

		// Reset
		glDisable( GL_SCISSOR_TEST );
	}

	_curLight = 0x0;

	// Draw occlusion proxies
	if( occSet >= 0 )
	{
		setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
		Modules::renderer().drawOccProxies( 1 );
	}
}


void Renderer::drawLightShapes( std::string const& shaderContext, bool noShadows, int occSet )
{
	MaterialResource *curMatRes = 0x0;
	
	Modules::sceneMan().updateQueues( "drawing light shapes", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
	                                  SceneNodeFlags::NoDraw, 0, true, false );
		
	for( const auto& entry : Modules::sceneMan().getLightQueue() )
	{
		_curLight = (LightNode *)entry;

		// Check if light is not visible
      if( !_curLight->_directional && _curCamera->getFrustum().cullFrustum( _curLight->getFrustum() ) ) {
         continue;
      }

      const Frustum* lightFrus;
      Frustum dirLightFrus;

      // Save the far plane distance in case we have a directional light we want to cast shadows with.
      float maxDist = _curCamera->_frustFar;
      float minDist = _curCamera->_frustNear;

      if (!noShadows && _curLight->_shadowMapCount > 0)
      {
         computeTightCameraBounds(&minDist, &maxDist);
      }

      if (_curLight->_directional)
      {
         dirLightFrus = computeDirectionalLightFrustum(minDist, maxDist);
         lightFrus = &dirLightFrus;
      } else {
         lightFrus = &(_curLight->getFrustum());
      }
		
		// Check if light is occluded
		if( occSet >= 0 )
		{
			if( occSet > (int)_curLight->_occQueries.size() - 1 )
			{
				_curLight->_occQueries.resize( occSet + 1, 0 );
				_curLight->_lastVisited.resize( occSet + 1, 0 );
			}
			if( _curLight->_occQueries[occSet] == 0 )
			{
				_curLight->_occQueries[occSet] = gRDI->createOcclusionQuery();
				_curLight->_lastVisited[occSet] = 0;
			}
			else
			{
				if( _curLight->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					_curLight->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					Vec3f bbMin, bbMax;
					lightFrus->calcAABB( bbMin, bbMax );
					
					// Check that viewer is outside light bounds
					if( nearestDistToAABB( _curCamera->getFrustum().getOrigin(), bbMin, bbMax ) > 0 )
					{
						Modules::renderer().pushOccProxy( 1, bbMin, bbMax, _curLight->_occQueries[occSet] );

						// Check query result from previous frame
						if( gRDI->getQueryResult( _curLight->_occQueries[occSet] ) < 1 )
						{
							continue;
						}
					}
				}
			}
		}
		
		// Update shadow map
		if( !noShadows && _curLight->_shadowMapCount > 0 )
		{	
         updateShadowMap(lightFrus, minDist, maxDist);
			setupShadowMap( false );
			curMatRes = 0x0;
		}
		else
		{
			setupShadowMap( true );
		}

		setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );

      // TODO(klochek): wait, what?  Pretty sure this is bogus, now.
      if (!_curLight->_directional) {
			commitGeneralUniforms();
      }

		glCullFace( GL_FRONT );
		glDisable( GL_DEPTH_TEST );

		if (_curLight->_directional) {
         drawFSQuad(curMatRes, _curLight->_lightingContext + "_" + _curPipeline->_pipelineName);
      } else if (_curLight->_fov < 180) {
			float r = _curLight->_radius * tanf( degToRad( _curLight->_fov / 2 ) );
			drawCone( _curLight->_radius, r, _curLight->_absTrans );
		} else {
			drawSphere( _curLight->_absPos, _curLight->_radius );
		}

		Modules().stats().incStat( EngineStats::LightPassCount, 1 );

		// Reset
		glEnable( GL_DEPTH_TEST );
		glCullFace( GL_BACK );
	}

	_curLight = 0x0;

	// Draw occlusion proxies
	if( occSet >= 0 )
	{
		setupViewMatrices( _curCamera->getViewMat(), _curCamera->getProjMat() );
		Modules::renderer().drawOccProxies( 1 );
	}
}


// =================================================================================================
// Scene Node Rendering Functions
// =================================================================================================

void Renderer::setGlobalUniform(const char* str, UniformType::List kind, void* value)
{
   float *f = (float*)value;
   if (kind == UniformType::FLOAT) {
      _uniformFloats[std::string(str)] = *f;
   } else if (kind == UniformType::VEC4) {
      _uniformVecs[std::string(str)] = Vec4f(f[0], f[1], f[2], f[3]);
   } else if (kind == UniformType::MAT44) {
      _uniformMats[std::string(str)] = Matrix4f(f);
   }
}


void Renderer::drawRenderables( std::string const& shaderContext, std::string const& theClass, bool debugView,
                                const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                                int occSet, int lodLevel )
{
	ASSERT( _curCamera != 0x0 );
	
	if( Modules::config().wireframeMode && !Modules::config().debugViewMode )
	{
		glDisable( GL_CULL_FACE );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	}
	else
	{
		glEnable( GL_CULL_FACE );
	}
	
	map< int, NodeRegEntry >::const_iterator itr = Modules::sceneMan()._registry.begin();
	while( itr != Modules::sceneMan()._registry.end() )
	{
		if( itr->second.renderFunc != 0x0 ) {
			(*itr->second.renderFunc)( shaderContext, theClass, debugView, frust1, frust2, order, occSet, lodLevel );
      }

      if (itr->second.instanceRenderFunc != 0x0) {
         (*itr->second.instanceRenderFunc)( shaderContext, theClass, debugView, frust1, frust2, order, occSet, lodLevel );
      }


		++itr;
	}

	// Reset states
	if( Modules::config().wireframeMode && !Modules::config().debugViewMode )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}


void Renderer::drawMeshes( std::string const& shaderContext, std::string const& theClass, bool debugView,
                           const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                           int occSet, int lodLevel )
{
	if( frust1 == 0x0 ) return;
	
	GeometryResource *curGeoRes = 0x0;
	MaterialResource *curMatRes = 0x0;

	// Loop over mesh queue
	for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::Mesh) )
	{
		MeshNode *meshNode = (MeshNode *)entry.node;
		ModelNode *modelNode = meshNode->getParentModel();
		
		// Check that mesh is valid
		if( modelNode->getGeometryResource() == 0x0 )
			continue;
		if( meshNode->getBatchStart() + meshNode->getBatchCount() > modelNode->getGeometryResource()->_indexCount )
			continue;
		
      bool modelChanged = true;
		uint32 queryObj = 0;

		// Occlusion culling
		if( occSet >= 0 )
		{
			if( occSet > (int)meshNode->_occQueries.size() - 1 )
			{
				meshNode->_occQueries.resize( occSet + 1, 0 );
				meshNode->_lastVisited.resize( occSet + 1, 0 );
			}
			if( meshNode->_occQueries[occSet] == 0 )
			{
				queryObj = gRDI->createOcclusionQuery();
				meshNode->_occQueries[occSet] = queryObj;
				meshNode->_lastVisited[occSet] = 0;
			}
			else
			{
				if( meshNode->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					meshNode->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					// Check query result (viewer must be outside of bounding box)
					if( nearestDistToAABB( frust1->getOrigin(), meshNode->getBBox().min(),
					                       meshNode->getBBox().max() ) > 0 &&
						gRDI->getQueryResult( meshNode->_occQueries[occSet] ) < 1 )
					{
						Modules::renderer().pushOccProxy( 0, meshNode->getBBox().min(), meshNode->getBBox().max(),
						                                  meshNode->_occQueries[occSet] );
						continue;
					}
					else
						queryObj = meshNode->_occQueries[occSet];
				}
			}
		}
		
		// Bind geometry
		if( curGeoRes != modelNode->getGeometryResource() )
		{
			curGeoRes = modelNode->getGeometryResource();
			ASSERT( curGeoRes != 0x0 );
		
			// Indices
			gRDI->setIndexBuffer( curGeoRes->getIndexBuf(),
			                      curGeoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

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
			if( !meshNode->getMaterialRes()->isOfClass( theClass ) ) continue;
			
			// Set material
			if( curMatRes != meshNode->getMaterialRes() )
			{
				if( !Modules::renderer().setMaterial( meshNode->getMaterialRes(), shaderContext ) )
				{	
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
		gRDI->drawIndexed( PRIM_TRILIST, meshNode->getBatchStart(), meshNode->getBatchCount(),
		                   meshNode->getVertRStart(), meshNode->getVertREnd() - meshNode->getVertRStart() + 1 );
		Modules::stats().incStat( EngineStats::BatchCount, 1 );
		Modules::stats().incStat( EngineStats::TriCount, meshNode->getBatchCount() / 3.0f );

		if( queryObj )
			gRDI->endQuery( queryObj );
	}

	// Draw occlusion proxies
	if( occSet >= 0 )
		Modules::renderer().drawOccProxies( 0 );

	gRDI->setVertexLayout( 0 );
}


void Renderer::drawHudElements(std::string const& shaderContext, std::string const& theClass, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   radiant::perfmon::TimelineCounterGuard dvm("drawHudElements");
	if( frust1 == 0x0 ) return;
	
	for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::HudElement) )
	{
      HudElementNode* hudNode = (HudElementNode*) entry.node;
      
      for (const auto& hudElement : hudNode->getSubElements())
      {
         hudElement->draw(shaderContext, theClass, hudNode->_absTrans);
      }
   }
	gRDI->setVertexLayout( 0 );
}


void Renderer::drawVoxelMeshes(std::string const& shaderContext, std::string const& theClass, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   radiant::perfmon::TimelineCounterGuard dvm("drawVoxelMeshes");
	if( frust1 == 0x0 ) return;
	
	VoxelGeometryResource *curVoxelGeoRes = 0x0;
	MaterialResource *curMatRes = 0x0;

	// Loop over mesh queue
	for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::VoxelMesh) )
	{
		VoxelMeshNode *meshNode = (VoxelMeshNode *)entry.node;
		VoxelModelNode *modelNode = meshNode->getParentModel();
		
		// Check that mesh is valid
		if( modelNode->getVoxelGeometryResource() == 0x0 )
			continue;
		if( meshNode->getBatchStart(lodLevel) + meshNode->getBatchCount(lodLevel) > modelNode->getVoxelGeometryResource()->_indexCount )
			continue;
		
		uint32 queryObj = 0;

		// Occlusion culling
		if( occSet >= 0 )
		{
			if( occSet > (int)meshNode->_occQueries.size() - 1 )
			{
				meshNode->_occQueries.resize( occSet + 1, 0 );
				meshNode->_lastVisited.resize( occSet + 1, 0 );
			}
			if( meshNode->_occQueries[occSet] == 0 )
			{
				queryObj = gRDI->createOcclusionQuery();
				meshNode->_occQueries[occSet] = queryObj;
				meshNode->_lastVisited[occSet] = 0;
			}
			else
			{
				if( meshNode->_lastVisited[occSet] != Modules::renderer().getFrameID() )
				{
					meshNode->_lastVisited[occSet] = Modules::renderer().getFrameID();
				
					// Check query result (viewer must be outside of bounding box)
					if( nearestDistToAABB( frust1->getOrigin(), meshNode->getBBox().min(),
					                       meshNode->getBBox().max() ) > 0 &&
						gRDI->getQueryResult( meshNode->_occQueries[occSet] ) < 1 )
					{
						Modules::renderer().pushOccProxy( 0, meshNode->getBBox().min(), meshNode->getBBox().max(),
						                                  meshNode->_occQueries[occSet] );
						continue;
					}
					else
						queryObj = meshNode->_occQueries[occSet];
				}
			}
		}
		
		// Bind geometry
		if( curVoxelGeoRes != modelNode->getVoxelGeometryResource() )
		{
			curVoxelGeoRes = modelNode->getVoxelGeometryResource();
			ASSERT( curVoxelGeoRes != 0x0 );
		
			// Indices
			gRDI->setIndexBuffer( curVoxelGeoRes->getIndexBuf(),
			                      curVoxelGeoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

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
               return;
				}
				curMatRes = Modules::renderer()._materialOverride;
         } else {
			   if( !meshNode->getMaterialRes()->isOfClass( theClass ) ) continue;
			
			   // Set material
			   if( curMatRes != meshNode->getMaterialRes() )
			   {
				   if( !Modules::renderer().setMaterial( meshNode->getMaterialRes(), shaderContext ) )
				   {	
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

		// Render
		gRDI->drawIndexed( PRIM_TRILIST, meshNode->getBatchStart(lodLevel), meshNode->getBatchCount(lodLevel),
		                   meshNode->getVertRStart(lodLevel), meshNode->getVertREnd(lodLevel) - meshNode->getVertRStart(lodLevel) + 1 );
		Modules::stats().incStat( EngineStats::BatchCount, 1 );
		Modules::stats().incStat( EngineStats::TriCount, meshNode->getBatchCount(lodLevel) / 3.0f );

		if( queryObj )
			gRDI->endQuery( queryObj );
	}

	// Draw occlusion proxies
	if( occSet >= 0 )
		Modules::renderer().drawOccProxies( 0 );

	gRDI->setVertexLayout( 0 );
   glDisable(GL_POLYGON_OFFSET_FILL);
}


void Renderer::drawVoxelMeshes_Instances(std::string const& shaderContext, std::string const& theClass, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
   const unsigned int VoxelInstanceCutoff = 10;
   radiant::perfmon::TimelineCounterGuard dvm("drawVoxelMeshes_Instances");
	if( frust1 == 0x0 ) return;
	
	MaterialResource *curMatRes = 0x0;

   // Loop over mesh queue
	for( const auto& instanceKind : Modules::sceneMan().getInstanceRenderableQueue(SceneNodeTypes::VoxelMesh) )
	{
      const InstanceKey& instanceKey = instanceKind.first;
      const VoxelGeometryResource *curVoxelGeoRes = (VoxelGeometryResource*) instanceKind.first.geoResource;
		// Check that mesh is valid
		if(curVoxelGeoRes == 0x0) {
			continue;
      }

      if (instanceKind.second.size() <= 0) {
         continue;
      }

      bool useInstancing = instanceKind.second.size() >= VoxelInstanceCutoff && gRDI->getCaps().hasInstancing;
      Modules::config().setGlobalShaderFlag("DRAW_WITH_INSTANCING", useInstancing);

      // TODO(klochek): awful--but how to fix?  We can keep cramming stuff into the InstanceKey, but to what end?
      VoxelMeshNode* vmn = (VoxelMeshNode*)instanceKind.second.front().node;

		// Bind geometry
		// Indices
		gRDI->setIndexBuffer(curVoxelGeoRes->getIndexBuf(),
			                     curVoxelGeoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

		// Vertices
      gRDI->setVertexBuffer( 0, curVoxelGeoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );

		
		ShaderCombination *prevShader = Modules::renderer().getCurShader();
		
		if( !debugView )
		{
         if (Modules::renderer()._materialOverride != 0x0) {
				if( !Modules::renderer().setMaterial(Modules::renderer()._materialOverride, shaderContext ) )
				{	
               return;
				}
				curMatRes = Modules::renderer()._materialOverride;
         } else {
            if( !instanceKey.matResource->isOfClass( theClass ) ) continue;
			
			   // Set material
			   //if( curMatRes != instanceKey.matResource )
			   //{
               if( !Modules::renderer().setMaterial( instanceKey.matResource, shaderContext ) )
				   {	
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

      if (useInstancing) {
         drawVoxelMesh_Instances_WithInstancing(instanceKind.second, vmn, lodLevel);
      } else {
         drawVoxelMesh_Instances_WithoutInstancing(instanceKind.second, vmn, lodLevel);
      }
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
   float* transformBuffer = _vbInstanceVoxelBuf;
   ASSERT(renderableQueue.size() <= MaxVoxelInstanceCount);
   for (const auto& node : renderableQueue) {
		const VoxelMeshNode *meshNode = (VoxelMeshNode *)node.node;
		const VoxelModelNode *modelNode = meshNode->getParentModel();
		
      memcpy(transformBuffer, &meshNode->_absTrans.x[0], sizeof(float) * 16);
      transformBuffer += 16;
   }
   gRDI->updateBufferData(_vbInstanceVoxelData, 0, (transformBuffer - _vbInstanceVoxelBuf) * sizeof(float), _vbInstanceVoxelBuf);

   radiant::perfmon::SwitchToCounter("draw mesh instances");
   // Draw instanced meshes.
   gRDI->setVertexBuffer(1, _vbInstanceVoxelData, 0, sizeof(float) * 16);
   gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, vmn->getBatchCount(lodLevel), vmn->getBatchStart(lodLevel), renderableQueue.size());
   radiant::perfmon::SwitchToCounter("drawVoxelMeshes_Instances");

	// Render
	Modules::stats().incStat( EngineStats::BatchCount, 1 );
   Modules::stats().incStat( EngineStats::TriCount, (vmn->getBatchCount(lodLevel) / 3.0f) * renderableQueue.size() );
}

void Renderer::drawVoxelMesh_Instances_WithoutInstancing(const RenderableQueue& renderableQueue, const VoxelMeshNode* vmn, int lodLevel)
{
   // Collect transform data for every node of this mesh/material kind.
   radiant::perfmon::SwitchToCounter("draw mesh instances");
   // Set vertex layout
   gRDI->setVertexLayout( Modules::renderer()._vlVoxelModel );
   ShaderCombination* curShader = Modules::renderer().getCurShader();
   for (const auto& node : renderableQueue) {
		VoxelMeshNode *meshNode = (VoxelMeshNode *)node.node;
		const VoxelModelNode *modelNode = meshNode->getParentModel();
		
      gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &meshNode->_absTrans.x[0] );
      gRDI->drawIndexed(RDIPrimType::PRIM_TRILIST, vmn->getBatchStart(lodLevel), vmn->getBatchCount(lodLevel),
         vmn->getVertRStart(lodLevel), vmn->getVertREnd(lodLevel) - vmn->getVertRStart(lodLevel) + 1);

      Modules::stats().incStat( EngineStats::BatchCount, 1 );
   }
   Modules::stats().incStat( EngineStats::TriCount, (vmn->getBatchCount(lodLevel) / 3.0f) * renderableQueue.size() );
}


void Renderer::drawInstanceNode(std::string const& shaderContext, std::string const& theClass, bool debugView,
                               const Frustum *frust1, const Frustum *frust2, RenderingOrder::List order,
                               int occSet, int lodLevel)
{
	if( frust1 == 0x0 || Modules::renderer().getCurCamera() == 0x0 ) return;
	if( debugView ) return;  // Don't render in debug view

   bool useInstancing = gRDI->getCaps().hasInstancing;
   Modules::config().setGlobalShaderFlag("DRAW_WITH_INSTANCING", useInstancing);
	
   if (useInstancing) {   
      gRDI->setVertexLayout( Modules::renderer()._vlInstanceVoxelModel );
	   MaterialResource *curMatRes = 0x0;
      for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::InstanceNode) )
	   {
		   InstanceNode* in = (InstanceNode *)entry.node;

		   if( !in->_matRes->isOfClass( theClass ) ) continue;

         gRDI->setVertexBuffer( 0, in->_geoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );
         gRDI->setVertexBuffer( 1, in->_instanceBufObj, 0, 16 * sizeof(float) );
         gRDI->setIndexBuffer( in->_geoRes->getIndexBuf(), in->_geoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

         if( curMatRes != in->_matRes )
		   {
            if( !Modules::renderer().setMaterial( in->_matRes, shaderContext ) ) continue;
			   curMatRes = in->_matRes;
		   }

         gRDI->drawInstanced(RDIPrimType::PRIM_TRILIST, 
            in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoIndexCountI), 
            0, in->_usedInstances);
	   }
   } else {
      gRDI->setVertexLayout( Modules::renderer()._vlVoxelModel );
	   MaterialResource *curMatRes = 0x0;
      for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::InstanceNode) )
	   {
		   InstanceNode* in = (InstanceNode *)entry.node;

		   if( !in->_matRes->isOfClass( theClass ) ) continue;

         gRDI->setVertexBuffer( 0, in->_geoRes->getVertexBuf(), 0, sizeof( VoxelVertexData ) );
         gRDI->setIndexBuffer( in->_geoRes->getIndexBuf(), in->_geoRes->_16BitIndices ? IDXFMT_16 : IDXFMT_32 );

         if( curMatRes != in->_matRes )
		   {
            if( !Modules::renderer().setMaterial( in->_matRes, shaderContext ) ) continue;
			   curMatRes = in->_matRes;
		   }
         ShaderCombination* curShader = Modules::renderer().getCurShader();

         for (int i = 0; i < in->_usedInstances; i++) {
            gRDI->setShaderConst( curShader->uni_worldMat, CONST_FLOAT44, &in->_instanceBuf[i * 16]);
            gRDI->drawIndexed(RDIPrimType::PRIM_TRILIST, 0, 
               in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoIndexCountI), 
               0, in->_geoRes->getElemParamI(VoxelGeometryResData::VoxelGeometryElem, 0, VoxelGeometryResData::VoxelGeoVertexCountI) - 1);
         }
	   }
   }

	gRDI->setVertexLayout( 0 );
}


void Renderer::drawParticles( std::string const& shaderContext, std::string const& theClass, bool debugView,
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
	for( const auto& entry : Modules::sceneMan().getRenderableQueue(SceneNodeTypes::Emitter) )
	{
		EmitterNode *emitter = (EmitterNode *)entry.node;
		
		if( emitter->_particleCount == 0 ) continue;
		if( !emitter->_materialRes->isOfClass( theClass ) ) continue;
		
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
               if (rendBuf == _shadowRB) {
                  // unbind the shadow texture
                  setupShadowMap( true );
               }
				   bindPipeBuffer( rendBuf, pc.params[1].getString(), (uint32)pc.params[2].getInt() );
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
				drawGeometry( pc.params[0].getString(), pc.params[1].getString(),
				              (RenderingOrder::List)pc.params[2].getInt(),
                          pc.params[3].getInt(), _curCamera->_occSet, pc.params[4].getFloat(), pc.params[5].getFloat(), pc.params[6].getInt() );
				break;

         case PipelineCommands::DrawProjections:
            drawProjections(pc.params[0].getString(), pc.params[1].getInt());
            break;

			case PipelineCommands::DrawOverlays:
            drawOverlays( pc.params[0].getString() );
				break;

			case PipelineCommands::DrawQuad:
				drawFSQuad( pc.params[0].getResource(), pc.params[1].getString() );
			break;

			case PipelineCommands::DoForwardLightLoop:
				drawLightGeometry( pc.params[0].getString(), pc.params[1].getString(),
               pc.params[2].getBool() || !drawShadows, (RenderingOrder::List)pc.params[3].getInt(),
                               _curCamera->_occSet, pc.params[4].getBool() );
				break;

			case PipelineCommands::DoDeferredLightLoop:
				drawLightShapes( pc.params[0].getString(), pc.params[1].getBool() || !drawShadows, 
               _curCamera->_occSet );
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
	Timer *timer = Modules::stats().getTimer( EngineStats::FrameTime );
	ASSERT( timer != 0x0 );

   Modules::stats().getStat( EngineStats::FrameTime, true );  // Reset
	Modules::stats().incStat( EngineStats::FrameTime, timer->getElapsedTimeMS() );
   logPerformanceData();
	timer->reset();
   Modules::sceneMan().clearQueryCache();
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

   Modules::sceneMan().updateQueues( "rendering debug view", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
	                                 SceneNodeFlags::NoDraw, 0, true, true, true );
	gRDI->setShaderConst( Modules::renderer()._defColShader_color, CONST_FLOAT4, &color[0] );
	//drawRenderables( "", "", true, &_curCamera->getFrustum(), 0x0, RenderingOrder::None, -1, 0 );
   for (const auto& queue : Modules::sceneMan().getRenderableQueues())
   {
      for( const auto& entry : queue.second )
	   {
		   const SceneNode *sn = entry.node;
		   drawAABB( sn->_bBox.min(), sn->_bBox.max() );
	   }
   }

	Modules::sceneMan().updateQueues( "rendering debug view", _curCamera->getFrustum(), 0x0, RenderingOrder::None,
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
	for( size_t i = 0, s = Modules::sceneMan().getLightQueue().size(); i < s; ++i )
	{
		LightNode *lightNode = (LightNode *)Modules::sceneMan().getLightQueue()[i];
		
		if( lightNode->_fov < 180 )
		{
			float r = lightNode->_radius * tanf( degToRad( lightNode->_fov / 2 ) );
			drawCone( lightNode->_radius, r, lightNode->_absTrans );
		}
		else
		{
			drawSphere( lightNode->_absPos, lightNode->_radius );
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
