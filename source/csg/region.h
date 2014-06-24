#ifndef _RADIANT_CSG_REGION_H
#define _RADIANT_CSG_REGION_H

#include <vector>
#include <map>
#include "cube.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <class S, int C>
class Region
{
public:
   typedef Cube<S, C> Cube;
   typedef Point<S, C> Point;
   typedef std::vector<Cube> CubeVector;
   typedef S ScalarType;

public:
   Region();
   Region(Cube const& cube);
   Region(Region const&& r);
   Region(protocol::region3i const& msg) { LoadValue(msg); }

   static const Region empty;
  
   const CubeVector& GetContents() const { return cubes_; }
   typename CubeVector::const_iterator begin() const { return cubes_.begin(); }
   typename CubeVector::const_iterator end() const { return cubes_.end(); }

public:
   S GetArea() const;
   bool IsEmpty() const;
   bool Intersects(Cube const& cube) const;
   bool Contains(Point const& pt) const;
   
   Point GetClosestPoint2(Point const& src, S *dSquared) const;
   Point GetClosestPoint(Point const& src) const { return GetClosestPoint2(src, nullptr); }
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
Point<float, C> GetCentroid(Region<S, C> const& region);

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, Region<S, C> const& o)
{
   os << "(" << o.GetCubeCount() << " cubes of area " << o.GetArea() << ")";
   return os;
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_H
