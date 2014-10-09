#include "pch.h"
#include "csg.h"
#include "cube.h"
#include "ray.h"
#include "region.h"
#include "csg/util.h" // xxx: should be csg/namespace.h
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Cube3 Cube3::zero(Point3(0, 0, 0), Point3(0, 0, 0));
Cube3 Cube3::one(Point3(0, 0, 0), Point3(1, 1, 1));
Rect2 Rect2::zero(Point2(0, 0), Point2(0, 0));
Rect2 Rect2::one(Point2(0, 0), Point2(1, 1));
Line1 Line1::zero(Point1(0), Point1(0));
Line1 Line1::one(Point1(0), Point1(1));

Cube3f Cube3f::zero(Point3f(0, 0, 0), Point3f(0, 0, 0));
Cube3f Cube3f::one(Point3f(0, 0, 0), Point3f(1, 1, 1));
Rect2f Rect2f::zero(Point2f(0, 0), Point2f(0, 0));
Rect2f Rect2f::one(Point2f(0, 0), Point2f(1, 1));
Line1f Line1f::zero(Point1f(0), Point1f(0));
Line1f Line1f::one(Point1f(0), Point1f(1));

template <class C>
static bool Cube3IntersectsImpl(const C& cube, const csg::Ray3& ray, double& d)
{
   const auto& min_value = cube.GetMin();
   const auto& max_value = cube.GetMax();
   double max_s = -FLT_MAX;
   double min_t = FLT_MAX;

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
         double s = (min_value[i] - ray.origin[i]) / ray.direction[i];
         double t = (max_value[i] - ray.origin[i]) / ray.direction[i];
         if (s > t) {
            double temp = s;
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

bool csg::Cube3Intersects(const Cube3& rgn, const Ray3& ray, double& distance)
{
   return Cube3IntersectsImpl(rgn, ray, distance);
}

bool csg::Cube3Intersects(const Cube3f& rgn, const Ray3& ray, double& distance)
{
   return Cube3IntersectsImpl(rgn, ray, distance);
}


template <typename S, int C>
Cube<S, C>::Cube() :
   tag_(0)
{
}

template <typename S, int C>
Cube<S, C>::Cube(Point const& min_value, int tag) :
   tag_(tag),
   min(min_value)
{
   for (int i = 0; i < C; i++) {
      max[i] = min[i] + 1;
   }
}

template <typename S, int C>
Cube<S, C>::Cube(Point const& min_value, Point const& max_value, int tag) :
   tag_(tag),
   min(min_value),
   max(max_value)
{
   DEBUG_ONLY(
      for (int i = 0; i < C; i++) {
         ASSERT(min[i] <= max[i]);
      }
   )
}

template <typename S, int C>
S GetAreaFn(Cube<S, C> const&);

template <typename S>
S GetAreaFn(Cube<S, 1> const& cube)
{
   return (cube.max.x - cube.min.x);
}

template <typename S>
S GetAreaFn(Cube<S, 2> const& cube)
{
   return (cube.max.x - cube.min.x) * (cube.max.y - cube.min.y);
}

template <typename S>
S GetAreaFn(Cube<S, 3> const& cube)
{
   return (cube.max.x - cube.min.x) * (cube.max.y - cube.min.y) * (cube.max.z - cube.min.z);
}

template <typename S, int C>
S Cube<S, C>::GetArea() const
{
   return GetAreaFn(*this);
}

/*
 * The compiler is not good at making this fast.
 *

template <typename S, int C>
bool Cube<S, C>::Intersects(Cube const& other) const
{
   for (int i = 0; i < C; i++) {
      if (min[i] >= other.max[i] || other.min[i] >= max[i]) {
         return false;
      }
   }
   return true;
}

*/

template <typename S, int C>
static bool IntersectsSpecialization(Cube<S, C> const& l, Cube<S, C> const& r);

template <typename S>
static inline bool IntersectsSpecialization(Cube<S, 3> const& l, Cube<S, 3> const& r)
{
   bool no = l.min.x >= r.max.x ||
             r.min.x >= l.max.x ||
             l.min.z >= r.max.z ||
             r.min.z >= l.max.z ||
             l.min.y >= r.max.y ||
             r.min.y >= l.max.y;
   return !no;
}

template <typename S>
static inline bool IntersectsSpecialization(Cube<S, 2> const& l, Cube<S, 2> const& r)
{
   bool no = l.min.x >= r.max.x ||
             r.min.x >= l.max.x ||
             l.min.y >= r.max.y ||
             r.min.y >= l.max.y;
   return !no;
}

template <typename S>
static inline bool IntersectsSpecialization(Cube<S, 1> const& l, Cube<S, 1> const& r)
{
   bool no = l.min.x >= r.max.x ||
             r.min.x >= l.max.x;
   return !no;
}

template <typename S, int C>
bool Cube<S, C>::Intersects(Cube const& other) const
{
   return IntersectsSpecialization(*this, other);
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator-() const
{
   return this->Scaled(-1);
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::Intersection(Cube const& other) const
{
   Cube result;
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
Cube<S, C> Cube<S, C>::Inflated(Point const& amount) const
{
   Point newMin = min - amount;
   Point newMax = max + amount;
   for (int i = 0; i < C; i++) {
      if (newMin[i] >= newMax[i]) {
         return Cube::zero;
      }
   }
   return Cube(newMin, newMax);
}

template <typename S, int C>
Region<S, C> Cube<S, C>::GetBorder() const
{
   Region result(*this);

   Cube inner = Inflated(-Point::one);
   if (inner != Cube::zero) {
      result.Subtract(inner);
   }
   return result;
}

template <typename S, int C>
Region<S, C> Cube<S, C>::operator-(Cube const& rhs) const
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
Region<S, C> Cube<S, C>::operator-(Region const& rhs) const
{
   return Region(*this) - rhs;
}

template <typename S, int C>
bool Cube<S, C>::operator==(Cube const& other) const
{
   return min == other.min && max == other.max;
}

template <typename S, int C>
bool Cube<S, C>::operator!=(Cube const& other) const
{
   return min != other.min || max != other.max;
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator&(Cube const& other) const
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
Region<S, C> Cube<S, C>::operator&(Region const& region) const
{
   Region result;

   for (Cube const& c : EachCube(region)) {
      Cube clipped = *this & c;
      if (!clipped.IsEmpty()) {
         result.AddUnique(clipped);
      }
   }
   return result;
}


// Specialize GetClosestPoint 3 times to avoid looping through C.
template <typename S, int C> 
inline Point<S, C> GetClosestPointFn(Cube<S, C> const& c, Point<S, C> const& other);

template <typename S> 
inline Point<S, 1> GetClosestPointFn(Cube<S, 1> const& c, Point<S, 1> const& other)
{
   return Point<S, 1>(
         std::max(std::min(other.x, c.max.x - 1), c.min.x)
      );
}

template <typename S> 
inline Point<S, 2> GetClosestPointFn(Cube<S, 2> const& c, Point<S, 2> const& other)
{
   return Point<S, 2>(
         std::max(std::min(other.x, c.max.x - 1), c.min.x),
         std::max(std::min(other.y, c.max.y - 1), c.min.y)
      );
}

template <typename S> 
inline Point<S, 3> GetClosestPointFn(Cube<S, 3> const& c, Point<S, 3> const& other)
{
   return Point<S, 3>(
         std::max(std::min(other.x, c.max.x - 1), c.min.x),
         std::max(std::min(other.y, c.max.y - 1), c.min.y),
         std::max(std::min(other.z, c.max.z - 1), c.min.z)
      );
}


template <typename S, int C>
Point<S, C> Cube<S, C>::GetClosestPoint(Point const& other) const
{
   return GetClosestPointFn(*this, other);
}

template <typename S, int C>
Cube<S, C> Cube<S, C>::operator+(Point const& offset) const
{
   return Cube(min + offset, max + offset, tag_);
}

template <typename S, int C>
double Cube<S, C>::DistanceTo(Point const& other) const
{
   Point closest = GetClosestPoint(other);
   return closest.DistanceTo(other);
}

template <typename S, int C>
inline double Cube<S, C>::SquaredDistanceTo(Point const& other) const
{
   Point closest = GetClosestPoint(other);
   return closest.SquaredDistanceTo(other);
}

template <typename S, int C>
double Cube<S, C>::DistanceTo(Cube const& other) const
{
   double d = 0;
   for (int i = 0; i < C; i++) {
      if (other.min[i] > max[i]) {
         d += (other.min[i] - max[i]) * (other.min[i] - max[i]);
      } else if (other.max[i] < min[i]) {
         d += (other.max[i] - min[i]) * (other.max[i] - min[i]);
      }
   }
   return std::sqrt(d);
}

template <typename S, int C>
inline double Cube<S, C>::SquaredDistanceTo(Cube const& other) const
{
   double d = 0;
   for (int i = 0; i < C; i++) {
      if (other.min[i] > max[i]) {
         d += (other.min[i] - max[i]) * (other.min[i] - max[i]);
      } else if (other.max[i] < min[i]) {
         d += (other.max[i] - min[i]) * (other.max[i] - min[i]);
      }
   }
   return d;
}

template <typename S, int C>
bool Cube<S, C>::Contains(Point const& pt) const
{
   return IsBetween(min, pt, max);
}

template <class S, int C>
void GrowFn(Cube<S, C>& cube, Point<S, C> const& pt);

template <class S>
void GrowFn(Cube<S, 1>& cube, Point<S, 1> const& pt)
{
   cube.min.x = std::min(cube.min.x, pt.x);
   cube.max.x = std::max(cube.max.x, pt.x);
}

template <class S>
void GrowFn(Cube<S, 2>& cube, Point<S, 2> const& pt)
{
   cube.min.x = std::min(cube.min.x, pt.x);
   cube.max.x = std::max(cube.max.x, pt.x);
   cube.min.y = std::min(cube.min.y, pt.y);
   cube.max.y = std::max(cube.max.y, pt.y);
}

template <class S>
void GrowFn(Cube<S, 3>& cube, Point<S, 3> const& pt)
{
   cube.min.x = std::min(cube.min.x, pt.x);
   cube.max.x = std::max(cube.max.x, pt.x);
   cube.min.y = std::min(cube.min.y, pt.y);
   cube.max.y = std::max(cube.max.y, pt.y);
   cube.min.z = std::min(cube.min.z, pt.z);
   cube.max.z = std::max(cube.max.z, pt.z);
}

template <class S, int C>
void Cube<S, C>::Grow(Point const& pt)
{
   GrowFn(*this, pt);
}

template <class S, int C>
void Cube<S, C>::Grow(Cube const& cube)
{
   Grow(cube.min);
   Grow(cube.max);
}

template <int C>
Cube<double, C> csg::ToFloat(Cube<int, C> const& cube) {
   return Cube<double, C>(ToFloat(cube.min), ToFloat(cube.max), cube.GetTag());
}

template <int C>
Cube<double, C> const& csg::ToFloat(Cube<double, C> const& pt) {
   return pt;
}

template <int C>
Cube<int, C> ToIntFn(Cube<double, C> const& cube);
 
template <>
Cube<int, 1> ToIntFn(Cube<double, 1> const& cube) {
   Cube<int, 1> result;
   result.SetTag(cube.GetTag());
   result.min.x = static_cast<int>(std::floor(cube.min.x)); // round toward negative infinity
   result.max.x = static_cast<int>(std::ceil(cube.max.x));  // round toward positive infinity
   return result;
}

template <>
Cube<int, 2> ToIntFn(Cube<double, 2> const& cube) {
   Cube<int, 2> result;
   result.SetTag(cube.GetTag());
   result.min.x = static_cast<int>(std::floor(cube.min.x)); // round toward negative infinity
   result.min.y = static_cast<int>(std::floor(cube.min.y));
   result.max.x = static_cast<int>(std::ceil(cube.max.x));  // round toward positive infinity
   result.max.y = static_cast<int>(std::ceil(cube.max.y));
   return result;
}

template <>
Cube<int, 3> ToIntFn(Cube<double, 3> const& cube) {
   Cube<int, 3> result;
   result.SetTag(cube.GetTag());
   result.min.x = static_cast<int>(std::floor(cube.min.x)); // round toward negative infinity
   result.min.y = static_cast<int>(std::floor(cube.min.y));
   result.min.z = static_cast<int>(std::floor(cube.min.z));
   result.max.x = static_cast<int>(std::ceil(cube.max.x));  // round toward positive infinity
   result.max.y = static_cast<int>(std::ceil(cube.max.y));
   result.max.z = static_cast<int>(std::ceil(cube.max.z));
   return result;
}

template <int C>
Cube<int, C> csg::ToInt(Cube<double, C> const& cube) {
   return ToIntFn(cube);
}

template <int C>
Cube<int, C> const& csg::ToInt(Cube<int, C> const& cube) {
   return cube;
}

template <class S, int C>
bool Cube<S, C>::CombineWith(Cube const& cube)
{
   if (tag_ != cube.tag_) {
      return false;
   }
   Cube together(*this);
   together.Grow(cube);

   S a1 = together.GetArea();
   S a2 = GetArea();
   S a3 = cube.GetArea();
   S a4 = a2 + a3;
   if (a1 == a4) {
      *this = together;
      return true;
   }
   return false;
}

template <typename S, int C>
Point<double, C> csg::GetCentroid(Cube<S, C> const& cube)
{
   Point<double, C> centroid = ToFloat(cube.min + cube.max).Scaled(0.5f);
   return centroid;
}

#define MAKE_CUBE(Cls) \
   template Cls::Cube(); \
   template Cls::Cube(const Cls::Point&, int); \
   template Cls::Cube(const Cls::Point&, const Cls::Point&, int); \
   template Cls::ScalarType Cls::GetArea() const; \
   template bool Cls::Intersects(const Cls& other) const; \
   template bool Cls::operator==(const Cls& offset) const; \
   template bool Cls::operator!=(const Cls& offset) const; \
   template Cls Cls::operator&(const Cls& offset) const; \
   template Cls::Region Cls::operator&(const Cls::Region& offset) const; \
   template Cls Cls::operator+(const Cls::Point& offset) const; \
   template Cls Cls::operator-() const; \
   template Cls::Region Cls::operator-(const Cls& other) const; \
   template Cls::Region Cls::operator-(const Cls::Region& other) const; \
   template bool Cls::Contains(const Cls::Point& other) const; \
   template Cls::Point Cls::GetClosestPoint(const Cls::Point& other) const; \
   template double Cls::DistanceTo(const Cls& other) const; \
   template double Cls::DistanceTo(const Cls::Point& other) const; \
   template void Cls::Grow(const Cls::Point& other); \
   template void Cls::Grow(const Cls& other); \
   template Cls Cls::Intersection(Cls const& other) const; \
   template bool Cls::CombineWith(const Cls& other); \
   template Cls::Region Cls::GetBorder() const; \

MAKE_CUBE(Cube3)
MAKE_CUBE(Cube3f)
MAKE_CUBE(Rect2)
MAKE_CUBE(Rect2f)
MAKE_CUBE(Line1)
MAKE_CUBE(Line1f)

#define DEFINE_CUBE_CONVERSIONS(C) \
   template Cube<double, C> csg::ToFloat(Cube<int, C> const&); \
   template Cube<double, C> const& csg::ToFloat(Cube<double, C> const&); \
   template Cube<int, C> const& csg::ToInt(Cube<int, C> const&); \
   template Cube<int, C> csg::ToInt(Cube<double, C> const&);

DEFINE_CUBE_CONVERSIONS(1)
DEFINE_CUBE_CONVERSIONS(2)
DEFINE_CUBE_CONVERSIONS(3)

template <> Cube3 csg::ConvertTo(Cube3 const& cube) { return cube; }
template <> Cube3f csg::ConvertTo(Cube3f const& cube) { return cube; }
template <> Cube3f csg::ConvertTo(Cube3 const& cube) { return ToFloat(cube); }
template <> Cube3 csg::ConvertTo(Cube3f const& cube) { return ToInt(cube); }

// define centroid methods
template Point2f csg::GetCentroid(Rect2 const& rect);
template Point3f csg::GetCentroid(Cube3 const& cube);
template Point2f csg::GetCentroid(Rect2f const& rect);
template Point3f csg::GetCentroid(Cube3f const& cube);
