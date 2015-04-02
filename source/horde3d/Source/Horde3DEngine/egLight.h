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

#ifndef _egLight_H_
#define _egLight_H_

#include "egPrerequisites.h"
#include "egMaterial.h"
#include "egScene.h"


namespace Horde3D {

// =================================================================================================
// Light Node
// =================================================================================================

struct LightNodeParams
{
	enum List
	{
		MatResI = 500,
		Radius1F,
		Radius2F,
		FovF,
		ColorF3,
		ColorMultiplierF,
      AmbientColorF3,
		ShadowMapCountI,
		ShadowSplitLambdaF,
		ShadowMapBiasF,
		LightingContextStr,
		ShadowContextStr,
      DirectionalI,
      ImportanceI,
      ShadowMapQualityI,
      DirtyShadowsI
	};
};

struct LightNodeImportance
{
   enum List
   {
      Required = 0x0,
      High     = 0x1,
      Low      = 0x2
   };
};

struct LightCubeFace
{
   enum List
   {
      POSITIVE_X = 0,
      NEGATIVE_X = 1,
      POSITIVE_Y = 2,
      NEGATIVE_Y = 3,
      POSITIVE_Z = 4,
      NEGATIVE_Z = 5
   };
};


// =================================================================================================

struct LightNodeTpl : public SceneNodeTpl
{
	std::string        lightingContext, shadowContext;
	float              radius1, radius2, fov;
	float              col_R, col_G, col_B, colMult;
	float              ambCol_R, ambCol_G, ambCol_B;
	uint32             shadowMapCount;
	float              shadowSplitLambda;
	float              shadowMapBias;
   bool               directional;
   int                importance;
   uint32             shadowMapQuality;

	LightNodeTpl( std::string const& name,
	              std::string const& lightingContext, std::string const& shadowContext ) :
		SceneNodeTpl( SceneNodeTypes::Light, name ), 
		lightingContext( lightingContext ), shadowContext( shadowContext ),
		radius1(0), radius2(100), fov( 90 ), col_R( 1 ), col_G( 1 ), col_B( 1 ), colMult( 1 ),
      ambCol_R( 0 ), ambCol_G( 0 ), ambCol_B( 0 ), shadowMapCount( 0 ), shadowSplitLambda( 0.5f ), 
      shadowMapBias( 0.005f ), directional(false), importance(0), shadowMapQuality(0)
	{
	}
};

// =================================================================================================

class LightNode : public SceneNode
{
public:
   static SceneNodeTpl *parsingFunc( std::map<std::string, std::string > &attribs );
   static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );
	
   int getParamI( int param ) const ;
   void setParamI( int param, int value );
   float getParamF( int param, int compIdx );
   void setParamF( int param, int compIdx, float value );
   const char *getParamStr( int param );
   void setParamStr( int param, const char *value );

   void calcScreenSpaceAABB( const Matrix4f &mat, float &x, float &y, float &w, float &h ) const;

   Frustum const& getFrustum() const { return _frustum; }
   Matrix4f const& getViewMat() const { return _viewMat; }
   
   Frustum const& getCubeFrustum(LightCubeFace::List faceNum) const { return _cubeFrustums[faceNum]; }
   Matrix4f const& getCubeViewMat(LightCubeFace::List faceNum) const { return _cubeViewMats[faceNum]; }
   
   void reallocateShadowBuffer(int size);

protected:
   uint32 getShadowBuffer() const { return _shadowMapBuffer; }

private:
   LightNode( const LightNodeTpl &lightTpl );
   ~LightNode();

   void registerForNodeTracking();
   void onPostUpdate();
   void dirtyCubeFrustums();

private:
   // Frustum for the entire light.  For a directional light, this frustum is re-calculated every
   // rendering occurs, as it is a function of the camera's frustum.
   Frustum                _frustum;
   // This is the view matrix of the entire light.
   Matrix4f               _viewMat;

   Frustum                _cubeFrustums[6];
   Matrix4f               _cubeViewMats[6];

   Vec3f                  _absPos, _spotDir;

   std::string            _lightingContext, _shadowContext;
   float                  _radius1, _radius2, _fov;
   bool                   _directional;
   Vec3f                  _diffuseCol;
   Vec3f                  _ambientCol;
   float                  _diffuseColMult;
   uint32                 _shadowMapCount;
   uint32                 _shadowMapBuffer;
   int                    _importance;
   float                  _shadowSplitLambda, _shadowMapBias;
   uint32                 _shadowMapQuality, _shadowMapSize;
   bool                   _dirtyShadows[6];

   friend class Scene;
   friend class Renderer;
};

}
#endif // _egLight_H_
