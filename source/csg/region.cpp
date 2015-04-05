#include "pch.h"
#include <tbb/spin_mutex.h>
#include "region.h"
#include "util.h"
#include "core/static_string.h"
#include "lib/perfmon/timer.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

#define REGION_LOG(level)   LOG(csg.region, level) << " [region:" << this << " ~size:" << (cubes_.size()) << "]"
#define CHURN_LOG(level)    LOG(csg.churn, level)  << " [region:" << this << " churn:" << _churn << " ~size:" << (cubes_.size()) << "]"

// REGION_PARANOIA_LEVEL determines how aggressively we should error check
// regions for internal consistency.
//
// REGION_PARANOIA_LEVEL == 1 simply checks for unique tags and cubes added
// via AddUnique() do not overlap the region.
//
// REGION_PARANOIA_LEVEL == 2 does agressive, O(n*n) error checking several
// times per operation (sometimes n times!).  It's only useful when debugging
// known Region errors, or changing implementation details.

#if defined(REGION_PARANOIA_LEVEL)
#elif RADIANT_OPT_LEVEL == RADIANT_OPT_LEVEL_DEV
#  define REGION_PARANOIA_LEVEL 1
#else
#  define REGION_PARANOIA_LEVEL 0
#endif

// #define REGION_PROFILE_OPTIMIZE

#if defined(REGION_COUNT_OPTIMIZE_COMBINES)
#  define INCREMENT_COMBINE_COUNT() ++_combineCount
#else
#  define INCREMENT_COMBINE_COUNT()
#endif

#if defined(REGION_PROFILE_OPTIMIZE)
   static tbb::spin_mutex __optimizeProfilerLock;
   static std::unordered_map<core::StaticString, perfmon::CounterValueType, core::StaticString::Hash> __optimizeProfiler;
   static perfmon::CounterValueType __optimizeLastReportTime = 0;
#endif

template <class S, int C>
void Region<S, C>::SetOptimizeStrategy(OptimizeStrategy s)
{
   __optimizeStrategy = s;
}


template <class S, int C>
Region<S, C>::Region() :
#if defined(REGION_COUNT_OPTIMIZE_COMBINES)
   _combineCount(0),
#endif
   _churn(0)
{
#if !defined(EASTL_REGIONS)
   cubes_.reserve(INITIAL_CUBE_SPACE);
#endif
}

template <class S, int C>
Region<S, C>::Region(Cube const& cube) :
#if defined(REGION_COUNT_OPTIMIZE_COMBINES)
   _combineCount(0),
#endif
   _churn(0)
{
#if !defined(EASTL_REGIONS)
   cubes_.reserve(INITIAL_CUBE_SPACE);
#endif
   if (!cube.IsEmpty()) {
      cubes_.push_back(cube);
   }
}

template <class S, int C>
Region<S, C>::Region(Region const&& r)
{
   cubes_ = std::move(r.cubes_);
   _churn = r._churn;
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
   _churn = 0;
   CHURN_LOG(7) << "resetting churn on clear";
   cubes_.clear();
}

template <class S, int C>
void Region<S, C>::Add(Point const& point)
{
   Add(Cube(point));
}

template <class S, int C>
void Region<S, C>::Add(Cube const& cube)
{
   Subtract(cube);
   AddUnique(cube);
}

template <class S, int C>
void Region<S, C>::Add(Region const& region)
{
   for (Cube const& c : EachCube(region)) {
      Add(c);
   }
}

template <class S, int C>
void Region<S, C>::AddUnique(Point const& pt)
{
   AddUnique(Cube(pt));
}

template <class S, int C>
void Region<S, C>::AddUnique(Cube const& cube)
{
   if (cube.IsEmpty()) {
      return;
   }
   Validate();
#if REGION_PARANOIA_LEVEL > 0
   ASSERT(!Intersects(cube));
#endif

   if (cubes_.empty() || !cubes_.back().CombineWith(cube)) {
      ++_churn;
      CHURN_LOG(7) << "incremented churn in add unique";
      cubes_.push_back(cube);
   } else {
      // The cube on the back has been merged with the new cube.
      // This could enable another merge!  I could write this in
      // a tail-end recursive way to avoid duplicating code, but
      // I believe it would be ultimately much more confusing to 
      // read.
      uint c = (uint)cubes_.size();
      while (c > 1) {
         if (!cubes_[c-2].CombineWith(cubes_[c-1])) {
            break;
         }
         --_churn;
         --c;
         CHURN_LOG(7) << "decremented churn in add unique due to merge";
      }
      cubes_.resize(c);
   }

   Validate();
}

template <class S, int C>
void Region<S, C>::AddUnique(Region const& region)
{
   Validate();
   region.Validate();
   for (Cube const& cube : EachCube(region)) {
#if REGION_PARANOIA_LEVEL > 0
      ASSERT(!Intersects(cube));
#endif
      cubes_.push_back(cube);
   }
   _churn += region._churn;
   CHURN_LOG(7) << "merged other region's churn of " << region._churn << " in add unique";
   Validate();
}

template <class S, int C>
void Region<S, C>::Subtract(Point const& pt)
{
   Subtract(Cube(pt));
}

template <class S, int C>
void Region<S, C>::Subtract(Cube const& cube)
{
   CubeVector added;

   unsigned int i = 0;
   unsigned int size = (int)cubes_.size();

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
            _churn--;
            CHURN_LOG(7) << "elimininated cube in subtract";
         } else {
            cubes_[i] = replacement[0];
            added.insert(added.end(), replacement.cubes_.begin() + 1, replacement.cubes_.end());
            i++;
         }
      }
   }
   cubes_.resize(size);
   cubes_.insert(cubes_.end(), added.begin(), added.end());
   size_t addCount = added.size();
   if (addCount) {
      _churn += (int)added.size();
      CHURN_LOG(7) << "added " << added.size() << " cubes in subtract";
   }

   Validate();
}

template <class S, int C>
Region<S, C> Region<S, C>::operator+(Cube const& cube) const
{
   Region result(*this);
   result += cube;
   return result;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator+(Region const& region) const
{
   Region result(*this);
   result += region;
   return result;
}

template <class S, int C>
Region<S, C> const& Region<S, C>::operator+=(Cube const& cube)
{
   Add(cube);
   return *this;
}

template <class S, int C>
Region<S, C> const& Region<S, C>::operator+=(Region const& region)
{
   Add(region);
   return *this;
}

template <class S, int C>
void Region<S, C>::Subtract(Region const& r)
{
   for (const auto &rc : EachCube(r)) {
      Subtract(rc);
   }
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(Cube const& cube) const
{
   Region result(*this);
   result -= cube;
   return result;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator-(Region const& region) const
{
   Region result(*this);
   result -= region;
   return result;
}

template <class S, int C>
Region<S, C> const& Region<S, C>::operator-=(Cube const& cube)
{
   Subtract(cube);
   return *this;
}

template <class S, int C>
Region<S, C> const& Region<S, C>::operator-=(Region const& region)
{
   Subtract(region);
   return *this;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator&(Region const& region) const
{
   Region result = *this;
   result &= region;
   return result;
}

template <class S, int C>
Region<S, C> Region<S, C>::operator&(Cube const& cube) const
{
   Region result = *this;
   result &= cube;
   return result;
}

template <class S, int C>
Region<S, C> const& Region<S, C>::operator&=(Cube const& cube)
{
   unsigned int i = 0;
   unsigned int size = (unsigned int)cubes_.size();

   Validate();

   while (i < size) {
      Cube& src = cubes_[i];
      if (src.Intersects(cube)) {
         src.Clip(cube);
         ++i;
      } else {
         cubes_[i] = cubes_[size - 1];
         size--;
      }
   }
   cubes_.resize(size);

   Validate();

   return *this;
}


template <class S, int C>
Region<S, C> const& Region<S, C>::operator&=(Region const& other)
{
   Region result;

   Validate();

   if (cubes_.size() && other.cubes_.size()) {
      // the union of all our little rects clipped against the other region
      for (Cube const& cube : cubes_) {
         result.AddUnique(cube & other);
      }
   }

   *this = result;

   Validate();

   return *this;
}

template <class S, int C>
bool Region<S, C>::Intersects(Cube const& cube) const
{
   for (Cube const& c : cubes_) {
      if (cube.Intersects(c)) {
         return true;
      }
   }
   return false;
}

template <class S, int C>
bool Region<S, C>::Intersects(Region const& region) const
{
   for (Cube const& c : cubes_) {
      if (region.Intersects(c)) {
         return true;
      }
   }
   return false;
}

template <class S, int C>
bool Region<S, C>::Contains(Point const& pt) const
{
   for (Cube const& cube : cubes_) {
      if (cube.Contains(pt)) {
         return true;
      }
   }
   return false;
}

template <class S, int C>
Point<S, C> Region<S, C>::GetClosestPoint(Point const& from) const
{
   ASSERT(!IsEmpty());

   double bestDistance = FLT_MAX;

   Point closest;
   for (int i = 0; i < C; i++) {
      // xxx - need some kind of type traits to make this NOT SUCK
      closest[i] = (S)INT_MAX;
   }

   for (const auto &cube : cubes_) {
      Point candidate = cube.GetClosestPoint(from);
      double candidateDistance = from.SquaredDistanceTo(candidate);
      if (candidateDistance < bestDistance) {
         bestDistance = candidateDistance;
         closest = candidate;
      }
   }
   return closest;
}

template <class S, int C>
bool Region<S, C>::ContainsAtMostOneTag() const
{
   if (IsEmpty()) {
      return true;
   }

   int tag = cubes_[0].GetTag();
   unsigned int size = GetRectCount();

   for (unsigned int i = 1; i < size; i++) {
      if (cubes_[i].GetTag() != tag) {
         return false;
      }
   }
   return true;
}

template <class S, int C>
std::map<int, std::unique_ptr<Region<S, C>>> Region<S, C>::SplitByTag() const
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
void Region<S, C>::ForceOptimizeByMerge(const char* reason)
{
   CHURN_LOG(5)  << "maxing out churn to force optimize (" << reason << ")";
   _churn = INT_MAX;
   OptimizeByMerge(reason);
}

template <class S, int C>
void Region<S, C>::OptimizeByMerge(const char* reason)
{
   size_t count = cubes_.size();
   if (count <= 1) {
      return;
   }

   if (_churn < (int)count) {
      CHURN_LOG(9) << "ignoring optimize: " << reason;
      ++_churn;
      return;
   }
#if defined(REGION_PROFILE_OPTIMIZE)
   perfmon::CounterValueType start = perfmon::Timer::GetCurrentCounterValueType();
#endif

   Cube bounds = GetBounds();
   Point dimensions = bounds.GetMax() - bounds.GetMin();
   REGION_LOG(7) << "Optimizing region" << C << " by merge - size: " << dimensions;
   REGION_LOG(7) << "# cubes before optimization: " << GetCubeCount();

   std::map<int, std::unique_ptr<Region>> regions = SplitByTag();

   if (regions.size() == 1) {
      OptimizeOneTagByMerge();
   } else {
      Clear();
      for (auto const& i : regions) {
         Region& region = *i.second;
         region.OptimizeOneTagByMerge();
         AddUnique(region);
      }
   }
#if defined(REGION_PROFILE_OPTIMIZE)
   {
      tbb::spin_mutex::scoped_lock lock(__optimizeProfilerLock);
    
      perfmon::CounterValueType end = perfmon::Timer::GetCurrentCounterValueType();
      __optimizeProfiler[reason] += end - start;

      if (perfmon::CounterToMilliseconds(end - __optimizeLastReportTime) > 5000) {
         if (__optimizeLastReportTime> 0) {
            typedef std::pair<core::StaticString, perfmon::CounterValueType> ReasonTime;
            perfmon::CounterValueType maxDuration = 0;
            std::vector<ReasonTime> order;
            for (auto const &entry : __optimizeProfiler) {
               maxDuration = std::max(maxDuration, entry.second);
               order.emplace_back(entry);
            }     
            // Sort by total size 
            std::sort(order.begin(), order.end(), [](ReasonTime const& lhs, ReasonTime const& rhs) -> bool {
               return lhs.second > rhs.second;
            });

            // Write the table.
            LOG(csg.region, 0) << "--- region optimization times --------------------------------------------";
            for (auto const &o : order) {
               core::StaticString reason = o.first;
               perfmon::CounterValueType duration = o.second;

               size_t rows = duration * 30 / maxDuration;
               ASSERT(rows >= 0 && rows <= 30);
               std::string stars(rows, '#');

               LOG(csg.region, 0) << std::setw(42) << reason << " : "
                                  << std::setw(8) << perfmon::CounterToMilliseconds(duration) << " ms : "
                                  << stars;
            }
         }
         __optimizeLastReportTime= end;
      }
   }
#endif

   _churn = 0;
   CHURN_LOG(7) << "resetting churn after optimize: " << reason;
}

// assumes all cubes have the same tag
template <class S, int C>
void Region<S, C>::OptimizeOneTagByMerge()
{
   if (IsEmpty()) {
      return;
   }

   // Sorting is just a win for both algorithms.
   std::sort(cubes_.begin(), cubes_.end(), Cube::Compare());

   // Testing shows that WorkForward is always better in time and compression
   // than WorkBackward.  Consider removing WorkBackward in the future.

   if (__optimizeStrategy == WorkForward) {
      //   merged:      everything below this is fully merged.
      //   count:       number of valid cubes.  
      uint merged = 1, c = (uint)cubes_.size();
      uint start = c;

      while (merged < c) {
         ASSERT(merged >= 0);

         uint candidate = merged;
         for (uint i = 0; i < merged; i++) {
            INCREMENT_COMBINE_COUNT();
            if (cubes_[i].CombineWith(cubes_[candidate])) {
               // first of all, we don't need candidate anymore.  kill it
               cubes_[candidate] = cubes_[--c];

               // next, we've potentially ruined our invariant by modifying
               // a cube in the merged list.  the ith node becomes the new
               // candidate!
               --merged;
               std::swap(cubes_[merged], cubes_[i]);
               break;
            }
         }

         if (candidate == merged) {
            ++merged;
         }
      }
      cubes_.resize(c);
      Validate();

      CHURN_LOG(3) << "optimize reduced cubes from " << start << " to " << c;
      return;
   }

#if REGION_PARANOIA_LEVEL > 1
   ASSERT(ContainsAtMostOneTag());
#endif
   Validate();
   S areaBefore = GetArea();

   unsigned int i, j, k;
   unsigned int size = (unsigned int)cubes_.size();
   bool merged;

   i = 0;
   while (i < size-1) {
      merged = false;

      // check ith cube against all cubes > i
      j = i + 1;
      while (j < size) {
         INCREMENT_COMBINE_COUNT();
         if (cubes_[i].CombineWith(cubes_[j])) {
            cubes_[j] = cubes_[size-1];
            size--;
            // ith cube now needs to be rechecked for merge against all other cubes
            // this may recursively invoke other merge checks
            merged = true;
            break;
         } else {
            j++;
         }
      }

      if (merged) {
         // check ith cube against all cubes < i
         unsigned int subSize = i;
            
         j = 0;
         while (j < subSize) {
            INCREMENT_COMBINE_COUNT();
            if (cubes_[i].CombineWith(cubes_[j])) {
               cubes_[j] = cubes_[subSize-1];
               subSize--;
               j = 0; // have to check against the whole list again
            } else {
               j++;
            }
         }

         int const numMerged = i - subSize;

         // compact the array
         if (numMerged > 0) {
            i -= numMerged;
            size -= numMerged;

            for (k = subSize; k < size; k++) {
               cubes_[k] = cubes_[k+numMerged];
            }
         }
      } else {
         i++;
      }
   }

   cubes_.resize(size);

   S areaAfter = GetArea();
   ASSERT(areaBefore == areaAfter);
   Validate();
}

template <class S, int C>
S Region<S, C>::GetOctTreeCubeSize(Cube const& bounds) const
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

// good optimization for regions with cubes that clump together
// not so great for regions with multiple tags
template <class S, int C>
void Region<S, C>::OptimizeByOctTree(const char* reason, S minCubeSize)
{
   size_t count = cubes_.size();
   if (count <= 1) {
      return;
   }

   if (_churn < (int)count) {
      CHURN_LOG(9) << "ignoring optimize: " << reason;
      ++_churn;
      return;
   }

   if (ContainsAtMostOneTag()) {
      OptimizeOneTagByOctTree(minCubeSize);
   } else {
      REGION_LOG(1) << "WARNING: OptimizeByOctTree contains more than one tag type and may perform poorly. Consider OptimizeByMerge instead.";

      std::map<int, std::unique_ptr<Region>> regions = SplitByTag();

      Clear();

      for (auto const& i : regions) {
         Region& region = *i.second;
         region.OptimizeOneTagByOctTree(minCubeSize);
         AddUnique(region);
      }
   }
   _churn = 0;
   CHURN_LOG(7) << "resetting churn after optimize by oct tree: " << reason;
}

// assumes all cubes have the same tag
template <class S, int C>
void Region<S, C>::OptimizeOneTagByOctTree(S minCubeSize)
{
   if (IsEmpty()) {
      return;
   }
#if REGION_PARANOIA_LEVEL >= 2
   ASSERT(ContainsAtMostOneTag());
#endif
   Validate();

   S areaBefore = GetArea();

   Cube const bounds = GetBounds();
   S cubeSize = GetOctTreeCubeSize(bounds);

   if (cubeSize < minCubeSize) {
      OptimizeOneTagByMerge();
      return;
   }

   Point dimensions = bounds.GetMax() - bounds.GetMin();
   REGION_LOG(7) << "Optimizing region" << C << " by oct tree - size: " << dimensions;
   REGION_LOG(7) << "OctTree max cube size: " << cubeSize;
   REGION_LOG(7) << "# cubes before optimization: " << GetCubeCount();

   Point min, max;

   for (int d = 0; d < C; d++) {
      // floor quantizes towards -infinity which is what we want
      min[d] = static_cast<S>(std::floor((double)bounds.GetMin()[d] / cubeSize) * cubeSize);
      // max[d] is an open interval and not included in the set
      max[d] = static_cast<S>(std::ceil((double)bounds.GetMax()[d] / cubeSize) * cubeSize);
   }

   Cube quantizedBounds = Cube(min, max);
   OptimizeOctTreeImpl(quantizedBounds, cubeSize, minCubeSize);
   REGION_LOG(7) << "# cubes after optimization phase 1: " << GetCubeCount();

   OptimizeOneTagByMerge();
   REGION_LOG(7) << "# cubes after optimization phase 2: " << GetCubeCount();

   S areaAfter = GetArea();
   ASSERT(areaBefore == areaAfter);
   Validate();
}

// Assumes all cubes have the same tag.
// Top level bounds does not have to be equal length in all dimensions.
// Will tile the top level bounds into cubes using partitionSize.
// Subsequent calls will have regions that are actually square cubes.
// We also explicitly pass in the bounds so we don't have to call O(n) Region::GetBounds().
template <class S, int C>
void Region<S, C>::OptimizeOctTreeImpl(Cube const& bounds, S partitionSize, S minCubeSize)
{
   // merging is optimal for <= 5 rects and <= 7? cubes
   static const int optimalMergeCount[] = { 5, 5, 7 };

   if (IsEmpty()) {
      return;
   }

   Point min = bounds.GetMin();
   Point max = bounds.GetMax();

   if (GetArea() == bounds.GetArea()) {
      // Bingo - found a filled region, replace it all with one cube.
      // This is the what this algorithm is designed to optimize.
      cubes_.resize(1);
      cubes_[0].min = min;
      cubes_[0].max = max;
      return;
   }

   if (GetCubeCount() <= optimalMergeCount[C] * 4) {
      OptimizeOneTagByMerge();
      return;
   }

   if (partitionSize < minCubeSize) {
      OptimizeOneTagByMerge();
      return;
   }

   Region intersection, optimized;
   Cube childBounds;
   S const childPartitionSize = partitionSize / 2;
   Point pt1, pt2;
   int d;

   pt1 = min;

   while (true) {
      for (d = 0; d < C; d++) {
         pt2[d] = pt1[d] + partitionSize;
      }

      childBounds = Cube(pt1, pt2);
      intersection = *this & childBounds;
      *this -= childBounds;

      intersection.OptimizeOctTreeImpl(childBounds, childPartitionSize, minCubeSize);

      optimized.AddUnique(intersection);

      // max[d] is an open interval and not included in the set
      for (d = 0; d < C; d++) {
         pt1[d] += partitionSize;

         if (pt1[d] < max[d]) {
            break;
         } else {
            pt1[d] = min[d];
         }
      }

      if (d == C) {
         break;
      }
   }

   *this = optimized;
}

template <class S, int C>
Cube<S, C> Region<S, C>::GetBounds() const
{
   if (IsEmpty()) {
      return Cube::zero;
   }

   Cube bounds = cubes_[0];
   int i, c = (int)cubes_.size();
   for (i = 1; i < c; i++) {
      bounds.Grow(cubes_[i].GetMin());
      bounds.Grow(cubes_[i].GetMax());
   }
   return bounds;
}

template <class S, int C>
void Region<S, C>::Translate(Point const& pt)
{
   for (auto &c : cubes_) {
      c.Translate(pt);
   }
}

template <class S, int C>
Region<S, C> Region<S, C>::Translated(Point const& pt) const
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

template <class S, int C>
void Region<S, C>::SetTag(int tag)
{
   for (auto &c : cubes_) {
      c.SetTag(tag);
   }
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
Region<double, C> csg::ToFloat(Region<int, C> const& region) {
   Region<double, C> result;
   for (Cube<int, C> const& cube : EachCube(region)) {
      result.AddUnique(ToFloat(cube));
   }
   return result;
}

template <int C>
Region<double, C> const& csg::ToFloat(Region<double, C> const& region) {
   return region;
}

#define FAST_COPY(c)                   \
   integer = static_cast<int>(in.c);         \
   if (in.c - integer != 0.0) return false;  \
   out.c = integer;

template <int C>
static inline bool FastToInt(Cube<double, C> const& in, Cube<int, C>& out);

template <>
inline bool FastToInt(Cube<double, 1> const& in, Cube<int, 1>& out)
{
   int integer;
   FAST_COPY(min.x);
   FAST_COPY(max.x);
   out.SetTag(in.GetTag());
   return true;
}

template <>
inline bool FastToInt(Cube<double, 2> const& in, Cube<int, 2>& out)
{
   int integer;
   FAST_COPY(min.x);
   FAST_COPY(max.x);
   FAST_COPY(min.y);
   FAST_COPY(max.y);
   out.SetTag(in.GetTag());
   return true;
}

template <>
inline bool FastToInt(Cube<double, 3> const& in, Cube<int, 3>& out)
{
   int integer;
   FAST_COPY(min.x);
   FAST_COPY(max.x);
   FAST_COPY(min.z);
   FAST_COPY(max.z);
   FAST_COPY(min.y);
   FAST_COPY(max.y);
   out.SetTag(in.GetTag());
   return true;
}

template <int C>
Region<int, C> csg::ToInt(Region<double, C> const& region) {
   Region<int, C> result;
   
   Region<double, C>::CubeVector const& cubes = region.GetContents();
   uint i = 0, c = (uint)cubes.size();

   while (i < c) {
      Cube<int, C> icube;
      if (!FastToInt(cubes[i], icube)) {
         break;
      }
      result.AddUnique(icube);
      ++i;
   }
   while (i < c) {
      result.Add(ToInt(cubes[i])); // so expensive!
      ++i;
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
#if REGION_PARANOIA_LEVEL > 1
   uint i, j, c = cubes_.size();
   for (i = 0; i < c; i++) {
      ASSERT(!cubes_[i].IsEmpty());
      for (j = 0; j < i; j++) {
         if (cubes_[i].Intersects(cubes_[j])) {
            LOG(csg.region, 0) << "cube " << j << ":" << cubes_[i] << " overlaps with " << i << ":" << cubes_[i];
         }
         ASSERT(!cubes_[i].Intersects(cubes_[j]));
      }
   }
#endif
}

template <typename S, int C>
Point<double, C> csg::GetCentroid(Region<S, C> const& region)
{
   Point<double, C> weightedSum = Point<double, C>::zero;
   double totalWeight = 0;

   for (Cube<S, C> const& cube : EachCube(region)) {
      Point<double, C> cubeCentroid = GetCentroid(cube);
      double cubeArea = (double)cube.GetArea();
      weightedSum += cubeCentroid.Scaled(cubeArea);
      totalWeight += cubeArea;
   }

   Point<double, C> centroid = weightedSum.Scaled(1 / totalWeight);
   return centroid;
}

#define MAKE_REGION(Cls) \
   const Cls Cls::zero; \
   template Cls::Region(); \
   template Cls::Region(const Cls::Cube&); \
   template Cls::ScalarType Cls::GetArea() const; \
   template void Cls::Clear(); \
   template void Cls::SetTag(int tag); \
   template void Cls::Add(const Cls&); \
   template void Cls::Add(const Cls::Cube&); \
   template void Cls::Add(const Cls::Point&); \
   template void Cls::AddUnique(const Cls::Point&); \
   template void Cls::AddUnique(const Cls::Cube&); \
   template void Cls::AddUnique(const Cls&); \
   template void Cls::Subtract(const Cls&); \
   template void Cls::Subtract(const Cls::Cube&); \
   template void Cls::Subtract(const Cls::Point&); \
   template Cls Cls::operator+(const Cls&) const; \
   template Cls Cls::operator+(const Cls::Cube&) const; \
   template Cls Cls::operator-(const Cls&) const; \
   template Cls Cls::operator-(const Cls::Cube&) const; \
   template Cls Cls::operator&(const Cls&) const; \
   template Cls Cls::operator&(const Cls::Cube&) const; \
   template const Cls& Cls::operator+=(const Cls&); \
   template const Cls& Cls::operator+=(const Cls::Cube&); \
   template const Cls& Cls::operator-=(const Cls&); \
   template const Cls& Cls::operator-=(const Cls::Cube&); \
   template const Cls& Cls::operator&=(const Cls&); \
   template const Cls& Cls::operator&=(const Cls::Cube&); \
   template bool Cls::Intersects(const Cls::Cube&) const; \
   template bool Cls::Intersects(const Cls&) const; \
   template bool Cls::Contains(const Cls::Point&) const; \
   template Cls::Point Cls::GetClosestPoint(const Cls::Point&) const; \
   template void Cls::OptimizeByMerge(const char*); \
   template void Cls::OptimizeByOctTree(const char*, Cls::ScalarType); \
   template void Cls::ForceOptimizeByMerge(const char*); \
   template Cls::Cube Cls::GetBounds() const; \
   template void Cls::Translate(const Cls::Point& pt); \
   template Cls Cls::Translated(const Cls::Point& pt) const; \
   template Cls Cls::Inflated(const Cls::Point& pt) const; \
   template void Cls::SetOptimizeStrategy(OptimizeStrategy s); \
   Cls::OptimizeStrategy Cls::__optimizeStrategy = Cls::OptimizeStrategy::WorkForward; \

MAKE_REGION(Region3)
MAKE_REGION(Region3f)
MAKE_REGION(Region2)
MAKE_REGION(Region2f)
MAKE_REGION(Region1)
MAKE_REGION(Region1f)

#define DEFINE_REGION_CONVERSIONS(C) \
   template Region<double, C> csg::ToFloat(Region<int, C> const&); \
   template Region<double, C> const& csg::ToFloat(Region<double, C> const&); \
   template Region<int, C> const& csg::ToInt(Region<int, C> const&); \
   template Region<int, C> csg::ToInt(Region<double, C> const&); \

DEFINE_REGION_CONVERSIONS(1)
DEFINE_REGION_CONVERSIONS(2)
DEFINE_REGION_CONVERSIONS(3)

// define centroid methods
template Point2f csg::GetCentroid(Region2 const& region);
template Point3f csg::GetCentroid(Region3 const& region);
template Point2f csg::GetCentroid(Region2f const& region);
template Point3f csg::GetCentroid(Region3f const& region);
