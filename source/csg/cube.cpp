#include "pch.h"
#include "csg.h"
#include "cube.h"
#include "ray.h"
#include "region.h"
#include "csg/util.h" // xxx: should be csg/namespace.h

using namespace ::radiant;
using namespace ::radiant::csg;

Cube3 Cube3::zero(Point3(0, 0, 0), Point3(0, 0, 0));
Cube3 Cube3::one(Point3(0, 0, 0), Point3(1, 1, 1));
Rect2 Rect2::zero(Point2(0, 0), Point2(0, 0));
Rect2 Rect2::one(Point2(0, 0), Point2(1, 1));
Line1 Line1::zero(Point1(0), Point1(0));
Line1 Line1::one(Point1(0), Point1(1));

Rect2f Rect2f::zero(Point2f(0, 0), Point2f(0, 0));
Rect2f Rect2f::one(Point2f(0, 0), Point2f(1, 1));
Cube3f Cube3f::zero(Point3f(0, 0, 0), Point3f(0, 0, 0));
Cube3f Cube3f::one(Point3f(0, 0, 0), Point3f(1, 1, 1));

template <class C>
static bool Cube3IntersectsImpl(const C& cube, const csg::Ray3& ray, float& d)
{
   const auto& min_value = cube.GetMin();
   const auto& max_value = cube.GetMax();
   float max_s = -FLT_MAX;
   float min_t = FLT_MAX;

   // do tests against three sets of planes
   for (int i = 0; i < 3; ++i) {
      // ray is parallel to plane
      if (csg::IsZero(ray.direction[i])) {
         // ray passes by box
         if (ray.origin[i] < min_value[i] || ray.origin[i] > max_value[i]) {
            return false;
         }
      } else {
         // compute intersection parameters and sort
         float s = (min_value[i] - ray.origin[i]) / ray.direction[i];
         float t = (max_value[i] - ray.origin[i]) / ray.direction[i];
         if (s > t) {
            float temp = s;
            s = t;
            t = temp;
         }

         // adjust min_value and max_value values
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
Cube<S, C>::Cube(const Point& min_value, int tag) :
   tag_(tag),
   min(min_value)
{
   for (int i = 0; i < C; i++) {
      max[i] = min[i] + 1;
   }
}

template <typename S, int C>
Cube<S, C>::Cube(const Point& min_value, const Point& max_value, int tag) :
   tag_(tag),
   min(min_value),
   max(max_value)
{
   for (int i = 0; i < C; i++) {
      ASSERT(min[i] <= max[i]);
   }
}

template <typename S, int C>
S Cube<S, C>::GetArea() const
{
   S area = 1;
   for (int i = 0; i < C; i++) {
      area *= (max[i] - min[i]);
   }
   return area;
}

template <typename S, int C>
bool Cube<S, C>::Intersects(const Cube& other) const
{
   for (int i = 0; i < C; i++) {
      if (csg::IsGreaterEqual(min[i], other.max[i]) ||
          csg::IsGreaterEqual(other.min[i], max[i])) {
         return false;
      }
   }
   return true;
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator-() const
{
   return this->Scaled(-1);
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
      ASSERT(lhs.min[i] < rhs.max[i] && rhs.min[i] < lhs.max[i]);
      if (lhs.min[i] < rhs.min[i]) {
         Cube keep(lhs);
         keep.max[i] = rhs.min[i];
         next.min[i] = rhs.min[i];
         result.AddUnique(keep);
      }
      if (rhs.max[i] < lhs.max[i]) {
         Cube keep(lhs);
         keep.min[i] = rhs.max[i];
         next.max[i] = rhs.max[i];
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
      result.min[i] = std::max(min[i], other.min[i]);
      result.max[i] = std::min(max[i], other.max[i]);
      if (result.max[i] < result.min[i]) {
         result.max[i] = result.min[i];
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
   return Cube(min + offset, max + offset, tag_);
}

template <typename S, int C>
Point<S, C> Cube<S, C>::GetClosestPoint2(const Point& other, S* d) const
{
   Point result;
   for (int i = 0; i < C; i++) {
      result[i] = std::max(std::min(other[i], max[i] - 1), min[i]);
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
      if (!IsBetween(min[i], pt[i], max[i])) {
         return false;
      }
   }
   return true;
}

template <class S, int C>
void Cube<S, C>::Grow(const Point& pt)
{
   for (int i = 0; i < C; i++) {
      min[i] = std::min(min[i], pt[i]);
      max[i] = std::max(max[i], pt[i]);
   }
}

template <int C>
Cube<float, C> csg::ToFloat(Cube<int, C> const& cube) {
   return Cube<float, C>(ToFloat(cube.min), ToFloat(cube.max), cube.GetTag());
}

template <int C>
Cube<float, C> const& csg::ToFloat(Cube<float, C> const& pt) {
   return pt;
}

template <int C>
Cube<int, C> csg::ToInt(Cube<float, C> const& cube) {
   return Cube<int, C>(csg::ToInt(cube.min), csg::ToInt(cube.max), cube.GetTag());
}

template <int C>
Cube<int, C> const& csg::ToInt(Cube<int, C> const& cube) {
   return cube;
}


#define MAKE_CUBE(Cls) \
   template Cls::Cube(); \
   template Cls::Cube(const Cls::Point&, int); \
   template Cls::Cube(const Cls::Point&, const Cls::Point&, int); \
   template Cls::ScalarType Cls::GetArea() const; \
   template bool Cls::Intersects(const Cls& other) const; \
   template Cls Cls::operator&(const Cls& offset) const; \
   template Cls::Region Cls::operator&(const Cls::Region& offset) const; \
   template Cls Cls::operator+(const Cls::Point& offset) const; \
   template Cls Cls::operator-() const; \
   template Cls::Region Cls::operator-(const Cls& other) const; \
   template Cls::Region Cls::operator-(const Cls::Region& other) const; \
   template bool Cls::Contains(const Cls::Point& other) const; \
   template Cls::Point Cls::GetClosestPoint2(const Cls::Point& other, Cls::ScalarType*) const; \
   template void Cls::Grow(const Cls::Point& other); \

MAKE_CUBE(Cube3)
MAKE_CUBE(Cube3f)
MAKE_CUBE(Rect2)
MAKE_CUBE(Rect2f)
MAKE_CUBE(Line1)

#define DEFINE_CUBE_CONVERSIONS(C) \
   template Cube<float, C> csg::ToFloat(Cube<int, C> const&); \
   template Cube<float, C> const& csg::ToFloat(Cube<float, C> const&); \
   template Cube<int, C> const& csg::ToInt(Cube<int, C> const&); \
   template Cube<int, C> csg::ToInt(Cube<float, C> const&);

DEFINE_CUBE_CONVERSIONS(2)
DEFINE_CUBE_CONVERSIONS(3)
