#ifndef _RADIANT_MATH3D_BOUNDS3F_H
#define _RADIANT_MATH3D_BOUNDS3F_H

#include "point3.h"
#include "ray3.h"
#include "color.h"

namespace radiant {
   namespace math3d {
      // axis-aligned bounding box

      class Bounds3f {
      public:
         point3         _min;
         point3         _max;

         Bounds3f() { }
         Bounds3f(const point3 &a, const point3 &b) : _min(a), _max(a) {
            add_point(b);
         }

         void Clean();
         
         bool operator==(const Bounds3f &other) const {
            return _min == other._min && _max == other._max;
         }

         Bounds3f operator+(const point3 &pt) const {
            return Bounds3f(_min + pt, _max + pt);
         }

         Bounds3f &operator-=(const point3 &pt) {
            _min -= pt;
            _max -= pt;
            return *this;
         }

         Bounds3f operator-(const point3 &pt) const {
            Bounds3f result(*this);
            result -= pt;
            return result;
         }

         Bounds3f &operator+=(const point3 &pt) {
            _min += pt;
            _max += pt;
            return *this;
         }

         float area() const {
            return ::fabs((_max.x - _min.x) * (_max.y - _min.y) * (_max.z - _min.z));
         }

         void add_point(const point3& pt) {
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

         void for_each_point(std::function<void (const point3 &)> fn) const {
            point3 pt;
            for (pt.y = _min.y; pt.y < _max.y; pt.y++) {
               for (pt.x = _min.x; pt.x < _max.x; pt.x++) {
                  for (pt.z = _min.z; pt.z < _max.z; pt.z++) {
                     fn(pt);
                  }
               }
            }
         }

         bool filter_each_point(std::function<bool (const point3 &)> fn) const {
            point3 pt;
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

         bool contains(const math3d::point3 &pt) const
         {
            return pt.x >= _min.x && pt.x < _max.x &&
                   pt.y >= _min.y && pt.y < _max.y &&
                   pt.z >= _min.z && pt.z < _max.z;
            return true;
         }

         void merge(const point3 &other) {
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

         Bounds3f merge(const Bounds3f &right) {
            point3 new_min(_min);
            point3 new_max(_max);

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
            return Bounds3f(new_min, new_max);
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

         friend bool intersects(Bounds3f &result, const Bounds3f& other);
      };

      std::ostream& operator<<(std::ostream& out, const Bounds3f& source);

      inline bool intersects(Bounds3f &result, const Bounds3f &a, const Bounds3f &b) {
         if (a._min.x > b._max.x || b._min.x > a._max.x ) {
            return false;
         }
         if (a._min.y > b._max.y || b._min.y > a._max.y ) {
            return false;
         }
         if (a._min.z > b._max.z || b._min.z > a._max.z ) {
            return false;
         }
         result._min.x = max(a._min.x, b._min.x);
         result._min.y = max(a._min.y, b._min.y);
         result._min.z = max(a._min.z, b._min.z);
         result._max.x = min(a._max.x, b._max.x);
         result._max.y = min(a._max.y, b._max.y);
         result._max.z = min(a._max.z, b._max.z);
         return true;
      }
   };
};

#endif // _RADIANT_MATH3D_BOUNDS3F_H
