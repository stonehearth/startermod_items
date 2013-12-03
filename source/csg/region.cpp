#include "pch.h"
#include "region.h"
#include "util.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

template <class S, int C>
Region<S, C>::Region()
{
   cubes_.reserve(64);
}

template <class S, int C>
Region<S, C>::Region(const Cube& cube)
{
   cubes_.reserve(64);
   cubes_.push_back(cube);
}

template <class S, int C>
Region<S, C>::Region(const Region&& r)
{
   cubes_ = std::move(r.cubes_);
}

template <class S, int C>
S Region<S, C>::GetArea() const
{
   S area = 0;
   for (const auto& cube : cubes_) {
      area += cube.GetArea();
   }
   return area;
}


template <class S, int C>
bool Region<S, C>::IsEmpty() const
{
   for (const auto& c : cubes_) {
      // xxx: would it be faster to ensure there are no 0 area cubes in
      // the array?  almost certainly so.
      if (c.GetArea() > 0) {
         return false;
      }
   }
   return true;
}

template <class S, int C>
void Region<S, C>::Clear()
{
   cubes_.clear();
}

template <class S, int C>
void Region<S, C>::Add(const Region& other)
{
   for (const Cube& c : other) {
      Add(c);
   }
}

template <class S, int C>
void Region<S, C>::Add(const Cube& cube)
{
   Subtract(cube);
   AddUnique(cube);
}

template <class S, int C>
void Region<S, C>::Add(const Point& point)
{
   Add(Cube(point));
}

template <class S, int C>
void Region<S, C>::AddUnique(const Cube& cube)
{
   ASSERT(!Intersects(cube));
   cubes_.push_back(cube);
}

template <class S, int C>
void Region<S, C>::AddUnique(const Region& region)
{
   for (Cube const& cube : region) {
      ASSERT(!Intersects(cube));
      cubes_.push_back(cube);
   }
}

template <class S, int C>
void Region<S, C>::Subtract(const Point& pt)
{
   Subtract(Cube(pt));
}

template <class S, int C>
void Region<S, C>::Subtract(const Cube& cube)
{
   CubeVector added;

   unsigned int i = 0;
   unsigned int size = cubes_.size();

   while (i < size) {
      Cube const& src = cubes_[i];
      if (!src.Intersects(cube)) {
         i++;
      } else {
         Region replacement = src - cube;
         if (replacement.IsEmpty()) {
            cubes_[i] = cubes_[size - 1];
            size--;
         } else {
            cubes_[i] = replacement[0];
            added.insert(added.end(), replacement.begin() + 1, replacement.end());
            i++;
         }
      }
   }
   cubes_.resize(size);
   cubes_.insert(cubes_.end(), added.begin(), added.end());
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(const Cube& cube) const
{
   Region result(*this);
   result -= cube;
   return result;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(const Region& r) const
{
   Region result(*this);
   for (const auto& c : r) {
      result -= c;
   }
   return result;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator-=(const Cube& cube)
{
   Subtract(cube);
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator+=(const Cube& cube)
{
   Add(cube);
   return *this;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator&(const Region& r) const
{
   Region result = *this;
   result &= r;
   return result;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Cube& cube)
{
   unsigned int i = 0;
   unsigned int size = cubes_.size();

   while (i < size) {
      Cube replacement = cubes_[i] & cube;
      if (replacement.IsEmpty()) {
         cubes_[i] = cubes_[size - 1];
         size--;
      } else {
         cubes_[i] = replacement;
         i++;
      }
   }
   cubes_.resize(size);
   return *this;
}


template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Region& other)
{
   Region result;

   // the union of all our little rects clipped against the other region
   for (Cube const& cube : cubes_) {
      result.AddUnique(cube & other);
   }

   *this = result;
   return *this;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator-=(const Region& r)
{
   Subtract(r);
   return *this;
}


template <class S, int C>
void Region<S, C>::Subtract(const Region& r)
{
   for (const auto &rc : r) {
      Subtract(rc);
   }
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator+=(const Region& r)
{
   for (const auto &rc : r) {
      Add(rc);
   }
   return *this;
}

template <class S, int C>
bool Region<S, C>::Intersects(const Cube& cube) const
{
   for (const Cube& c : *this) {
      if (cube.Intersects(c)) {
         return true;
      }
   }
   return false;
}
template <class S, int C>
bool Region<S, C>::Contains(const Point& pt) const
{
   for (Cube const& cube : cubes_) {
      if (cube.Contains(pt)) {
         return true;
      }
   }
   return false;
}

template <class S, int C>
Point<S, C> Region<S, C>::GetClosestPoint2(const Point& from, S *dSquared) const
{
   ASSERT(!IsEmpty());

   S best = (S)INT_MAX;

   Point closest;
   for (int i = 0; i < C; i++) {
      // xxx - need some kind of type traits to make this NOT SUCK
      closest[i] = (S)INT_MAX;
   }

   for (const auto &c : cubes_) {
      S d2;
      Point candidate = c.GetClosestPoint2(from, &d2);
      if (d2 < best) {
         best = d2;
         closest = candidate;
      }
   }
   if (dSquared) {
      *dSquared = best;
   }
   return closest;
}

template <class S, int C>
void Region<S, C>::Optimize()
{
}

template <class S, int C>
Cube<S, C> Region<S, C>::GetBounds() const
{
   if (IsEmpty()) {
      return Cube::zero;
   }

   Cube bounds = cubes_[0];
   int i, c = cubes_.size();
   for (i = 1; i < c; i++) {
      bounds.Grow(cubes_[i].GetMin());
      bounds.Grow(cubes_[i].GetMax());
   }
   return bounds;
}

template <class S, int C>
void Region<S, C>::Translate(const Point& pt)
{
   for (auto &c : cubes_) {
      c.Translate(pt);
   }
}

template <class S, int C>
Region<S, C> Region<S, C>::Translated(const Point& pt) const
{
   Region result = (*this);
   result.Translate(pt);
   return std::move(result);
}

template <class S, int C>
Region<S, C> Region<S, C>::Inflated(Point const& pt) const
{
   Region result;
   for (auto &c : cubes_) {
      result.Add(c.Inflated(pt));
   }
   return std::move(result);
}

#if 0
Region3 radiant::csg::GetBorderXZ(const Region3 &other)
{
   Region3 result;

   for (const Cube3 &c : other) {
      Point3 p0 = c.GetMin();
      Point3 p1 = c.GetMax();
      Region3::ScalarType w = p1[0] - p0[0], h = p1[2] - p0[2];
      result.Add(Cube3(p0 - Point3(0, 0, 1), p0 + Point3(w, 1, 0)));  // top
      result.Add(Cube3(p0 - Point3(1, 0, 0), p0 + Point3(0, 1, h)));  // left
      result.Add(Cube3(p0 + Point3(0, 0, h), p0 + Point3(w, 1, h + 1)));  // bottom
      result.Add(Cube3(p0 + Point3(w, 0, 0), p0 + Point3(w + 1, 1, h)));  // right
   }
   result.Optimize();

   return result;
}
#endif

template <int C>
Region<float, C> csg::ToFloat(Region<int, C> const& region) {
   Region<float, C> result;
   for (Cube<int, C> const& cube : region) {
      result.AddUnique(ToFloat(cube));
   }
   return result;
}

template <int C>
Region<float, C> const& csg::ToFloat(Region<float, C> const& region) {
   return region;
}

template <int C>
Region<int, C> csg::ToInt(Region<float, C> const& region) {
   Region<int, C> result;
   for (Cube<float, C> const& cube : region) {
      result.Add(ToInt(cube)); // so expensive!
   }
   return result;
}

template <int C>
Region<int, C> const& csg::ToInt(Region<int, C> const& region) {
   return region;
}

#define MAKE_REGION(Cls) \
   const Cls Cls::empty; \
   template Cls::Region(); \
   template Cls::Region(const Cls::Cube&); \
   template Cls::ScalarType Cls::GetArea() const; \
   template void Cls::Clear(); \
   template void Cls::Add(const Cls&); \
   template void Cls::Add(const Cls::Cube&); \
   template void Cls::Add(const Cls::Point&); \
   template void Cls::AddUnique(const Cls::Cube&); \
   template void Cls::AddUnique(const Cls&); \
   template void Cls::Subtract(const Cls&); \
   template void Cls::Subtract(const Cls::Cube&); \
   template void Cls::Subtract(const Cls::Point&); \
   template Cls Cls::operator-(const Cls&) const; \
   template Cls Cls::operator-(const Cls::Cube&) const; \
   template Cls Cls::operator&(const Cls&) const; \
   template const Cls& Cls::operator&=(const Cls::Cube&); \
   template const Cls& Cls::operator&=(const Cls&); \
   template const Cls& Cls::operator+=(const Cls::Cube&); \
   template const Cls& Cls::operator-=(const Cls::Cube&); \
   template const Cls& Cls::operator+=(const Cls&); \
   template const Cls& Cls::operator-=(const Cls&); \
   template bool Cls::Intersects(const Cls::Cube&) const; \
   template bool Cls::Contains(const Cls::Point&) const; \
   template Cls::Point Cls::GetClosestPoint2(const Cls::Point&, Cls::ScalarType*) const; \
   template void Cls::Optimize(); \
   template Cls::Cube Cls::GetBounds() const; \
   template void Cls::Translate(const Cls::Point& pt); \
   template Cls Cls::Translated(const Cls::Point& pt) const; \
   template Cls Cls::Inflated(const Cls::Point& pt) const; \

MAKE_REGION(Region3)
MAKE_REGION(Region3f)
MAKE_REGION(Region2)
MAKE_REGION(Region2f)
MAKE_REGION(Region1)

#define DEFINE_REGION_CONVERSIONS(C) \
   template Region<float, C> csg::ToFloat(Region<int, C> const&); \
   template Region<float, C> const& csg::ToFloat(Region<float, C> const&); \
   template Region<int, C> const& csg::ToInt(Region<int, C> const&); \
   template Region<int, C> csg::ToInt(Region<float, C> const&); \

DEFINE_REGION_CONVERSIONS(2)
DEFINE_REGION_CONVERSIONS(3)

