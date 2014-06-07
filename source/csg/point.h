#ifndef _RADIANT_CSG_POINT_H
#define _RADIANT_CSG_POINT_H

#include <ostream>
#include "csg.h"
#include "dm/dm.h"
#include "namespace.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <typename S, int C, typename Derived>
class PointBase
{
public:
   PointBase() { }

   typedef S Scalar;
   static const int Dimension;

   S& operator[](int offset) { return static_cast<Derived*>(this)->Coord(offset); }
   S operator[](int offset) const { return static_cast<const Derived*>(this)->Coord(offset);  }

   // manipulators
   void Scale(float s) {
      for (int i = 0; i < C; i++) {
         (*this)[i] = static_cast<S>((*this)[i] * s);
      }
   }

   Derived Scaled(float s) const {
      Derived result(*static_cast<Derived const*>(this));
      result.Scale(s);
      return result;
   }

   void Translate(const Derived& pt) {
      for (int i = 0; i < C; i++) {
         (*this)[i] += pt[i];
      }
   }

   Derived Translated(const Derived& pt) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i];
      }
      result.Translate(pt);
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

   struct Hash { 
      size_t operator()(const Derived& o) const {
         size_t result = 0;
         for (int i = 0; i < C; i++) {
            result ^= std::hash<int>()(o[i]);
         }
         return result;
      }
   };

   void SetZero() { 
      for (int i = 0; i < C; i++) {
         (*this)[i] = 0;
      }
   }

   S Dot(Derived const& other) const {
      S result = 0;
      for (int i = 0; i < C; i++) {
         result += (*this)[i] * other[i];
      }
      return result;
   }

   float LengthSquared() const {
      float result = 0;
      for (int i = 0; i < C; i++) {
         result += (*this)[i] * (*this)[i];
      }
      return result;
   }

   float Length() const {
      return csg::Sqrt(LengthSquared());
   }

   float DistanceTo(Derived const& other) const {
      float l2 = ((*this) - other).LengthSquared();
      return csg::Sqrt(l2);
   }

   void Normalize()
   {
      float lengthsq = LengthSquared();
      if (IsZero(lengthsq)) {
         SetZero();
      } else {
         Scale(InvSqrt(lengthsq));
      }
   }

   Derived operator*(S amount) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] * amount;
      }
      return result;
   }

   // dot product
   Derived operator*(Derived const& rhs) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] * rhs[i];
      }
      return result;
   }

   const Derived& operator*=(S scale) {
      for (int i = 0; i < C; i++) {
         (*this)[i] *= scale;
      }
      return static_cast<const Derived&>(*this);
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

   bool operator>(const Derived& other) const {
      return other < (*this);
   }
};

template <typename S, int C>
class Point
{
};

template <typename S>
class Point<S, 1> : public PointBase<S, 1, Point<S, 1>>
{
public:
   Point() { }
   Point(S x) : x(x) { }

   static const Point zero;
   static const Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

public:
   template <class T> void SaveValue(T* msg) const {
      msg->set_x(x);
   }
   template <class T> void LoadValue(const T& msg) {
      x = msg.x();
   }

public:
   S x;
};


template <typename S>
class Point<S, 2> : public PointBase<S, 2, Point<S, 2>>
{
public:
   Point() { }
   Point(S x, S y) : x(x), y(y) { }

   static const Point zero;
   static const Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

public:
   template <class T> void SaveValue(T* msg) const {
      msg->set_x(x);
      msg->set_y(y);
   }
   template <class T> void LoadValue(const T& msg) {
      x = msg.x();
      y = msg.y();
   }

public:
   S x, y;
};

template <typename S>
class Point<S, 3> : public PointBase<S, 3, Point<S, 3>>
{
public:
   Point() { }
   Point(S x, S y, S z) : x(x), y(y), z(z) { }
   Point(protocol::point3f const& msg) { LoadValue(msg); }

   static const Point zero;
   static const Point one;
   static const Point unitX;
   static const Point unitY;
   static const Point unitZ;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

   Point Cross(Point const& other) const
   {
      return Point(y*other.z - z*other.y,
                   z*other.x - x*other.z,
                   x*other.y - y*other.x);
   }

   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount, y / amount, z  / amount);
   }

public:
   template <class T> void SaveValue(T* msg) const {
      msg->set_x(x);
      msg->set_y(y);
      msg->set_z(z);
   }
   template <class T> void LoadValue(const T& msg) {
      x = msg.x();
      y = msg.y();
      z = msg.z();
   }

public:
   S x, y, z;
};

template <typename S>
class Point<S, 4> : public PointBase<S, 4, Point<S, 4>>
{
public:
   Point() { }
   Point(S x, S y, S z, S w) : x(x), y(y), z(z), w(w) { }

   static const Point zero;
   static const Point one;

   S Coord(int i) const { return (&x)[i]; }
   S& Coord(int i) { return (&x)[i]; }

   // operators
   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount, y / amount, z  / amount, w / amount);
   }

public:
   template <class T> void SaveValue(T* msg) const {
      msg->set_x(x);
      msg->set_y(y);
      msg->set_z(z);
      msg->set_w(w);
   }
   template <class T> void LoadValue(const T& msg) {
      x = msg.x();
      y = msg.y();
      z = msg.z();
      w = msg.w();
   }

public:
   S x, y, z, w;
};

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, Point<S, C> const& in)
{
   return in.Print(os);
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_POINT_H