#include "pch.h"
#include "csg.h"
#include "cube.h"
#include "ray.h"
#include "region.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Cube3 Cube3::zero(Point3(0, 0, 0), Point3(0, 0, 0));
Cube3 Cube3::one(Point3(0, 0, 0), Point3(1, 1, 1));

template <class C>
static bool Cube3IntersectsImpl(const C& cube, const csg::Ray3& ray, float& d)
{
   const auto& min = cube.GetMin();
   const auto& max = cube.GetMax();
   float max_s = -FLT_MAX;
   float min_t = FLT_MAX;

   // do tests against three sets of planes
   for (int i = 0; i < 3; ++i) {
      // ray is parallel to plane
      if (csg::IsZero(ray.direction[i])) {
         // ray passes by box
         if (ray.origin[i] < min[i] || ray.origin[i] > max[i]) {
            return false;
         }
      } else {
         // compute intersection parameters and sort
         float s = (min[i] - ray.origin[i]) / ray.direction[i];
         float t = (max[i] - ray.origin[i]) / ray.direction[i];
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

bool csg::Cube3Intersects(const Cube3& rgn, const Ray3& ray, float& distance)
{
   return Cube3IntersectsImpl(rgn, ray, distance);
}

bool csg::Cube3Intersects(const Cube3f& rgn, const Ray3& ray, float& distance)
{
   return Cube3IntersectsImpl(rgn, ray, distance);
}


template <typename S, int C>
Cube<S, C>::Cube() :
   tag_(0)
{
}

template <typename S, int C>
Cube<S, C>::Cube(const Point& min, int tag) :
   tag_(tag),
   min_(min)
{
   for (int i = 0; i < C; i++) {
      max_[i] = min_[i] + 1;
   }
}

template <typename S, int C>
Cube<S, C>::Cube(const Point& min, const Point& max, int tag) :
   tag_(tag),
   min_(min),
   max_(max)
{
   for (int i = 0; i < C; i++) {
      ASSERT(min_[i] <= max_[i]);
   }
}

template <typename S, int C>
S Cube<S, C>::GetArea() const
{
   S area = 1;
   for (int i = 0; i < C; i++) {
      area *= (max_[i] - min_[i]);
   }
   return area;
}

template <typename S, int C>
bool Cube<S, C>::Intersects(const Cube& other) const
{
   for (int i = 0; i < C; i++) {
      if (min_[i] >= other.max_[i] || other.min_[i] >= max_[i]) {
         return false;
      }
   }
   return true;
}

template <typename S, int C>
Region<S, C> Cube<S, C>::operator-(const Cube& rhs) const
{
   if (!Intersects(rhs)) {
      return Region(*this);
   }

   Region result;
   Cube lhs(*this), next(*this);

   // Trim plane by plane
   for (int i = 0; i < C; i++) {
      ASSERT(lhs.min_[i] < rhs.max_[i] && rhs.min_[i] < lhs.max_[i]);
      if (lhs.min_[i] < rhs.min_[i]) {
         Cube keep(lhs);
         keep.max_[i] = rhs.min_[i];
         next.min_[i] = rhs.min_[i];
         result.AddUnique(keep);
      }
      if (rhs.max_[i] < lhs.max_[i]) {
         Cube keep(lhs);
         keep.min_[i] = rhs.max_[i];
         next.max_[i] = rhs.max_[i];
         result.AddUnique(keep);
      }
      lhs = next;
   }
   return result;
}

template <typename S, int C>
Region<S, C> Cube<S, C>::operator-(const Region& rhs) const
{
   return Region(*this) - rhs;
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator&(const Cube& other) const
{
   Cube result;

   result.tag_ = tag_;
   for (int i = 0; i < C; i++) {
      result.min_[i] = std::max(min_[i], other.min_[i]);
      result.max_[i] = std::min(max_[i], other.max_[i]);
      if (result.max_[i] < result.min_[i]) {
         result.max_[i] = result.min_[i];
      }
   }
   return result;
}

template <typename S, int C>
Region<S, C> Cube<S, C>::operator&(const Region& region) const
{
   Region result;

   for (Cube const& c : region) {
      Cube clipped = *this & c;
      if (!clipped.IsEmpty()) {
         result.AddUnique(clipped);
      }
   }
   return result;
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator+(const Point& offset) const
{
   return Cube(min_ + offset, max_ + offset, tag_);
}

template <typename S, int C>
Point<S, C> Cube<S, C>::GetClosestPoint2(const Point& other, S* d) const
{
   Point result;
   for (int i = 0; i < C; i++) {
      result[i] = std::max(std::min(other[i], max_[i] - 1), min_[i]);
   }
   if (d) {
      *d = 0;
      for (int i = 0; i < C; i++) {
         *d += ((other[i] - result[i]) * (other[i] - result[i]));
      }
   }
   return result;
}

template <typename S, int C>
bool Cube<S, C>::Contains(const Point& pt) const
{
   for (int i = 0; i < C; i++) {
      if (pt[i] < min_[i] || pt[i] >= max_[i]) {
         return false;
      }
   }
   return true;
}

template <class S, int C>
void Cube<S, C>::Grow(const Point& pt)
{
   for (int i = 0; i < C; i++) {
      min_[i] = std::min(min_[i], pt[i]);
      max_[i] = std::max(max_[i], pt[i]);
   }
}

template <class S, int C>
Cube<S, C> Cube<S, C>::ProjectOnto(int axis, S plane) const
{
   return Cube(min_.ProjectOnto(axis, plane), max_.ProjectOnto(axis, plane + 1), tag_);
}

#if 0
template <class S, int C>
Cube<S, C> Cube<S, C>::Construct(Point min, Point max, int tag)
{
   for (int i = 0; i < C; i++) {
      if (min[i] > max[i]) {
         std::swap(min[i], max[i]);
      }
   }
   return Cube(min, max, tag);
}
#endif

#define MAKE_CUBE(Cls) \
   template Cls::Cube(); \
   template Cls::Cube(const Cls::Point&, int); \
   template Cls::Cube(const Cls::Point&, const Cls::Point&, int); \
   template Cls::ScalarType Cls::GetArea() const; \
   template bool Cls::Intersects(const Cls& other) const; \
   template Cls Cls::operator&(const Cls& offset) const; \
   template Cls::Region Cls::operator&(const Cls::Region& offset) const; \
   template Cls Cls::operator+(const Cls::Point& offset) const; \
   template Cls::Region Cls::operator-(const Cls& other) const; \
   template Cls::Region Cls::operator-(const Cls::Region& other) const; \
   template bool Cls::Contains(const Cls::Point& other) const; \
   template Cls::Point Cls::GetClosestPoint2(const Cls::Point& other, Cls::ScalarType*) const; \
   template void Cls::Grow(const Cls::Point& other); \
   template Cls Cls::ProjectOnto(int axis, Cls::ScalarType plane) const; \

MAKE_CUBE(Cube3)
MAKE_CUBE(Cube3f)
MAKE_CUBE(Rect2)
MAKE_CUBE(Line1)
