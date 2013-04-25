//===============================================================================
// @ aabb.h
// 
// Axis-aligned bounding box class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COLLISION_AABB_H
#define _RADIANT_MATH3D_COLLISION_AABB_H

#include "math3d/common/aabb.h"
#include "math3d/common/quaternion.h"
#include "math3d/common/point3.h"
#include "store.pb.h"
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      class aabb
      {
      public:
         // constructor/destructor
         inline aabb() : _min(FLT_MAX, FLT_MAX, FLT_MAX), _max(FLT_MIN, FLT_MIN, FLT_MIN) { }
         inline aabb(const point3& mini, const point3& maxi) : _min(mini), _max(maxi) { }
         explicit inline aabb(const ibounds3 &bounds) : _min(bounds._min), _max(bounds._max) { }

         aabb(const math2d::Rect2d &r, int extraDimension = 0, int depth = 1) {
            _min = point3(ipoint3(r.GetP0(), extraDimension));
            _max = point3(ipoint3(r.GetP1(), extraDimension + depth));
         }

         inline ~aabb() {}

         // copy operations
         aabb(const aabb& other);
         aabb& operator=(const aabb& other);

         // accessors
         math3d::ibounds3 ToBounds() const;

         // comparison
         bool operator==(const aabb& other) const;
         bool operator!=(const aabb& other) const;

         // serialization
         void SaveValue(protocol::aabb *msg) const {
            _min.SaveValue(msg->mutable_minimum());
            _max.SaveValue(msg->mutable_maximum());
         }

         void LoadValue(const protocol::aabb &msg) {
            _min.LoadValue(msg.minimum());
            _max.LoadValue(msg.maximum());
         }

         void SaveValue(protocol::box *box) const { 
            _min.SaveValue(box->mutable_minimum());
            _max.SaveValue(box->mutable_maximum());
         }

         void SaveValue(protocol::box *box, const math3d::color4 &c) const { 
            SaveValue(box);
            c.SaveValue(box->mutable_color());
         }

         // manipulators
         point3 GetCentroid() const {
            return point3((_max.x + _min.x) / 2.0f, (_max.y + _min.y) / 2.0f, (_max.z + _min.z) / 2.0f);
         }

         void Clean();
         void set_zero() { set(math3d::point3::origin, math3d::point3::origin); }
         float area() const {
            return ::fabs((_max.x - _min.x) * (_max.y - _min.y) * (_max.z - _min.z));
         }
         void set(const point3* points, unsigned int num_points);
         inline void set(const point3& min, const point3& max) {
            _min = min;
            _max = max;
         }
         void add_point(const point3& point);
         aabb &operator+=(const vector3& distance) {
            _min += distance;
            _max += distance;
            return *this;
         }
         aabb &operator+=(const point3& distance) {
            _min += distance;
            _max += distance;
            return *this;
         }
         aabb operator+(const vector3& distance) const {
            aabb result;
            result._min = _min + distance;
            result._max = _max + distance;
            return result;
         }
         aabb operator+(const point3& distance) const {
            aabb result;
            result._min = _min + distance;
            result._max = _max + distance;
            return result;
         }
         aabb operator-(const point3& distance) const {
            aabb result;
            result._min = _min - distance;
            result._max = _max - distance;
            return result;
         }
         aabb rotate(const quaternion& q) const;

         // intersection
         bool Intersects(const aabb& other) const;
         bool Intersects(const line3& line) const;
         bool Intersects(float& t, const ray3& ray) const;
         bool Intersects(const line_segment3& segment) const;
         bool Intersects(const point3 &pt) const
         {
            return   pt.x >= _min.x && pt.x < _max.x &&
               pt.y >= _min.y && pt.y < _max.y &&
               pt.z >= _min.z && pt.z < _max.z;
         }
         bool contains(const math3d::point3 &pt) const
         {
            return Intersects(pt);
         }
         bool contains(const math3d::ipoint3 &pt) const
         {
            return Intersects(point3(pt));
         }

         // collision
         bool collide(const aabb &initial_position, const vector3 &velocity, vector3 &uv, float &u) const;
         bool find_closest_point(point3 &closest, const ray3& ray) const;

         // signed distance to plane
         float classify(const plane& plane) const;

         friend void merge(aabb& result, const aabb& b0, const aabb& b1);
         friend inline void inset(aabb &inset, const math3d::ibounds3 &bounds, const math3d::point3 &amount);
         friend inline bool intersects(aabb &result, const aabb &a, const aabb& b);

      public:
         point3       _min, _max;
      };
      std::ostream& operator<<(std::ostream& out, const aabb& source);
      void merge(aabb& result, const aabb& b0, const aabb& b1);

      inline void inset(aabb &result, const math3d::ibounds3 &bounds, const math3d::point3 &amount) {
         result._min.x = std::min(bounds._min.x + amount.x, (float)bounds._max.x);
         result._min.y = std::min(bounds._min.y + amount.y, (float)bounds._max.y);
         result._min.z = std::min(bounds._min.z + amount.z, (float)bounds._max.z);
         result._max.x = std::max(result._min.x, bounds._max.x - amount.x);
         result._max.y = std::max(result._min.y, bounds._max.y - amount.y);
         result._max.z = std::max(result._min.z, bounds._max.z - amount.z);
      }

      inline void inset(aabb &result, const math3d::ibounds3 &bounds, float margin) {
         inset(result, bounds, math3d::point3(margin, margin, margin));
      }

      inline bool intersects(aabb &result, const aabb &a, const aabb& b) {
         if (a._min.x > b._max.x || b._min.x > a._max.x ) {
            return false;
         }
         if (a._min.y > b._max.y || b._min.y > a._max.y ) {
            return false;
         }
         if (a._min.z > b._max.z || b._min.z > a._max.z ) {
            return false;
         }
         result._min.x = std::max(a._min.x, b._min.x);
         result._min.y = std::max(a._min.y, b._min.y);
         result._min.z = std::max(a._min.z, b._min.z);
         result._max.x = std::min(a._max.x, b._max.x);
         result._max.y = std::min(a._max.y, b._max.y);
         result._max.z = std::min(a._max.z, b._max.z);
         return true;
      }

   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::aabb, Protocol::aabb)

#endif
