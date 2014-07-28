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

#include "egPrimitives.h"

#include "utDebug.h"

#include <xmmintrin.h>

namespace Horde3D {

// *************************************************************************************************
// Frustum
// *************************************************************************************************

void Frustum::buildViewFrustum( const Matrix4f &transMat, float fov, float aspect, float nearPlane, float farPlane )
{
	float ymax = nearPlane * tanf( degToRad( fov / 2 ) );
	float xmax = ymax * aspect;
	
	buildViewFrustum( transMat, -xmax, xmax, -ymax, ymax, nearPlane, farPlane );
}


void Frustum::buildViewFrustum( const Matrix4f &transMat, float left, float right,
							    float bottom, float top, float nearPlane, float farPlane ) 
{
	// Use intercept theorem to get params for far plane
	float left_f = left * farPlane / nearPlane;
	float right_f = right * farPlane / nearPlane;
	float bottom_f = bottom * farPlane / nearPlane;
	float top_f = top * farPlane / nearPlane;

	// Get points on near plane
	_corners[0] = Vec3f( left, bottom, -nearPlane );
	_corners[1] = Vec3f( right, bottom, -nearPlane );
	_corners[2] = Vec3f( right, top, -nearPlane );
	_corners[3] = Vec3f( left, top, -nearPlane );

	// Get points on far plane
	_corners[4] = Vec3f( left_f, bottom_f, -farPlane );
	_corners[5] = Vec3f( right_f, bottom_f, -farPlane );
	_corners[6] = Vec3f( right_f, top_f, -farPlane );
	_corners[7] = Vec3f( left_f, top_f, -farPlane );

	// Transform points to fit camera position and rotation
	_origin = transMat * Vec3f( 0, 0, 0 );
	for( uint32 i = 0; i < 8; ++i )
		_corners[i] = transMat * _corners[i];

	// Build planes
	_planes[0] = Plane( _origin, _corners[3], _corners[0] );		// Left
	_planes[1] = Plane( _origin, _corners[1], _corners[2] );		// Right
	_planes[2] = Plane( _origin, _corners[0], _corners[1] );		// Bottom
	_planes[3] = Plane( _origin, _corners[2], _corners[3] );		// Top
	_planes[4] = Plane( _corners[0], _corners[1], _corners[2] );	// Near
	_planes[5] = Plane( _corners[5], _corners[4], _corners[7] );	// Far
}


void Frustum::buildViewFrustum( const Matrix4f &viewMat, const Matrix4f &projMat )
{
	// This routine works with the OpenGL projection matrix
	// The view matrix is the inverse camera transformation matrix
	// Note: Frustum corners are not updated!
	
	Matrix4f m = projMat * viewMat;
	
	_planes[0] = Plane( -(m.c[0][3] + m.c[0][0]), -(m.c[1][3] + m.c[1][0]),
						-(m.c[2][3] + m.c[2][0]), -(m.c[3][3] + m.c[3][0]) );	// Left
	_planes[1] = Plane( -(m.c[0][3] - m.c[0][0]), -(m.c[1][3] - m.c[1][0]),
						-(m.c[2][3] - m.c[2][0]), -(m.c[3][3] - m.c[3][0]) );	// Right
	_planes[2] = Plane( -(m.c[0][3] + m.c[0][1]), -(m.c[1][3] + m.c[1][1]),
						-(m.c[2][3] + m.c[2][1]), -(m.c[3][3] + m.c[3][1]) );	// Bottom
	_planes[3] = Plane( -(m.c[0][3] - m.c[0][1]), -(m.c[1][3] - m.c[1][1]),
						-(m.c[2][3] - m.c[2][1]), -(m.c[3][3] - m.c[3][1]) );	// Top
	_planes[4] = Plane( -(m.c[0][3] + m.c[0][2]), -(m.c[1][3] + m.c[1][2]),
						-(m.c[2][3] + m.c[2][2]), -(m.c[3][3] + m.c[3][2]) );	// Near
	_planes[5] = Plane( -(m.c[0][3] - m.c[0][2]), -(m.c[1][3] - m.c[1][2]),
						-(m.c[2][3] - m.c[2][2]), -(m.c[3][3] - m.c[3][2]) );	// Far

	_origin = viewMat.inverted() * Vec3f( 0, 0, 0 );

	// Calculate corners
	Matrix4f mm = m.inverted();
	Vec4f corner = mm * Vec4f( -1, -1,  -1, 1 );
	_corners[0] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( 1, -1,  -1, 1 );
	_corners[1] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( 1,  1,  -1, 1 );
	_corners[2] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( -1,  1,  -1, 1 );
	_corners[3] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( -1, -1, 1, 1 );
	_corners[4] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( 1, -1, 1, 1 );
	_corners[5] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( 1, 1, 1, 1 );
	_corners[6] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
	corner = mm * Vec4f( -1, 1, 1, 1 );
	_corners[7] = Vec3f( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
}


void Frustum::buildBoxFrustum( const Matrix4f &transMat, float left, float right,
							   float bottom, float top, float front, float back ) 
{
	// Get points on front plane
	_corners[0] = Vec3f( left, bottom, front );
	_corners[1] = Vec3f( right, bottom, front );
	_corners[2] = Vec3f( right, top, front );
	_corners[3] = Vec3f( left, top, front );

	// Get points on far plane
	_corners[4] = Vec3f( left, bottom, back );
	_corners[5] = Vec3f( right, bottom, back );
	_corners[6] = Vec3f( right, top, back );
	_corners[7] = Vec3f( left, top, back );

	// Transform points to fit camera position and rotation
	_origin = transMat * Vec3f( 0, 0, 0 );
	for( uint32 i = 0; i < 8; ++i )
		_corners[i] = transMat * _corners[i];

	// Build planes
	_planes[0] = Plane( _corners[0], _corners[3], _corners[7] );	// Left
	_planes[1] = Plane( _corners[2], _corners[1], _corners[6] );	// Right
	_planes[2] = Plane( _corners[0], _corners[4], _corners[5] );	// Bottom
	_planes[3] = Plane( _corners[3], _corners[2], _corners[6] );	// Top
	_planes[4] = Plane( _corners[0], _corners[1], _corners[2] );	// Front
	_planes[5] = Plane( _corners[4], _corners[7], _corners[6] );	// Back
}


bool Frustum::cullSphere( Vec3f pos, float rad ) const
{
	// Check the distance of the center to the planes
	for( uint32 i = 0; i < 6; ++i )
	{
		if( _planes[i].distToPoint( pos ) > rad ) return true;
	}

	return false;
}

// From http://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/ .  This only processes one
// box at a time; when we have better data locality, we'll be able to process 4 at a time.
// Also, might consider using Vec4s for the box extents/centers, since then we can load them in
// one shot.
bool Frustum::cullBox( const BoundingBox &b ) const
{
   Vec3f const& center = b.center();
   Vec3f const& extent = b.extent();
   __m128 extentm = _mm_set_ps(extent.x, extent.y, extent.z, 0.0f);
   __m128 centerm = _mm_set_ps(center.x, center.y, center.z, 0.0f);
   __m128 zeros = _mm_set_ps1(0.0f);
   int mask = 0x80000000;
   __m128 signflipMask = _mm_load_ps1((float*)&mask);
   float dot;
   for (uint32 i = 0; i < 6; i++)
   {
      const Plane& p = _planes[i];
      const Vec3f& pNorm = p.normal;
      const float& pw = p.dist;
      __m128 planes = _mm_set_ps(pNorm.x, pNorm.y, pNorm.z, 0.0f);
      planes = _mm_add_ps(planes, zeros);
      
      __m128 selected = _mm_and_ps(planes, signflipMask);
      selected = _mm_xor_ps(selected, extentm);
      selected = _mm_add_ps(selected, centerm);
      selected = _mm_mul_ps(planes, selected);
      selected = _mm_hadd_ps(selected, selected);
      selected = _mm_hadd_ps(selected, selected);
      _mm_store_ss(&dot, selected);
      if (dot > -pw) {
         return true;
      }
   }
   return false;
}


bool Frustum::cullFrustum( const Frustum &frust ) const
{
	for( uint32 i = 0; i < 6; ++i )
	{
		bool allOut = true;
		
		for( uint32 j = 0; j < 8; ++j )
		{
			if( _planes[i].distToPoint( frust._corners[j] ) < 0 )
			{
				allOut = false;
				break;
			}
		}

		if( allOut ) return true;
	}

	return false;
}


void Frustum::calcAABB( Vec3f &mins, Vec3f &maxs ) const
{
	mins.x = Math::MaxFloat; mins.y = Math::MaxFloat; mins.z = Math::MaxFloat;
	maxs.x = -Math::MaxFloat; maxs.y = -Math::MaxFloat; maxs.z = -Math::MaxFloat;
	
	for( uint32 i = 0; i < 8; ++i )
	{
		if( _corners[i].x < mins.x ) mins.x = _corners[i].x;
		if( _corners[i].y < mins.y ) mins.y = _corners[i].y;
		if( _corners[i].z < mins.z ) mins.z = _corners[i].z;
		if( _corners[i].x > maxs.x ) maxs.x = _corners[i].x;
		if( _corners[i].y > maxs.y ) maxs.y = _corners[i].y;
		if( _corners[i].z > maxs.z ) maxs.z = _corners[i].z;
	}
}


void Frustum::clipAABB(const BoundingBox& box, std::vector<Polygon>* results) const
{
   std::vector<Polygon> initialQuads;
   initialQuads.push_back(Polygon(box.getCorner(4), box.getCorner(7), box.getCorner(6), box.getCorner(5)));
   initialQuads.push_back(Polygon(box.getCorner(5), box.getCorner(6), box.getCorner(2), box.getCorner(1)));
   initialQuads.push_back(Polygon(box.getCorner(1), box.getCorner(2), box.getCorner(3), box.getCorner(0)));
   initialQuads.push_back(Polygon(box.getCorner(0), box.getCorner(3), box.getCorner(7), box.getCorner(4)));
   initialQuads.push_back(Polygon(box.getCorner(7), box.getCorner(3), box.getCorner(2), box.getCorner(6)));
   initialQuads.push_back(Polygon(box.getCorner(0), box.getCorner(4), box.getCorner(5), box.getCorner(1)));
   
   for (const auto& side : initialQuads)
   {
      Polygon temp = clipPolyToPlane(_planes[0], side);
      for (int i = 1; i < 6; i++)
      {
         temp = clipPolyToPlane(_planes[i], temp);
      }

      if (temp.points().size() > 0)
      {
         results->push_back(temp);
      }
   }
}


bool Frustum::operator==(const Frustum& other) const
{
   for (int i = 0; i < 6; i++)
   {
      if (other._planes[i] != _planes[i])
      {
         return false;
      }
   }

   return true;
}

bool Frustum::operator!=(const Frustum& other) const
{
   for (int i = 0; i < 6; i++)
   {
      if (other._planes[i] != _planes[i])
      {
         return true;
      }
   }

   return false;
}



// Nice bit of code from Real-Time Collision Detection.
float BoundingBox::intersectionOf(const Vec3f& rayStart, const Vec3f& rayDir) const {
   float tmin = 0.0f;
   float tmax = 999999.0f;
   for (int i = 0; i < 3; i++) {
      if (fabs(rayDir.get(i)) < Math::Epsilon) {
         // If the ray is parallel to the slab, and the origin isn't contained in the slab, then we don't hit it.
         if (rayStart.get(i) < _min.get(i) || rayStart.get(i) > _max.get(i)) {
            return -1;
         }
      } else {
         float ood = 1.0f / rayDir.get(i);
         float t1 = (_min.get(i) - rayStart.get(i)) * ood;
         float t2 = (_max.get(i) - rayStart.get(i)) * ood;

         if (t1 > t2) {
            std::swap(t1, t2);
         }

         if (t1 > tmin) {
            tmin = t1;
         }
         if (t2 < tmax) {
            tmax = t2;
         }
         if (tmin > tmax) {
            return -1;
         }
      }
   }
   return tmin;
}


}  // namespace
