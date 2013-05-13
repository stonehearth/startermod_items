#ifndef _RADIANT_CSG_POINT_H
#define _RADIANT_CSG_POINT_H

#include <ostream>
#include "namespace.h"
#include "math3d.h"
#include "dm/dm.h"

struct lua_State;
namespace luabind { 
   struct scope; 
}

BEGIN_RADIANT_CSG_NAMESPACE

template <typename S, int C, typename Derived>
class PointBase
{
public:
   PointBase() { }

   S& operator[](int offset) { return static_cast<Derived*>(this)->Coord(offset); }
   S operator[](int offset) const { return static_cast<const Derived*>(this)->Coord(offset);  }

   // manipulators
   Derived Scale(S s) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] * s;
      }
      return result;
   }

   // output
   std::ostream& Print(std::ostream& os) const {
      os << "(";
      for (int i = 0; i < C; i++) {
         os << (*this)[i];
         if (i < (C-1)) {
            os << ", ";
         }
      }
      os << ")";
      return os;
   }

   // utilty
   static Derived MidPoint(const Derived& a, const Derived& b) {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (a[i] + b[i]) / 2;
      }
      return result;
   }

   Derived ProjectOnto(int axis, S plane) const
   {
      Derived result = *static_cast<const Derived*>(this);
      result[axis] = plane;
      return result;
   }

   struct Hash { 
      size_t operator()(const Derived& o) const {
         size_t result = 0;
         for (int i = 0; i < C; i++) {
            result ^= std::hash<int>()(o[i]);
         }
         return result;
      }
   };


   Derived operator*(S amount) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] * amount;
      }
      return result;
   }
   const Derived& operator+=(const Derived& other) {
      for (int i = 0; i < C; i++) {
         (*this)[i] += other[i];
      }
      return static_cast<const Derived&>(*this);
   }

   const Derived& operator-=(const Derived& other) {
      for (int i = 0; i < C; i++) {
         (*this)[i] -= other[i];
      }
      return static_cast<const Derived&>(*this);
   }

   Derived operator+(const Derived& other) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] + other[i];
      }
      return result;
   }

   template <class U> void Translate(const U& pt) {
      for (int i = 0; i < C; i++) {
         (*this)[i] += static_cast<S>(pt[i]);
      }
   }

   Derived operator-(const Derived& other) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] - other[i];
      }
      return result;
   }

   Derived operator-() const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = -(*this)[i];
      }
      return result;
   }

   bool operator==(const Derived& other) const {
      for (int i = 0; i < C; i++) {
         if ((*this)[i] != other[i]) {
            return false;
         }
      }
      return true;
   }

   bool operator!=(const Derived& other) const {
      return !((*this) == other);
   }

   bool operator<(const Derived& other) const {
      for (int i = 0; i < C; i++) {
         S lhs = (*this)[i], rhs = other[i];
         if (lhs < rhs) {
            return true;
         }
         if (lhs != rhs) {
            break;
         }
      }
      return false;
   }

public:
   void SaveValue(protocol::point* msg) const {
      for (int i = 0; i < C; i++) {
         // xxx: this doesn't work at all for floats.  luckily we
         // never, ever remote them.  fix this by moving the protobuf
         // stuff into helpers instead of this LoadValue/SaveValue crap
         msg->add_coord((S)(*this)[i]);
      }
   }
   void LoadValue(const protocol::point& msg) {
      for (int i = 0; i < C; i++) {
         // xxx: this doesn't work at all for floats.  luckily we
         // never, ever remote them.  fix this by moving the protobuf
         // stuff into helpers instead of this LoadValue/SaveValue crap
         (*this)[i] = (S)msg.coord(i);
      }
   }
};

template <typename S, int C>
class Point
{
};

template <typename S>
class Point<S, 1> : public PointBase<S, 2, Point<S, 1>>
{
public:
   Point() { }
   Point(S x) : x(x) { }
   Point(const protocol::point& msg) { LoadValue(msg); }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   static Point zero;
   static Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

public:
   S x;
};


template <typename S>
class Point<S, 2> : public PointBase<S, 2, Point<S, 2>>
{
public:
   Point() { }
   Point(S x, S y) : x(x), y(y) { }
   Point(const protocol::point& msg) { LoadValue(msg); }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   static Point zero;
   static Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

public:
   S x, y;
};

template <typename S>
class Point<S, 3> : public PointBase<S, 3, Point<S, 3>>
{
public:
   Point() { }
   Point(S x, S y, S z) : x(x), y(y), z(z) { }
   Point(const protocol::point& msg) { LoadValue(msg); }

   static luabind::scope RegisterLuaType(struct lua_State* L, const char* name);
   static Point zero;
   static Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

   // nuke these...
   Point(const math3d::ipoint3& pt) : x(pt.x), y(pt.y), z (pt.z) { }
   Point(const math3d::point3& pt) { *this = math3d::ipoint3(pt); }
   operator math3d::ipoint3() const { return math3d::ipoint3(x, y, z); }
   operator math3d::point3() const { return math3d::point3(x, y, z); }

   // operators
   Point operator/(S amount) const { return Point(x / amount, y / amount, z  / amount); }

public:
   S x, y, z;
};

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, const Point<S, C>& in)
{
   return in.Print(os);
}

typedef Point<int, 1>      Point1;
typedef Point<int, 2>      Point2;
typedef Point<int, 3>      Point3;
typedef Point<float, 2>    Point2f;
typedef Point<float, 3>    Point3f;

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Point2, Protocol::point)
IMPLEMENT_DM_EXTENSION(::radiant::csg::Point3, Protocol::point)

#endif // _RADIANT_CSG_POINT_H