#ifndef _RADIANT_CSG_REGION_H
#define _RADIANT_CSG_REGION_H

#include <vector>
#include <map>
#include <EASTL/vector.h>
#include "cube.h"

// eastl vectors behave just like std::vector, but they don't allocate until
// used.  this is a big win!
#define EASTL_REGIONS

//
// (note: used only in the non-EASTL_REGIONS case.)
// Emperical evidence suggests that without prior knowledge of how big a region
// is going to be, reserving capacity does not actually improve CPU performance.
// Dropping this value from 64 to 0 reduces the commited memory from ~370MB to
// ~120MB in my testing of starting random worlds.  Perhaps this will change when
// we more efficiently share regions, but for now lets go with it.
//
#define INITIAL_CUBE_SPACE    2

BEGIN_RADIANT_CSG_NAMESPACE

template <class S, int C>
class Region
{
public:
   typedef Cube<S, C> Cube;
   typedef Point<S, C> Point;
#if !defined(EASTL_REGIONS)
   typedef std::vector<Cube> CubeVector;
#else
   typedef eastl::vector<Cube> CubeVector;
#endif
   enum { Dimension = C };
   typedef S ScalarType;

public:
   Region();
   Region(Cube const& cube);
   Region(Region const&& r);

   static const Region zero; 
   const CubeVector& GetContents() const { return cubes_; }

public:
   S GetArea() const;
   bool IsEmpty() const;
   bool Intersects(Cube const& cube) const;
   bool Contains(Point const& pt) const;

   Point GetClosestPoint(Point const& src) const;
   Cube GetBounds() const;
   Point GetCentroid() const;
   int GetRectCount() const { return cubes_.size(); }
   Cube GetRect(int i) const { return cubes_[i]; }

   int GetCubeCount() const { return cubes_.size(); }
   Cube const& operator[](int i) const { return cubes_[i]; }
   void Translate(Point const& pt);
   Region Translated(Point const& pt) const;
   Region Inflated(Point const& pt) const;

   // xxx: make regions fluent!
   void Clear();
   void Add(Point const& point);
   void Add(Cube const& cube);
   void Add(Region const& region);
   void AddUnique(Point const& cube);
   void AddUnique(Cube const& cube);
   void AddUnique(Region const& region);  
   void Subtract(Point const& point);
   void Subtract(Cube const& cube);
   void Subtract(Region const& region);
   void SetTag(int tag);

   Region<S, C> operator-(Cube const& cube) const;
   Region<S, C> operator-(Region const& region) const;
   Region<S, C> operator&(Cube const& cube) const;
   Region<S, C> operator&(Region const& region) const;
   Region<S, C> const& operator+=(Region const& region);
   Region<S, C> const& operator-=(Region const& region);
   Region<S, C> const& operator&=(Cube const& cube);
   Region<S, C> const& operator&=(Region const& region);
   Region<S, C> const& operator+=(Cube const& cube);
   Region<S, C> const& operator-=(Cube const& cube);
   Region<S, C> const& operator+=(Point const& pt) { return operator+=(Cube(pt)); }
   Region<S, C> const& operator-=(Point const& pt) { return operator-=(Cube(pt)); }

private:
   void Validate() const;
   bool ContainsAtMostOneTag() const;
   std::map<int, std::unique_ptr<Region<S, C>>> SplitByTag() const;
   S GetOctTreeCubeSize(Cube const& bounds) const;
   void OptimizeOneTagByOctTree(S minCubeSize);
   void OptimizeOctTreeImpl(Cube const& bounds, S partitionSize, S minCubeSize);
   void OptimizeOneTagByMerge();

public:
   template <class T> void SaveValue(T* msg) const {
      for (const auto& c : cubes_) {
         c.SaveValue(msg->add_cubes());
      }
   }
   template <class T> void LoadValue(const T& msg) {
      cubes_.clear();
      for (const auto &c : msg.cubes()) {
         cubes_.push_back(Cube());
         cubes_.back().LoadValue(c);
      }
   }

   void OptimizeByOctTree(S minCubeSize);
   void OptimizeByMerge();

private:
   CubeVector     cubes_;
};

template <typename S, int C>
Point<double, C> GetCentroid(Region<S, C> const& region);

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, Region<S, C> const& o)
{
   os << "(" << o.GetCubeCount() << " cubes of area " << o.GetArea() << ")";
   return os;
}


#define DECLARE_CUBE_ITERATOR(T) \
class T ## CubeRange \
{ \
public: \
   T ## CubeRange(T const& container) : _container(container) { } \
 \
   T::CubeVector::const_iterator begin() const { return _container.GetContents().begin(); } \
   T::CubeVector::const_iterator end() const { return _container.GetContents().end(); } \
 \
private: \
   T const& _container; \
}; \
 \
static inline T ## CubeRange EachCube(T const& cube) \
{ \
   return T ## CubeRange(cube); \
} \

DECLARE_CUBE_ITERATOR(Region1)
DECLARE_CUBE_ITERATOR(Region1f)
DECLARE_CUBE_ITERATOR(Region2)
DECLARE_CUBE_ITERATOR(Region2f)
DECLARE_CUBE_ITERATOR(Region3)
DECLARE_CUBE_ITERATOR(Region3f)

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_H
