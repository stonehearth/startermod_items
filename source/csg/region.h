#ifndef _RADIANT_CSG_REGION_H
#define _RADIANT_CSG_REGION_H

#include <vector>
#include "cube.h"

struct lua_State;
namespace luabind { 
   struct scope; 
}

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
   Region(const protocol::region& msg) { LoadValue(msg); }

   static const Region empty;
   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
  
   const CubeVector& GetContents() const { return cubes_; }
   typename CubeVector::const_iterator begin() const { return cubes_.begin(); }
   typename CubeVector::const_iterator end() const { return cubes_.end(); }

public:
   S GetArea() const;
   bool IsEmpty() const { return GetArea() == 0; }
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

   // util
   Region ProjectOnto(int axis, S plane) const;

   // non-optimizing...
   void Clear();
   void Add(const Cube& cube);
   void AddUnique(const Cube& cube);
   void AddUnique(const Region& cube);
   
   // optimizing...
   Region<S, C> operator-(const Cube& other) const;
   Region<S, C> operator-(const Region& other) const;
   const Region<S, C>& operator=(const Region& other);
   const Region<S, C>& operator+=(const Region& cube);
   const Region<S, C>& operator-=(const Region& cube);
   const Region<S, C>& operator&=(const Cube& cube);
   const Region<S, C>& operator&=(const Region& other);
   const Region<S, C>& operator+=(const Cube& cube);
   const Region<S, C>& operator-=(const Cube& cube);
   const Region<S, C>& operator+=(const Point& pt) { return operator+=(Cube(pt)); }
   const Region<S, C>& operator-=(const Point& pt) { return operator-=(Cube(pt)); }

   std::ostream& Format(std::ostream& os) const {
      os << "( " << cubes_.size() << " cubes of area " << GetArea() << ")";
      return os;
   }

public:
   void SaveValue(protocol::region* msg) const {
      for (const auto& c : cubes_) {
         c.SaveValue(msg->add_cubes());
      }
   }
   void LoadValue(const protocol::region& msg) {
      cubes_.clear();
      for (const auto &c : msg.cubes()) {
         cubes_.push_back(Cube(c));
      }
   }

   void Optimize();

private:
   void Subtract(const Cube& other);
   void Subtract(const Region& other);

private:
   CubeVector     cubes_;
};

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, const Region<S, C>& in)
{
   return in.Format(os);
}

typedef Region<int, 1>   Region1;
typedef Region<int, 2>   Region2;
typedef Region<int, 3>   Region3;
typedef Region<float, 3> Region3f;

bool Region3Intersects(const Region3& rgn, const math3d::ray3& ray, float& distance);

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Region3, Protocol::region)
IMPLEMENT_DM_EXTENSION(::radiant::csg::Region2, Protocol::region)

#endif // _RADIANT_CSG_REGION_H
