#ifndef _RADIANT_MATH3D_AABB_H
#define _RADIANT_MATH3D_AABB_H

#include "point3.h"
#include "ipoint3.h"
#include "ray3.h"
#include "color.h"
#include "rect2d.h"
#include <functional>
#include <algorithm>
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      class aabb;

      // axis-aligned bounding box
      inline bool intersects(ibounds3 &result, const ibounds3 &a, const ibounds3 &b);

      class ibounds3 {
      public:
         ipoint3         _min;
         ipoint3         _max;

      public:

         ibounds3() { }
         explicit ibounds3(aabb& a);

         ibounds3(const ipoint3 &a, const ipoint3 &b) : _min(a), _max(a) {
            add_point(b);
         }

         ibounds3(const math2d::Rect2d &r, int extraDimension = 0, int depth = 1) {
            _min = ipoint3(r.GetP0(), extraDimension);
            _max = ipoint3(r.GetP1(), extraDimension + depth);
         }

         void Clean();

         bool operator==(const ibounds3 &other) const {
            return _min == other._min && _max == other._max;
         }

         bool operator!=(const ibounds3 &other) const {
            return _min != other._min || _max != other._max;
         }

         ibounds3 operator+(const ipoint3 &pt) const {
            ASSERT(&pt != &_min && &pt != &_max); // xxx: _min and _max should not be publicly accessible!
            return ibounds3(_min + pt, _max + pt);
         }

         ibounds3 &operator-=(const ipoint3 pt) {
            ASSERT(&pt != &_min && &pt != &_max); // xxx: _min and _max should not be publicly accessible!
            _min -= pt;
            _max -= pt;
            return *this;
         }

         ibounds3 operator-(const ipoint3 &pt) const {
            ASSERT(&pt != &_min && &pt != &_max); // xxx: _min and _max should not be publicly accessible!
            ibounds3 result(*this);
            result -= pt;
            return result;
         }

         ibounds3 &operator+=(const ipoint3 &pt) {
            ASSERT(&pt != &_min && &pt != &_max); // xxx: _min and _max should not be publicly accessible!
            _min += pt;
            _max += pt;
            return *this;
         }

         int area() const {
            return ::abs((_max.x - _min.x) * (_max.y - _min.y) * (_max.z - _min.z));
         }

         void add_point(const ipoint3& pt) {
            ASSERT(&pt != &_min && &pt != &_max); // xxx: _min and _max should not be publicly accessible!

            if (pt.x < _min.x)
                 _min.x = pt.x;
             else if (pt.x > _max.x)
                 _max.x = pt.x;
             if (pt.y < _min.y)
                 _min.y = pt.y;
             else if (pt.y > _max.y)
                 _max.y = pt.y;
             if (pt.z < _min.z)
                 _min.z = pt.z;
             else if (pt.z > _max.z)
                 _max.z = pt.z;
         }

         point3 centroid() const {
            return point3((_max.x + _min.x) / 2.0f, (_max.y + _min.y) / 2.0f, (_max.z + _min.z) / 2.0f);
         }

         void set_zero() {
            _min.set_zero();
            _max.set_zero();
         }

         bool is_zero() const {
            return _min.is_zero() && _max.is_zero();
         }

         void for_each_point(std::function<void (const ipoint3 &)> fn) const {
            ipoint3 pt;
            for (pt.y = _min.y; pt.y < _max.y; pt.y++) {
               for (pt.x = _min.x; pt.x < _max.x; pt.x++) {
                  for (pt.z = _min.z; pt.z < _max.z; pt.z++) {
                     fn(pt);
                  }
               }
            }
         }

         bool filter_each_point(std::function<bool (const ipoint3 &)> fn) const {
            ipoint3 pt;
            for (pt.y = _min.y; pt.y < _max.y; pt.y++) {
               for (pt.x = _min.x; pt.x < _max.x; pt.x++) {
                  for (pt.z = _min.z; pt.z < _max.z; pt.z++) {
                     if (!fn(pt)) {
                        return false;
                     }
                  }
               }
            }
            return true;
         }

         ibounds3(const protocol::ibounds3 &b) {
            LoadValue(b);
         }

         void SaveValue(protocol::ibounds3 *msg) const {
            _min.SaveValue(msg->mutable_minimum());
            _max.SaveValue(msg->mutable_maximum());
         }

         void LoadValue(const protocol::ibounds3 &msg) {
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

         bool contains(const math3d::ipoint3 &pt) const
         {
            return pt.x >= _min.x && pt.x < _max.x &&
                   pt.y >= _min.y && pt.y < _max.y &&
                   pt.z >= _min.z && pt.z < _max.z;
            return true;
         }

         void merge(const ipoint3 &other) {
            ASSERT(&other != &_min && &other != &_max); // xxx: _min and _max should not be publicly accessible!

            if (other.x < _min.x) {
               _min.x = other.x;
            }
            if (other.y < _min.y) {
               _min.y = other.y;
            }
            if (other.z < _min.z) {
               _min.z = other.z;
            }

            if (other.x > _max.x) {
               _max.x = other.x;
            }
            if (other.y > _max.y) {
               _max.y = other.y;
            }
            if (other.z > _max.z) {
               _max.z = other.z;
            }
         }

         ibounds3 merge(const ibounds3 &right) {
            ipoint3 new_min(_min);
            ipoint3 new_max(_max);

            if (right._min.x < new_min.x) {
               new_min.x = right._min.x;
            }
            if (right._min.y < new_min.y) {
               new_min.y = right._min.y;
            }
            if (right._min.z < new_min.z) {
               new_min.z = right._min.z;
            }

            if (right._max.x > new_max.x) {
               new_max.x = right._max.x;
            }
            if (right._max.y > new_max.y) {
               new_max.y = right._max.y;
            }
            if (right._max.z > new_max.z) {
               new_max.z = right._max.z;
            }
            return ibounds3(new_min, new_max);
         } 

         bool Intersects(const ibounds3 &other) const {
            ibounds3 ignore;
            return intersects(ignore, *this, other);
         }

         bool Intersects(float &d, const ray3 &ray) const {
            float max_s = -FLT_MAX;
            float min_t = FLT_MAX;

            // do tests against three sets of planes
            for (int i = 0; i < 3; ++i) {
               // ray is parallel to plane
               if (math3d::is_zero(ray.direction[i])) {
                  // ray passes by box
                  if (ray.origin[i] < _min[i] || ray.origin[i] > _max[i]) {
                     return false;
                  }
               } else {
                  // compute intersection parameters and sort
                  float s = (_min[i] - ray.origin[i]) / ray.direction[i];
                  float t = (_max[i] - ray.origin[i]) / ray.direction[i];
                  if (s > t) {
                     float temp = s;
                     s = t;
                     t = temp;
                  }

                  // adjust min and max values
                  if (s > max_s) {
                     max_s = s;
                  }
                  if (t < min_t) {
                     min_t = t;
                  }
                  // check for intersection failure
                  if (min_t < 0.0f || max_s > min_t) {
                     return false;
                  }
               }
            }
            d = max_s;
            //hit = ray.origin + (max_s * ray.direction);

            // done, have intersection
            return true;
         }

      };

      std::ostream& operator<<(std::ostream& out, const ibounds3& source);

      inline bool intersects(ibounds3 &result, const ibounds3 &a, const ibounds3 &b) {
         if (a._min.x >= b._max.x || b._min.x >= a._max.x ) {
            return false;
         }
         if (a._min.y >= b._max.y || b._min.y >= a._max.y ) {
            return false;
         }
         if (a._min.z >= b._max.z || b._min.z >= a._max.z ) {
            return false;
         }
         result._min.x = std::max(a._min.x, b._min.x);
         result._min.y = std::max(a._min.y, b._min.y);
         result._min.z = std::max(a._min.z, b._min.z);
         result._max.x = std::min(a._max.x, b._max.x);
         result._max.y = std::min(a._max.y, b._max.y);
         result._max.z = std::min(a._max.z, b._max.z);
         ASSERT(result.area() > 0);
         return true;
      }
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::ibounds3, Protocol::ibounds3)

#endif // _RADIANT_MATH3D_AABB_H
