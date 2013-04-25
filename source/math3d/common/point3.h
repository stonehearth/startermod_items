#ifndef _RADIANT_MATH_POINT3_H
#define _RADIANT_MATH_POINT3_H

#include "radiant.pb.h"
#include "store.pb.h"
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      class matrix3;
      class vector3;
      class ipoint3;

      class point3
      {
         public:
             // constructor/destructor
             inline point3() {}
             inline point3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) { }
             explicit point3(const ipoint3 &p);
             inline ~point3() {}

             // copy operations
             point3(const point3& other);
             point3& operator=(const point3& other);

             // accessors
             inline float& operator[](unsigned int i)          { return (&x)[i]; }
             inline float operator[](unsigned int i) const { return (&x)[i]; }

             float length() const;
             float length_squared() const;

             float distance(const point3& other) const;
             float distance_squared(const point3& other) const;

             inline point3(const protocol::point3 &p) : x(p.x()), y(p.y()), z(p.z()) { }

             void SaveValue(protocol::point3 *p) const {
                p->set_x(x);
                p->set_y(y);
                p->set_z(z);
             }
             void LoadValue(const protocol::point3 &p) {
                x = p.x();
                y = p.y();
                z = p.z();
             }

             // comparison
             bool operator==(const point3& other) const;
             bool operator!=(const point3& other) const;
             bool is_zero() const;
             bool is_unit() const;
             bool operator<(const point3 &pt) const {
                if (x != pt.x) {
                   return x < pt.x;
                }
                if (y != pt.y) {
                   return y < pt.y;
                }
                return z < pt.z;
             }
             bool operator>(const point3 &pt) const {
                if (x != pt.x) {
                   return x > pt.x;
                }
                if (y != pt.y) {
                   return y > pt.y;
                }
                return z > pt.z;
             }

             // manipulators
             void clean();       // sets near-zero elements to 0
             void normalize();   // sets to unit vector
             inline void set_zero() {
                x = y = z = 0.0f;
             }
             inline void set(float _x, float _y, float _z) {
                x = _x; y = _y; z = _z;
             }

             // operators

             // addition/subtraction
             point3 operator+(const point3& other) const;
             point3 operator+(const vector3& other) const;
             point3 operator-(const point3& other) const;
             point3 operator-(const vector3& other) const;

             point3 operator-() const;

             // scalar multiplication
             point3 operator*(float scalar);
             point3& operator*=(float scalar);
             point3 operator/(float scalar);
             point3& operator/=(float scalar);
             point3& operator-=(const vector3& other);
             point3& operator-=(const point3& other);

             // useful defaults
             static point3    origin;
             static point3    unit_x;
             static point3    unit_y;
             static point3    unit_z;
    
         public:
             float x, y, z;        
      };
      math3d::point3 interpolate(const math3d::point3 &p0, const math3d::point3 &p1, float alpha);      

      float distance(const point3& p0, const point3& p1);
      float distance_squared(const point3& p0, const point3& p1);

      point3 operator*(float scalar, const point3& vector);
      point3& operator+=(point3& p, const vector3& other);
      point3& operator+=(point3& p, const point3& other);
      point3& operator-=(point3& p, const vector3& other);

      point3 operator*(const point3& vector, const matrix3& mat);

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const point3& source);
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::point3, Protocol::point3)

#endif
