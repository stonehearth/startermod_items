#ifndef _RADIANT_CSG_REGION_H
#define _RADIANT_CSG_REGION_H

#include <vector>
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
   Region(const Cube& cube);
   Region(const Region&& r);
   Region(protocol::region3i const& msg) { LoadValue(msg); }

   static const Region empty;
  
   const CubeVector& GetContents() const { return cubes_; }
   typename CubeVector::const_iterator begin() const { return cubes_.begin(); }
   typename CubeVector::const_iterator end() const { return cubes_.end(); }

public:
   S GetArea() const;
   bool IsEmpty() const;
   bool Intersects(const Cube& cube) const;
   bool Contains(const Point& pt) const;
   
   Point GetClosestPoint2(const Point& src, S *dSquared) const;
   Point GetClosestPoint(const Point& src) const { return GetClosestPoint2(src, nullptr); }
   Cube GetBounds() const;
   int GetRectCount() const { return cubes_.size(); }
   Cube GetRect(int i) const { return cubes_[i]; }

   int GetCubeCount() const { return cubes_.size(); }
   const Cube& operator[](int i) const { return cubes_[i]; }
   void Translate(const Point& pt);
   Region Translated(const Point& pt) const;
   Region Inflated(Point const& pt) const;

   // non-optimizing... (xxx: make regions fluent!)
   void Clear();
   void Add(const Region& cube);
   void Add(Cube const& cube);
   void Add(const Point& cube);
   void AddUnique(const Cube& cube);
   void AddUnique(const Region& cube);  
   void Subtract(const Point& other);
   void Subtract(const Cube& other);
   void Subtract(const Region& other);

   // optimizing...
   Region<S, C> operator-(const Cube& other) const;
   Region<S, C> operator-(const Region& other) const;
   Region<S, C> operator&(const Cube& region) const;
   Region<S, C> operator&(const Region& region) const;
   const Region<S, C>& operator+=(const Region& region);
   const Region<S, C>& operator-=(const Region& region);
   const Region<S, C>& operator&=(const Cube& cube);
   const Region<S, C>& operator&=(const Region& other);
   const Region<S, C>& operator+=(const Cube& cube);
   const Region<S, C>& operator-=(const Cube& cube);
   const Region<S, C>& operator+=(const Point& pt) { return operator+=(Cube(pt)); }
   const Region<S, C>& operator-=(const Point& pt) { return operator-=(Cube(pt)); }

private:
   void Validate() const;

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

   void Optimize();

private:
   CubeVector     cubes_;
};

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, const Region<S, C>& o)
{
   os << "(" << o.GetCubeCount() << " cubes of area " << o.GetArea() << ")";
   return os;
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_REGION_H
