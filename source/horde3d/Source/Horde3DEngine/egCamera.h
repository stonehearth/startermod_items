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

#ifndef _egCamera_H_
#define _egCamera_H_

#include "egPrerequisites.h"
#include "egScene.h"
#include "egTexture.h"
#include <cstring>


namespace Horde3D {

// =================================================================================================
// Camera Node
// =================================================================================================

struct CameraNodeParams
{
	enum List
	{
		PipeResI = 600,
		OutTexResI,
		OutBufIndexI,
		LeftPlaneF,
		RightPlaneF,
		BottomPlaneF,
		TopPlaneF,
		NearPlaneF,
		FarPlaneF,
		ViewportXI,
		ViewportYI,
		ViewportWidthI,
		ViewportHeightI,
		OrthoI,
		OccCullingI,
      FOVf
	};
};

// =================================================================================================

struct CameraNodeTpl : public SceneNodeTpl
{
	PPipelineResource   pipeRes;
	PTextureResource    outputTex;
	float               leftPlane, rightPlane;
	float               bottomPlane, topPlane;
	float               nearPlane, farPlane;
	int                 outputBufferIndex;
	bool                orthographic;
	bool                occlusionCulling;

	CameraNodeTpl( std::string const& name) :
		SceneNodeTpl( SceneNodeTypes::Camera, name, nullptr),
		outputTex( 0x0 ),
		// Default params: fov=45, aspect=4/3
		leftPlane( -0.055228457f ), rightPlane( 0.055228457f ), bottomPlane( -0.041421354f ),
		topPlane( 0.041421354f ), nearPlane( 0.1f ), farPlane( 1000.0f ), outputBufferIndex( 0 ),
		orthographic( false ), occlusionCulling( false )
	{
	}
};

// =================================================================================================

class CameraNode : public SceneNode
{
public:
	static SceneNodeTpl *parsingFunc( std::map< std::string, std::string > &attribs );
	static SceneNode *factoryFunc( const SceneNodeTpl &nodeTpl );

	~CameraNode();
	
	int getParamI( int param ) const;
	void setParamI( int param, int value );
	float getParamF( int param, int compIdx );
	void setParamF( int param, int compIdx, float value );

	void setupViewParams( float fov, float aspect, float nearPlane, float farPlane );

	const Frustum &getFrustum() const { return _frustum; }
	const Matrix4f &getViewMat() const { return _viewMat; }
	const Matrix4f &getProjMat() const { return _projMat; }
	const Vec3f &getAbsPos() const { return _absPos; }

   const Vec4f toScreenPos(const Vec3f& worldPos) const;
   const Vec3f toWorldPos(const Vec4f& screenPos) const;

private:
	CameraNode( const CameraNodeTpl &cameraTpl );
	void onPostUpdate();

private:
	PTextureResource    _outputTex;
	Matrix4f            _viewMat, _projMat;
	Frustum             _frustum;
	Vec3f               _absPos;
	int					_vpX, _vpY, _vpWidth, _vpHeight;
	float               _frustLeft, _frustRight, _frustBottom, _frustTop;
	float               _frustNear, _frustFar;
	int                 _outputBufferIndex;
	int                 _occSet;
	bool                _orthographic;  // Perspective or orthographic frustum?

	friend class Scene;
	friend class Renderer;
};

}
#endif // _egCamera_H_
