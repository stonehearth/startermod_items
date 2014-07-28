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

#ifndef _egPrimitives_H_
#define _egPrimitives_H_

#include "egPrerequisites.h"
#include "utMath.h"

#ifdef _DEBUG
#define ASSERT_VALID_BOX \
   if ((_min.x == FLT_MAX || _min.y == FLT_MAX || _min.z == FLT_MAX) || \
       (_max.x == -FLT_MAX || _max.y == -FLT_MAX || _max.z == -FLT_MAX)) { \
         ASSERT(false); \
   }
#else
#define ASSERT_VALID_BOX
#endif


namespace Horde3D {

// =================================================================================================
// Bounding Box
// =================================================================================================

struct BoundingBox
{	
   BoundingBox()
   {
      clear();
   }

   void addPoint(const Vec3f& p)
   {
      if (_min.x > p.x)
      {
         _min.x = p.x;
      }
      if (_min.y > p.y)
      {
         _min.y = p.y;
      }
      if (_min.z > p.z)
      {
         _min.z = p.z;
      }

      if (_max.x < p.x)
      {
         _max.x = p.x;
      }
      if (_max.y < p.y)
      {
         _max.y = p.y;
      }
      if (_max.z < p.z)
      {
         _max.z = p.z;
      }

      updateExtentAndCenter();
      ASSERT_VALID_BOX
   }
	
	void clear()
	{
      _min = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
      _max = Vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}

	Vec3f getCorner( uint32 index ) const
	{
		switch( index )
		{
		case 0:
			return Vec3f( _min.x, _min.y, _max.z );
		case 1:
			return Vec3f( _max.x, _min.y, _max.z );
		case 2:
			return Vec3f( _max.x, _max.y, _max.z );
		case 3:
			return Vec3f( _min.x, _max.y, _max.z );
		case 4:
			return Vec3f( _min.x, _min.y, _min.z );
		case 5:
			return Vec3f( _max.x, _min.y, _min.z );
		case 6:
			return Vec3f( _max.x, _max.y, _min.z );
		case 7:
			return Vec3f( _min.x, _max.y, _min.z );
		default:
			return Vec3f();
		}
	}


	void transform( const Matrix4f &m )
	{
		// Efficient algorithm for transforming an AABB, taken from Graphics Gems

      // Don't transform invalid bounding boxes.
      if (_min.x > _max.x) {
         return;
      }
		
		float minA[3] = { _min.x, _min.y, _min.z }, minB[3];
		float maxA[3] = { _max.x, _max.y, _max.z }, maxB[3];

		for( uint32 i = 0; i < 3; ++i )
		{
			minB[i] = m.c[3][i];
			maxB[i] = m.c[3][i];
			
			for( uint32 j = 0; j < 3; ++j )
			{
				float x = minA[j] * m.c[j][i];
				float y = maxA[j] * m.c[j][i];
				minB[i] += minf( x, y );
				maxB[i] += maxf( x, y );
			}
		}

		_min = Vec3f( minB[0], minB[1], minB[2] );
		_max = Vec3f( maxB[0], maxB[1], maxB[2] );

      updateExtentAndCenter();
	}


	bool makeUnion( const BoundingBox &b )
	{
		bool changed = false;

		// Trivially accept invalid boxes.
		if( _min.x > _max.x )
		{
			changed = true;
			_min = b._min;
			_max = b._max;
		}
		else if( b._min != b._max )
		{
			if( b._min.x < _min.x ) { changed = true; _min.x = b._min.x; }
			if( b._min.y < _min.y ) { changed = true; _min.y = b._min.y; }
			if( b._min.z < _min.z ) { changed = true; _min.z = b._min.z; }

			if( b._max.x > _max.x ) { changed = true; _max.x = b._max.x; }
			if( b._max.y > _max.y ) { changed = true; _max.y = b._max.y; }
			if( b._max.z > _max.z ) { changed = true; _max.z = b._max.z; }
		}

      updateExtentAndCenter();
      ASSERT_VALID_BOX
		return changed;
	}

   void feather()
   {
      ASSERT_VALID_BOX
      if (_min.x == _max.x)
      {
         _max.x += Math::Epsilon;
      }
      if (_min.y == _max.y)
      {
         _max.y += Math::Epsilon;
      }
      if (_min.z == _max.z)
      {
         _max.z += Math::Epsilon;
      }
      updateExtentAndCenter();
   }

   inline const Vec3f& min() const {
      return _min;
   }

   inline const Vec3f& max() const {
      return _max;
   }

   bool contains(const Vec3f& v) const {
      ASSERT_VALID_BOX

      return _max.x >= v.x && _max.y >= v.y && _max.z >= v.z &&
         _min.x <= v.x && _min.y <= v.y && _min.z <= v.z;
   }

   void growMax(const Vec3f& v)
   {
      ASSERT_VALID_BOX

      _max += v;
      updateExtentAndCenter();

      ASSERT_VALID_BOX
   }

   void growMin(const Vec3f& v)
   {
      ASSERT_VALID_BOX

      _max += v;
      updateExtentAndCenter();

      ASSERT_VALID_BOX
   }

   bool isValid() const
   {
      return !((_min.x == FLT_MAX || _min.y == FLT_MAX || _min.z == FLT_MAX) ||
          (_max.x == -FLT_MAX || _max.y == -FLT_MAX || _max.z == -FLT_MAX));
   }

   float intersectionOf(const Vec3f& rayStart, const Vec3f& rayDir) const;

   inline void updateExtentAndCenter() {
      _center = (_min + _max) * 0.5f;
      _extent = (_min - _max) * 0.5f;
   }

   inline const Vec3f& extent() const {
      return _extent;
   }

   inline const Vec3f& center() const {
      return _center;
   }

private:
   Vec3f  _min, _max;
   Vec3f _center, _extent;
};


// =================================================================================================
// Frustum
// =================================================================================================

class Frustum
{
public:
	const Vec3f &getOrigin() const { return _origin; }
	const Vec3f &getCorner( uint32 index ) const { return _corners[index]; }
	
	void buildViewFrustum( const Matrix4f &transMat, float fov, float aspect, float nearPlane, float farPlane );
	void buildViewFrustum( const Matrix4f &transMat, float left, float right,
	                       float bottom, float top, float nearPlane, float farPlane );
	void buildViewFrustum( const Matrix4f &viewMat, const Matrix4f &projMat );
	void buildBoxFrustum( const Matrix4f &transMat, float left, float right,
	                      float bottom, float top, float front, float back );
	bool cullSphere( Vec3f pos, float rad ) const;
	bool cullBox( const BoundingBox &b ) const;
	bool cullFrustum( const Frustum &frust ) const;

	void calcAABB( Vec3f &mins, Vec3f &maxs ) const;

   void clipAABB(const BoundingBox& b, std::vector<Polygon>* results) const;

   bool operator==(const Frustum& other) const;
   bool operator!=(const Frustum& other) const;

private:
	Plane  _planes[6];  // Planes of frustum
	Vec3f  _origin;
	Vec3f  _corners[8];  // Corner points
};

}
#endif // _egPrimitives_H_
