//===============================================================================
// @ vector3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_VECTOR3_H
#define _RADIANT_MATH_VECTOR3_H

#include "point3.h"

namespace radiant {
   namespace math3d {
      class matrix3;

      class vector3
      {
         public:
            // constructor/destructor
            inline vector3() {}
            inline vector3(float _x, float _y, float _z) :x(_x), y(_y), z(_z) { }
            explicit inline vector3(const point3 &pt) : x(pt.x), y(pt.y), z(pt.z) { }
            inline ~vector3() {}

            // copy operations
            vector3(const vector3& other);
            vector3& operator=(const vector3& other);

            // accessors
            inline float& operator[](unsigned int i)          { return (&x)[i]; }
            inline float operator[](unsigned int i) const { return (&x)[i]; }

            float length() const;
            float length_squared() const;

#if !defined(SWIG)
            inline vector3::vector3(const protocol::vector3 &v) : x(v.x()), y(v.y()), z(v.z()) { }

            void SaveValue(protocol::vector3 *v) const {
               v->set_x(x);
               v->set_y(y);
               v->set_z(z);
            }
            void LoadValue(const protocol::vector3 &v) {
               x = v.x();
               y = v.y();
               z = v.z();
            }
#endif

            // comparison
            bool operator==(const vector3& other) const;
            bool operator!=(const vector3& other) const;
            bool is_zero() const;
            bool is_unit() const;

            // manipulators
            void set(float _x, float _y, float _z) {
               x = _x; y = _y; z = _z;
            }
            void clean();       // sets near-zero elements to 0
            inline void set_zero() {
               x = y = z = 0.0f;
            }
            void normalize();   // sets to unit vector

            // operators

            // addition/subtraction
            vector3 operator+(const vector3& other) const;
            vector3 operator-(const vector3& other) const;

            vector3 operator-() const;

            // scalar multiplication
            vector3   operator*(float scalar) const;
            vector3&  operator*=(float scalar);
            vector3   operator/(float scalar) const;
            vector3&  operator/=(float scalar);

            // dot product/cross product
            float               dot(const vector3& vector) const;
            vector3           cross(const vector3& vector) const;

            // useful defaults
            static vector3    unit_x;
            static vector3    unit_y;
            static vector3    unit_z;
            static vector3    origin;
            static vector3    zero;

         public:
            float x, y, z;        
      };

      float distance(const vector3& p0, const vector3& p1);
      float distance_squared(const vector3& p0, const vector3& p1);
      vector3& operator+=(vector3& vector, const vector3& other);
      vector3& operator-=(vector3& vector, const vector3& other);
      vector3 operator*(float scalar, const vector3& vector);
      float dot(const vector3& vector1, const vector3& vector2);
      vector3 cross(const vector3& vector1, const vector3& vector2);      

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const vector3& source);
   };
};


#endif
