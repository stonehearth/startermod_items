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

#include "egLight.h"
#include "egMaterial.h"
#include "egModules.h"
#include "egCom.h"
#include "egRenderer.h"

#include "utDebug.h"


namespace Horde3D {

using namespace std;


LightNode::LightNode( const LightNodeTpl &lightTpl ) :
	SceneNode( lightTpl )
{
   _directional = lightTpl.directional;
	_lightingContext = lightTpl.lightingContext;
	_shadowContext = lightTpl.shadowContext;
	_radius1 = lightTpl.radius1; _radius2 = lightTpl.radius2; _fov = lightTpl.fov;
	_diffuseCol = Vec3f( lightTpl.col_R, lightTpl.col_G, lightTpl.col_B );
   _ambientCol = Vec3f( lightTpl.ambCol_R, lightTpl.ambCol_G, lightTpl.ambCol_B );
	_diffuseColMult = lightTpl.colMult;
	_shadowMapCount = lightTpl.shadowMapCount;
	_shadowSplitLambda = lightTpl.shadowSplitLambda;
	_shadowMapBias = lightTpl.shadowMapBias;
   _importance = lightTpl.importance;
   _shadowMapQuality = lightTpl.shadowMapQuality;
   _shadowMapBuffer = 0;
   _shadowMapSize = 0;

   if (!_directional && _shadowMapCount > 0) {
      registerForNodeTracking();
   }

   reallocateShadowBuffer(gRenderer->calculateShadowBufferSize(this));

   setFlags(SceneNodeFlags::NoRayQuery, false);
}

void LightNode::registerForNodeTracking()
{
   Modules::sceneMan().registerNodeTracker(this, [this](SceneNode const*n) {
      for (int i = 0; i < 6; i++) {
         if (_dirtyShadows[i]) {
            continue;
         }

         if (n->getFlags() & SceneNodeFlags::NoCastShadow) {
            continue;
         }

         const int nodeType = n->getType();
         if (nodeType == SceneNodeTypes::Camera || nodeType == SceneNodeTypes::Group ||
            nodeType == SceneNodeTypes::HudElement || nodeType == SceneNodeTypes::Joint ||
            nodeType == SceneNodeTypes::Light || nodeType == SceneNodeTypes::ProjectorNode)
         {
            continue;
         }

         if (!getCubeFrustum((LightCubeFace::List)i).cullBox(n->getBBox())) {
            _dirtyShadows[i] = true;
         }
      }
   });
}


void LightNode::dirtyCubeFrustums()
{
   for (int i = 0; i < 6; i++) {
      _dirtyShadows[i] = true;
   }
}


void LightNode::reallocateShadowBuffer(int size)
{
   if (!_directional) {
      if (_shadowMapBuffer) {
         gRDI->destroyRenderBuffer(_shadowMapBuffer);
         _shadowMapBuffer = 0;
      }
   }
   _shadowMapSize = size;

   if (_shadowMapSize == 0 || _shadowMapCount == 0) {
      _shadowMapCount = 0;
      return;
   }

   if (Modules::config().getOption(EngineOptions::EnableShadows)) {
      if (_directional) {
         // For now, use a global 'master' shadow cascade buffer for directional lights.
         _shadowMapBuffer = gRenderer->getShadowCascadeBuffer();
      } else {
         _shadowMapBuffer = gRDI->createRenderBuffer(_shadowMapSize, _shadowMapSize, TextureFormats::R32, true, 1, 0, 0, true);
         dirtyCubeFrustums();
      }
      if (!_shadowMapBuffer)
	   {
		   Modules::log().writeError("Failed to create shadow map");
	   }
   }
}

LightNode::~LightNode()
{
   if (_shadowMapBuffer && !_directional) {
      gRDI->destroyRenderBuffer(_shadowMapBuffer);
      _shadowMapBuffer = 0;
   }
   Modules::sceneMan().clearNodeTracker(this);
}


SceneNodeTpl *LightNode::parsingFunc( map< string, std::string > &attribs )
{
	bool result = true;
	
	map< string, std::string >::iterator itr;
	LightNodeTpl *lightTpl = new LightNodeTpl( "", "", "" );

	itr = attribs.find( "lightingContext" );
	if( itr != attribs.end() ) lightTpl->lightingContext = itr->second;
	else result = false;
	itr = attribs.find( "shadowContext" );
	if( itr != attribs.end() ) lightTpl->shadowContext = itr->second;
	else result = false;
	itr = attribs.find( "radius" );
	if( itr != attribs.end() ) lightTpl->radius1 = (float)atof( itr->second.c_str() );
	itr = attribs.find( "fov" );
	if( itr != attribs.end() ) lightTpl->fov = (float)atof( itr->second.c_str() );
	itr = attribs.find( "col_R" );
	if( itr != attribs.end() ) lightTpl->col_R = (float)atof( itr->second.c_str() );
	itr = attribs.find( "col_G" );
	if( itr != attribs.end() ) lightTpl->col_G = (float)atof( itr->second.c_str() );
	itr = attribs.find( "col_B" );
	if( itr != attribs.end() ) lightTpl->col_B = (float)atof( itr->second.c_str() );
	itr = attribs.find( "colMult" );
	if( itr != attribs.end() ) lightTpl->colMult = (float)atof( itr->second.c_str() );
	itr = attribs.find( "ambCol_R" );
	if( itr != attribs.end() ) lightTpl->ambCol_R = (float)atof( itr->second.c_str() );
	itr = attribs.find( "ambCol_G" );
	if( itr != attribs.end() ) lightTpl->ambCol_G = (float)atof( itr->second.c_str() );
	itr = attribs.find( "ambCol_B" );
	if( itr != attribs.end() ) lightTpl->ambCol_B = (float)atof( itr->second.c_str() );
	itr = attribs.find( "shadowMapCount" );
	if( itr != attribs.end() ) lightTpl->shadowMapCount = atoi( itr->second.c_str() );
	itr = attribs.find( "shadowSplitLambda" );
	if( itr != attribs.end() ) lightTpl->shadowSplitLambda = (float)atof( itr->second.c_str() );
	itr = attribs.find( "shadowMapBias" );
	if( itr != attribs.end() ) lightTpl->shadowMapBias = (float)atof( itr->second.c_str() );
	
	if( !result )
	{
		delete lightTpl; lightTpl = 0x0;
	}
	
	return lightTpl;
}


SceneNode *LightNode::factoryFunc( const SceneNodeTpl &nodeTpl )
{
	if( nodeTpl.type != SceneNodeTypes::Light ) return 0x0;

	return new LightNode( *(LightNodeTpl *)&nodeTpl );
}


int LightNode::getParamI( int param ) const
{
	switch( param )
	{
	case LightNodeParams::ShadowMapCountI:
		return _shadowMapCount;
   case LightNodeParams::ImportanceI:
      return _importance;
   case LightNodeParams::DirectionalI:
      return _directional ? 1 : 0;
   case LightNodeParams::ShadowMapQualityI:
      return _shadowMapQuality;
	}

	return SceneNode::getParamI( param );
}


void LightNode::setParamI( int param, int value )
{
	switch( param )
	{
	case LightNodeParams::ShadowMapCountI:
		if( value == 0 || value == 1 || value == 2 || value == 3 || value == 4 ) {
         if (_shadowMapCount == 0 && value != 0) {
            registerForNodeTracking();
         } else if (value == 0) {
            Modules::sceneMan().clearNodeTracker(this);
         }
			_shadowMapCount = (uint32)value;
         reallocateShadowBuffer(gRenderer->calculateShadowBufferSize(this));
      } else {
			Modules::setError( "Invalid value in h3dSetNodeParamI for H3DLight::ShadowMapCountI" );
      }
		return;
   case LightNodeParams::ImportanceI:
      _importance = value;
      return;
   case LightNodeParams::ShadowMapQualityI:
      _shadowMapQuality = value;
      reallocateShadowBuffer(gRenderer->calculateShadowBufferSize(this));
      return;
	}

	return SceneNode::setParamI( param, value );
}


float LightNode::getParamF( int param, int compIdx )
{
	switch( param )
	{
	case LightNodeParams::Radius1F:
		return _radius1;
	case LightNodeParams::Radius2F:
		return _radius2;
	case LightNodeParams::ColorF3:
		if( (unsigned)compIdx < 3 ) return _diffuseCol[compIdx];
		break;
	case LightNodeParams::ColorMultiplierF:
		return _diffuseColMult;
	case LightNodeParams::AmbientColorF3:
		if( (unsigned)compIdx < 3 ) return _ambientCol[compIdx];
		break;
	case LightNodeParams::FovF:
		return _fov;
	case LightNodeParams::ShadowSplitLambdaF:
		return _shadowSplitLambda;
	case LightNodeParams::ShadowMapBiasF:
		return _shadowMapBias;
   case LightNodeParams::DirtyShadowsI:
      return _dirtyShadows[compIdx] ? 1.0f : 0.0f;
	}

	return SceneNode::getParamF( param, compIdx );
}


void LightNode::setParamF( int param, int compIdx, float value )
{
	switch( param )
	{
	case LightNodeParams::Radius1F:
		_radius1 = value;
		markDirty(SceneNodeDirtyKind::Ancestors);
		return;
	case LightNodeParams::Radius2F:
		_radius2 = value;
		markDirty(SceneNodeDirtyKind::Ancestors);
		return;
	case LightNodeParams::FovF:
		_fov = value;
		markDirty(SceneNodeDirtyKind::Ancestors);
		return;
	case LightNodeParams::ColorF3:
		if( (unsigned)compIdx < 3 )
		{
			_diffuseCol[compIdx] = value;
			return;
		}
		break;
	case LightNodeParams::ColorMultiplierF:
		_diffuseColMult = value;
		return;
	case LightNodeParams::AmbientColorF3:
		if( (unsigned)compIdx < 3 )
		{
			_ambientCol[compIdx] = value;
			return;
		}
		break;
	case LightNodeParams::ShadowSplitLambdaF:
		_shadowSplitLambda = value;
		return;
	case LightNodeParams::ShadowMapBiasF:
		_shadowMapBias = value;
		return;
	}

	SceneNode::setParamF( param, compIdx, value );
}


const char *LightNode::getParamStr( int param )
{
	switch( param )
	{
	case LightNodeParams::LightingContextStr:
		return _lightingContext.c_str();
	case LightNodeParams::ShadowContextStr:
		return _shadowContext.c_str();
	}

	return SceneNode::getParamStr( param );
}


void LightNode::setParamStr( int param, const char *value )
{
	switch( param )
	{
	case LightNodeParams::LightingContextStr:
		_lightingContext = value;
		return;
	case LightNodeParams::ShadowContextStr:
      if (_shadowContext != value) {
         dirtyCubeFrustums();
      }
		_shadowContext = value;
		return;
	}

	SceneNode::setParamStr( param, value );
}


void LightNode::calcScreenSpaceAABB( const Matrix4f &mat, float &x, float &y, float &w, float &h ) const
{
	uint32 numPoints = 0;
	Vec3f points[8];
	Vec4f pts[8];

	float min_x = Math::MaxFloat, min_y = Math::MaxFloat;
	float max_x = -Math::MaxFloat, max_y = -Math::MaxFloat;
	
	if( _fov < 180 )
	{
		// Generate frustum for spot light
		numPoints = 5;
		float val = 1.0f * tanf( degToRad( _fov / 2 ) );
		points[0] = _absTrans * Vec3f( 0, 0, 0 );
		points[1] = _absTrans * Vec3f( -val * _radius2, -val * _radius2, -_radius2 );
		points[2] = _absTrans * Vec3f(  val * _radius2, -val * _radius2, -_radius2 );
		points[3] = _absTrans * Vec3f(  val * _radius2,  val * _radius2, -_radius2 );
		points[4] = _absTrans * Vec3f( -val * _radius2,  val * _radius2, -_radius2 );
	}
	else
	{
		// Generate sphere for point light
		numPoints = 8;
		points[0] = _absPos + Vec3f( -_radius2, -_radius2, -_radius2 );
		points[1] = _absPos + Vec3f(  _radius2, -_radius2, -_radius2 );
		points[2] = _absPos + Vec3f(  _radius2,  _radius2, -_radius2 );
		points[3] = _absPos + Vec3f( -_radius2,  _radius2, -_radius2 );
		points[4] = _absPos + Vec3f( -_radius2, -_radius2,  _radius2 );
		points[5] = _absPos + Vec3f(  _radius2, -_radius2,  _radius2 );
		points[6] = _absPos + Vec3f(  _radius2,  _radius2,  _radius2 );
		points[7] = _absPos + Vec3f( -_radius2,  _radius2,  _radius2 );
	}

	// Project points to screen-space and find extents
	for( uint32 i = 0; i < numPoints; ++i )
	{
		pts[i] = mat * Vec4f( points[i].x, points[i].y, points[i].z, 1 );
		
		if( pts[i].w != 0 )
		{
			pts[i].x = (pts[i].x / pts[i].w) * 0.5f + 0.5f;
			pts[i].y = (pts[i].y / pts[i].w) * 0.5f + 0.5f;
		}

		if( pts[i].x < min_x ) min_x = pts[i].x;
		if( pts[i].y < min_y ) min_y = pts[i].y;
		if( pts[i].x > max_x ) max_x = pts[i].x;
		if( pts[i].y > max_y ) max_y = pts[i].y;
	}
	
	// Clamp values
	if( min_x < 0 ) min_x = 0; if( min_x > 1 ) min_x = 1;
	if( max_x < 0 ) max_x = 0; if( max_x > 1 ) max_x = 1;
	if( min_y < 0 ) min_y = 0; if( min_y > 1 ) min_y = 1;
	if( max_y < 0 ) max_y = 0; if( max_y > 1 ) max_y = 1;
	
	x = min_x; y = min_y;
	w = max_x - min_x; h = max_y - min_y;

	// Check if viewer is inside bounding box
	if( pts[0].w < 0 || pts[1].w < 0 || pts[2].w < 0 || pts[3].w < 0 || pts[4].w < 0 )
	{
		x = 0; y = 0; w = 1; h = 1;
	}
	else if( numPoints == 8 && (pts[5].w < 0 || pts[6].w < 0 || pts[7].w < 0) )
	{
		x = 0; y = 0; w = 1; h = 1;
	}
}


void LightNode::onPostUpdate()
{
	// Calculate view matrix
	_viewMat = _absTrans.inverted();
	
	// Get position and spot direction
	Matrix4f m = _absTrans;
	m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0;
	_spotDir = m * Vec3f( 0, 0, -1 );
	_spotDir.normalize();

   Vec3f newPos = Vec3f( _absTrans.c[3][0], _absTrans.c[3][1], _absTrans.c[3][2] );
   bool lightMoved = newPos != _absPos;
	_absPos = newPos;

	// Generate frustum
	if( _fov < 180 ) {
		_frustum.buildViewFrustum( _absTrans, _fov, 1.0f, 0.1f, _radius2 );
   } else {
		_frustum.buildBoxFrustum( _absTrans, -_radius2, _radius2, -_radius2, _radius2, _radius2, -_radius2 );

      if (lightMoved) {
		   float ymax = 0.1f * tanf(degToRad(45.0f));
		   float xmax = ymax * 1.0f;  // ymax * aspect
         Matrix4f projMat = Matrix4f::PerspectiveMat(-xmax, xmax, -ymax, ymax, 0.1f, _radius2);

         static const float xRot[6] = { 0, 0, degToRad(-90), degToRad(90), 0, 0};
         static const float yRot[6] = { degToRad(90), degToRad(-90), degToRad(180), 0, degToRad(180), 0};
         Matrix4f lightView;
         for (int i = 0; i < 6; i++) {
            lightView.toIdentity();
            lightView.rotate(xRot[i], yRot[i], 0);
            lightView.translate(_absPos.x, _absPos.y, _absPos.z);
            _cubeViewMats[i] = lightView.inverted();
            _cubeFrustums[i].buildViewFrustum(_cubeViewMats[i], projMat);
            _dirtyShadows[i] = true;
         }
      }
   }
   if (!_directional) {
      _bBox.clear();
      Vec3f mins, maxs;
      _frustum.calcAABB(mins, maxs);
      _bBox.addPoint(mins);
      _bBox.addPoint(maxs);
   }
}

}  // namespace
