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

Point3 Cube3::PointIterator::end(INT_MAX, INT_MAX, INT_MAX);
Point2 Rect2::PointIterator::end(INT_MAX, INT_MAX);

Point3f Cube3f::PointIterator::end(FLT_MAX, FLT_MAX, FLT_MAX);
Point2f Rect2f::PointIterator::end(FLT_MAX, FLT_MAX);

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
S Cube<S, C>::GetArea() const
{
   S area = 1;
   for (int i = 0; i < C; i++) {
      area *= (max[i] - min[i]);
   }
   return area;
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

   for (Cube const& c : region) {
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
float Cube<S, C>::DistanceTo(Point const& other) const
{
   Point closest = GetClosestPoint(other);
   return closest.DistanceTo(other);
}

template <typename S, int C>
inline float Cube<S, C>::SquaredDistanceTo(Point const& other) const
{
   Point closest = GetClosestPoint(other);
   return closest.SquaredDistanceTo(other);
}

template <typename S, int C>
float Cube<S, C>::DistanceTo(Cube const& other) const
{
   float d = 0;
   for (int i = 0; i < C; i++) {
      if (other.min[i] > max[i]) {
         d += (other.min[i] - max[i]) * (other.min[i] - max[i]);
      } else if (other.max[i] < min[i]) {
         d += (other.max[i] - min[i]) * (other.max[i] - min[i]);
      }
   }
   return csg::Sqrt(d);
}

template <typename S, int C>
inline float Cube<S, C>::SquaredDistanceTo(Cube const& other) const
{
   float d = 0;
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
void Cube<S, C>::Grow(Point const& pt)
{
   for (int i = 0; i < C; i++) {
      min[i] = std::min(min[i], pt[i]);
      max[i] = std::max(max[i], pt[i]);
   }
}

template <class S, int C>
void Cube<S, C>::Grow(Cube const& cube)
{
   Grow(cube.min);
   Grow(cube.max);
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
   Point<int, C> min, max;
   for (int i = 0; i < C; i++) {
      min[i] = static_cast<int>(std::floor(cube.min[i])); // round toward negative infinity
      max[i] = static_cast<int>(std::ceil(cube.max[i]));  // round toward positive infinity
   }
   return Cube<int, C>(min, max, cube.GetTag());
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
   if (together.GetArea() == GetArea() + cube.GetArea()) {
      *this = together;
      return true;
   }
   return false;
}

template <typename S, int C>
Point<float, C> csg::GetCentroid(Cube<S, C> const& cube)
{
   Point<float, C> centroid = ToFloat(cube.min + cube.max).Scaled(0.5);
   return centroid;
}


template <typename S>
PointIterator<S, 3>::PointIterator(Cube const& c, Point const& iter) :
   bounds_(ToInt(c)),
   axis_(0)
{
   if (iter == end) {
      iter_ = iter;
   } else if (c.GetArea() == 0) {
      iter_ = end;
   } else {
      ASSERT(c.Contains(iter));
      iter_ = iter;
   }
}


template <typename S>
typename PointIterator<S, 3>::Point PointIterator<S, 3>::operator*() const {
   return iter_;
}

template <typename S>
void PointIterator<S, 3>::operator++() {
   if (iter_ != end) {
      iter_.z++;
      if (iter_.z >= bounds_.max.z) {
         iter_.z = static_cast<S>(bounds_.min.z);
         iter_.x++;
         if (iter_.x >= bounds_.max.x) {
            iter_.z = static_cast<S>(bounds_.min.z);
            iter_.x = static_cast<S>(bounds_.min.x);
            iter_.y++;
            if (iter_.y >= bounds_.max.y) {
               iter_ = end;
            }
         }
      }
   }
}

template <typename S>
bool PointIterator<S, 3>::operator!=(const PointIterator& rhs) const {
   return iter_ != rhs.iter_;
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
   template float Cls::DistanceTo(const Cls& other) const; \
   template float Cls::DistanceTo(const Cls::Point& other) const; \
   template void Cls::Grow(const Cls::Point& other); \
   template void Cls::Grow(const Cls& other); \
   template Cls Cls::Intersection(Cls const& other) const; \
   template bool Cls::CombineWith(const Cls& other); \
   template Cls::Region Cls::GetBorder() const; \


#define MAKE_POINT_ITERATOR(Cls) \
   template Cls::PointIterator(Cube const& c, Point const& iter); \
   template Cls::Point Cls::operator*() const; \
   template void Cls::operator++(); \
   template bool Cls::operator!=(Cls const& rhs) const; \


MAKE_CUBE(Cube3)
MAKE_CUBE(Cube3f)
MAKE_CUBE(Rect2)
MAKE_CUBE(Rect2f)
MAKE_CUBE(Line1)
MAKE_CUBE(Line1f)

MAKE_POINT_ITERATOR(PointIterator3)
MAKE_POINT_ITERATOR(PointIterator3f)

#define DEFINE_CUBE_CONVERSIONS(C) \
   template Cube<float, C> csg::ToFloat(Cube<int, C> const&); \
   template Cube<float, C> const& csg::ToFloat(Cube<float, C> const&); \
   template Cube<int, C> const& csg::ToInt(Cube<int, C> const&); \
   template Cube<int, C> csg::ToInt(Cube<float, C> const&);

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
