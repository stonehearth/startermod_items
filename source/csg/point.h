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

   typedef S ScalarType;
   enum { Dimension = C };

   S& operator[](int offset) {
      ASSERT(offset >= 0 && offset < C);
      Derived *obj = static_cast<Derived*>(this);
      return reinterpret_cast<S*>(obj)[offset];
   }

   S operator[](int offset) const {
      ASSERT(offset >= 0 && offset < C);
      Derived const *obj = static_cast<Derived const*>(this);
      return reinterpret_cast<S const*>(obj)[offset];
   }

   // manipulators
   Derived Scaled(float s) const {
      Derived result(*static_cast<Derived const*>(this));
      result.Scale(s);
      return result;
   }

   Derived Translated(const Derived& pt) const {
      Derived result(*static_cast<Derived const*>(this));
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
   private:
      template <typename S> inline std::size_t hash(S val, int i) const {
         return std::hash<S>()(val) << i;
      }

      // std::hash<int> on Windows is expensive.  we're cheap!
      template <> inline std::size_t hash(int val, int i) const {
         static const int factors[] = { 73856093, 19349663, 83492791 };
         //ASSERT(i < ARRAY_SIZE(factors));
         return factors[i] * val;
      }

   public:
      inline size_t operator()(const Derived& o) const {
         size_t result = 0;
         for (int i = 0; i < C; i++) {
            result ^= hash(o[i], i);
         }
         return result;
      }
   };

   float Length() const {
      return csg::Sqrt(static_cast<Derived const&>(*this).LengthSquared());
   }

   float DistanceTo(Derived const& other) const {
      float l2 = (static_cast<Derived const&>(*this) - other).LengthSquared();
      return csg::Sqrt(l2);
   }

   inline float SquaredDistanceTo(Derived const& other) const {
      return (static_cast<Derived const&>(*this) - other).LengthSquared();
   }

   void Normalize()
   {
      float lengthsq = static_cast<Derived const&>(*this).LengthSquared();
      if (IsZero(lengthsq)) {
         static_cast<Derived&>(*this).SetZero();
      } else {
         static_cast<Derived&>(*this).Scale(InvSqrt(lengthsq));
      }
   }

   Derived operator*(S amount) const {
      Derived result;
      for (int i = 0; i < C; i++) {
         result[i] = (*this)[i] * amount;
      }
      return result;
   }

   bool operator!=(const Derived& other) const {
      return !(static_cast<Derived const&>(*this) == other);
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
      return other < (*this);u
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

   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount);
   }

   Point operator*(S amount) const {
      return Point(x * amount);
   }

   bool operator==(Point const& other) const {
      return x == other.x;
   }

   const Point& operator*=(S scale) {
      x *= scale;
      return *this;
   }

   const Point& operator+=(const Point& other) {
      x += other.x;
      return *this;
   }

   const Point& operator-=(const Point& other) {
      x -= other.x;
      return *this;
   }

   Point operator*(const Point& other) const {
      return Point(x * other.x);
   }

   Point operator+(const Point& other) const {
      return Point(x + other.x);
   }

   Point operator-(const Point& other) const {
      return Point(x - other.x);
   }

   Point operator-() const {
      return Point(-x);
   }

   void SetZero() { 
      x = 0;
   }

   float LengthSquared() const {
      float result = 0;
      result += x * x;
      return result;
   }

   void Scale(float s) {
      x = static_cast<S>(x * s);
   }

   void Translate(const Point& pt) {
      x += pt.x;
   }

   S Dot(Point const& other) const {
      S result = 0;
      result += x * other.x;
      return result;
   }
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


   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount, y / amount);
   }

   Point operator*(S amount) const {
      return Point(x * amount, y * amount);
   }

   bool operator==(Point const& other) const {
      return x == other.x && y == other.y;
   }

   const Point& operator*=(S scale) {
      x *= scale;
      y *= scale;
      return *this;
   }

   const Point& operator+=(const Point& other) {
      x += other.x;
      y += other.y;
      return *this;
   }

   const Point& operator-=(const Point& other) {
      x -= other.x;
      y -= other.y;
      return *this;
   }

   Point operator*(const Point& other) const {
      return Point(x * other.x, y * other.y);
   }

   Point operator+(const Point& other) const {
      return Point(x + other.x, y + other.y);
   }

   Point operator-(const Point& other) const {
      return Point(x - other.x, y - other.y);
   }

   Point operator-() const {
      return Point(-x, -y);
   }

   void SetZero() { 
      x = 0;
      y = 0;
   }

   float LengthSquared() const {
      float result = 0;
      result += x * x;
      result += y * y;
      return result;
   }

   void Scale(float s) {
      x = static_cast<S>(x * s);
      y = static_cast<S>(y * s);
   }

   void Translate(const Point& pt) {
      x += pt.x;
      y += pt.y;
   }

   S Dot(Point const& other) const {
      S result = 0;
      result += x * other.x;
      result += y * other.y;
      return result;
   }

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

   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount, y / amount, z  / amount);
   }

   Point operator*(S amount) const {
      return Point(x * amount, y * amount, z * amount);
   }

   bool operator==(Point const& other) const {
      return x == other.x && y == other.y && z == other.z;
   }

   const Point& operator*=(S scale) {
      x *= scale;
      y *= scale;
      z *= scale;
      return *this;
   }

   const Point& operator+=(const Point& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      return *this;
   }

   const Point& operator-=(const Point& other) {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      return *this;
   }

   Point operator*(const Point& other) const {
      return Point(x * other.x, y * other.y, z * other.z);
   }

   Point operator+(const Point& other) const {
      return Point(x + other.x, y + other.y, z + other.z);
   }

   Point operator-(const Point& other) const {
      return Point(x - other.x, y - other.y, z - other.z);
   }

   Point operator-() const {
      return Point(-x, -y, -z);
   }

   void SetZero() { 
      x = 0;
      y = 0;
      z = 0;
   }

   float LengthSquared() const {
      float result = 0;
      result += x * x;
      result += y * y;
      result += z * z;
      return result;
   }

   void Scale(float s) {
      x = static_cast<S>(x * s);
      y = static_cast<S>(y * s);
      z = static_cast<S>(z * s);
   }

   void Translate(const Point& pt) {
      x += pt.x;
      y += pt.y;
      z += pt.z;
   }

   S Dot(Point const& other) const {
      S result = 0;
      result += x * other.x;
      result += y * other.y;
      result += z * other.z;
      return result;
   }


   Point Cross(Point const& other) const
   {
      return Point(y*other.z - z*other.y,
                   z*other.x - x*other.z,
                   x*other.y - y*other.x);
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

   Point operator/(S amount) const {
      // floating point divide by zero does not throw exception (it returns 1.#INF000) so check for it explicitly
      ASSERT(amount != 0);
      return Point(x / amount, y / amount, z  / amount, w / amount);
   }

   Point operator*(S amount) const {
      return Point(x * amount, y * amount, z * amount, w * amount);
   }

   bool operator==(Point const& other) const {
      return x == other.x && y == other.y && z == other.z && w == other.w;
   }

   const Point& operator*=(S scale) {
      x *= scale;
      y *= scale;
      z *= scale;
      w *= scale;
      return *this;
   }

   const Point& operator+=(const Point& other) {
      x += other.x;
      y += other.y;
      z += other.z;
      w += other.w;
      return *this;
   }

   const Point& operator-=(const Point& other) {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      w -= other.w;
      return *this;
   }

   Point operator*(const Point& other) const {
      return Point(x * other.x, y * other.y, z * other.z, w * other.w);
   }

   Point operator+(const Point& other) const {
      return Point(x + other.x, y + other.y, z + other.z, w + other.w);
   }

   Point operator-(const Point& other) const {
      return Point(x - other.x, y - other.y, z - other.z, w - other.w);
   }

   Point operator-() const {
      return Point(-x, -y, -z, -w);
   }

   void SetZero() { 
      x = 0;
      y = 0;
      z = 0;
      w = 0;
   }

   float LengthSquared() const {
      float result = 0;
      result += x * x;
      result += y * y;
      result += z * z;
      result += w * w;
      return result;
   }

   void Scale(float s) {
      x = static_cast<S>(x * s);
      y = static_cast<S>(y * s);
      z = static_cast<S>(z * s);
      w = static_cast<S>(w * s);
   }

   void Translate(const Point& pt) {
      x += pt.x;
      y += pt.y;
      z += pt.z;
      w += pt.w;
   }

   S Dot(Point const& other) const {
      S result = 0;
      result += x * other.x;
      result += y * other.y;
      result += z * other.z;
      result += w * other.w;
      return result;
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