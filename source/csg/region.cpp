#include "pch.h"
#include "region.h"
#include "util.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define REGION_LOG(level)    LOG(csg.region, level)

template <class S, int C>
Region<S, C>::Region()
{
   cubes_.reserve(64);
}

template <class S, int C>
Region<S, C>::Region(const Cube& cube)
{
   cubes_.reserve(64);
   if (!cube.IsEmpty()) {
      cubes_.push_back(cube);
   }
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
void Region<S, C>::Add(const Region& region)
{
   for (const Cube& c : region) {
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
   if (cube.IsEmpty()) {
      return;
   }
   Validate();
   ASSERT(!Intersects(cube));

   if (cubes_.empty() || !cubes_.back().CombineWith(cube)) {
      cubes_.push_back(cube);
   } else {
      // The cube on the back has been merged with the new cube.
      // This could enable another merge!  I could write this in
      // a tail-end recursive way to avoid duplicating code, but
      // I believe it would be ultimately much more confusing to 
      // read.
      uint c = cubes_.size();
      while (c > 1) {
         if (!cubes_[c-2].CombineWith(cubes_[c-1])) {
            break;
         }
         c--;
      }
      cubes_.resize(c);
   }

   Validate();
}

template <class S, int C>
void Region<S, C>::AddUnique(const Region& region)
{
   Validate();
   region.Validate();
   for (Cube const& cube : region) {
      ASSERT(!Intersects(cube));
      cubes_.push_back(cube);
   }
   Validate();
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

   Validate();

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

   Validate();
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
Region<S, C> Region<S, C>::operator&(const Cube& c) const
{
   Region result = *this;
   result &= c;
   return result;
}

template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Cube& cube)
{
   unsigned int i = 0;
   unsigned int size = cubes_.size();

   Validate();

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

   Validate();

   return *this;
}


template <class S, int C>
const Region<S, C>& Region<S, C>::operator&=(const Region& other)
{
   Region result;

   Validate();

   // the union of all our little rects clipped against the other region
   for (Cube const& cube : cubes_) {
      result.AddUnique(cube & other);
   }

   *this = result;

   Validate();

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
std::map<int, std::unique_ptr<Region<S, C>>> Region<S, C>::SplitByTagType()
{
   std::map<int, std::unique_ptr<Region<S, C>>> map;

   for (Cube const& cube : cubes_) {
      int tag = cube.GetTag();
      auto i = map.find(tag);

      if (i == map.end()) {
         map[tag] = std::unique_ptr<Region>(new Region());
         i = map.find(tag);
      }

      Region& region = *i->second;
      region.AddUnique(cube);
   }

   return map;
}

template <class S, int C>
S Region<S, C>::GetOctTreeCubeSize(Cube const& bounds)
{
   Point widths = bounds.max - bounds.min;
   S minDimension = widths[0];

   // find the smallest dimension of the bounds
   for (int d = 1; d < C; d++) {
      if (widths[d] < minDimension) {
         minDimension = widths[d];
      }
   }

   // maxSize is the longest length that will fit inside minDimension regardless of origin/offset/alignment
   double maxSize = minDimension / 2;

   // find the largest power of 2 <= maxSize
   double powerOfTwo = std::floor(std::log(maxSize) / std::log(2));
   S size = static_cast<S>(std::pow(2, powerOfTwo));

   return size;
}

// assumes region contains only 1 tag type!
// pass in the bounds so we don't have to call O(n) Region::GetBounds()
template <class S, int C>
void Region<S, C>::OptimizeOctTreeCube(Cube const& bounds)
{
   static const int minCubeCounts[] = { 5, 5, 7 };
   static const S minCubeSize = 32; // CHECKCHECK

   if (IsEmpty()) {
      return;
   }

   Point min = bounds.GetMin();
   Point max = bounds.GetMax();

   if (GetArea() == bounds.GetArea()) {
      cubes_.resize(1);
      cubes_[0].min = min;
      cubes_[0].max = max;
      return;
   }

   if (GetCubeCount() <= minCubeCounts[C]) {
      // merging is optimal for <= 5 rects and <= 7? cubes
      OptimizeByMerge();
      return;
   }

   S cubeSize = max[0] - min[0];

   if (cubeSize < minCubeSize) {
      OptimizeByMerge();
      return;
   }

   Region intersection, optimized;
   Cube childBounds;
   S const childSize = cubeSize / 2;
   Point pt1, pt2;
   bool done = false;
   int d;

   pt1 = min;

   // TODO: refactor
   while (true) {
      for (d = 0; d < C; d++) {
         pt2[d] = pt1[d] + childSize;
      }

      childBounds = Cube(pt1, pt2);
      intersection = *this & childBounds;
      *this -= childBounds;

      intersection.OptimizeOctTreeCube(childBounds);

      optimized.AddUnique(intersection);

      for (d = 0; d < C; d++) {
         if (pt1[d] < max[d]) {
            pt1[d] += childSize;
            break;
         } else {
            // pt1[d] >= max[d]
            if (d < C-1) {
               pt1[d] = min[d];
               // don't break, increment the next dimension
            } else {
               done = true;
               break;
            }
         }
      }

      if (done) {
         break;
      }
   }

   *this = optimized;
}

// TODO: partition by tag type!
template <class S, int C>
void Region<S, C>::OptimizeByOctTree()
{
   if (IsEmpty()) {
      return;
   }
   Validate();

   S areaBefore = GetArea();

   Cube const bounds = GetBounds();
   S cubeSize = GetOctTreeCubeSize(bounds);
   if (cubeSize < 4) {
      OptimizeByMerge();
      return;
   }

   REGION_LOG(7) << "Optimizing region by OctTree - bounds: " << bounds;
   REGION_LOG(7) << "OctTree max cube size: " << cubeSize;
   REGION_LOG(7) << "# cubes before optimization: " << GetCubeCount();

   int d;
   Point min, max;

   for (d = 0; d < C; d++) {
      // floor quantizes towards -infinity which is what we want
      min[d] = static_cast<S>(std::floor((float)bounds.GetMin()[d] / cubeSize) * cubeSize);
      // probably could be GetMax()[d]-1 when S is int
      max[d] = static_cast<S>(std::floor((float)bounds.GetMax()[d] / cubeSize) * cubeSize);
   }

   Region intersection, optimized;
   Cube childBounds;
   Point pt1, pt2;
   bool done = false;

   pt1 = min;

   // TODO: refactor
   while (true) {
      for (d = 0; d < C; d++) {
         pt2[d] = pt1[d] + cubeSize;
      }

      childBounds = Cube(pt1, pt2);
      intersection = *this & childBounds;
      *this -= childBounds;

      intersection.OptimizeOctTreeCube(childBounds);

      optimized.AddUnique(intersection);

      for (d = 0; d < C; d++) {
         if (pt1[d] < max[d]) {
            pt1[d] += cubeSize;
            break;
         } else {
            // pt1[d] >= max[d]
            if (d < C-1) {
               pt1[d] = min[d];
               // don't break, increment the next dimension
            } else {
               done = true;
               break;
            }
         }
      }

      if (done) {
         break;
      }
   }

   *this = optimized;
   REGION_LOG(7) << "# cubes after optimization phase 1: " << GetCubeCount();

   OptimizeByMerge();
   REGION_LOG(7) << "# cubes after optimization phase 2: " << GetCubeCount();

   S areaAfter = GetArea();
   ASSERT(areaBefore == areaAfter);
   Validate();
}

// could speed up by grouping by tag
template <class S, int C>
void Region<S, C>::OptimizeByMerge()
{
   if (IsEmpty()) {
      return;
   }
   Validate();
   S areaBefore = GetArea();

   unsigned int i, j, merged;
   unsigned int size = cubes_.size();
   unsigned int sizeBefore = size;

   //REGION_LOG(7) << "Optimizing region by merge - bounds: " << GetBounds();
   //REGION_LOG(7) << "# cubes before optimization: " << sizeBefore;

   do {
      merged = false;

      // n^2 comparison of all cubes to see if they can be merged
      i = 0;
      while (i < size-1) {
         j = i + 1;
         while (j < size) {
            if (cubes_[i].CombineWith(cubes_[j])) {
               cubes_[j] = cubes_[size-1];
               size--;
               // ith cube now needs to be rechecked for merge on all cubes < i
               // which may recursively invoke other merge checks
               merged = true;
            } else {
               j++;
            }
         }
         i++;
      }

      // don't need to recheck entire list, but keeps code simple for now
   } while (merged);

   cubes_.resize(size);

   //REGION_LOG(7) << "# cubes after optimization: " << size;

   S areaAfter = GetArea();
   ASSERT(areaBefore == areaAfter);
   Validate();
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

template <class S, int C>
void Region<S, C>::Validate() const
{
   // Paranoia validation.  If you're ever concerned that regions may be getting tweaked
   // in some weird way (e.g. a buggy implementation of += or something), Validate is
   // called to ensure variants are correct whenever the state of the region changes.
   // This is INCREDIBLY slow if you do anything > O(1), so leave this compiled out (turn
   // it on if you need it!)
#if 0
   uint i, j, c = cubes_.size();
   for (i = 0; i < c; i++) {
      ASSERT(!cubes_[i].IsEmpty());
      for (j = 0; j < i; j++) {
         if (cubes_[i].Intersects(cubes_[j])) {
            LOG_(0) << "cube " << j << ":" << cubes_[i] << " overlaps with " << i << ":" << cubes_[i];
         }
         ASSERT(!cubes_[i].Intersects(cubes_[j]));
      }
   }
#endif
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
   template Cls Cls::operator&(const Cls::Cube&) const; \
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
   template void Cls::OptimizeByOctTree(); \
   template void Cls::OptimizeByMerge(); \
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

