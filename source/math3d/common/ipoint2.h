//===============================================================================
// @ ipoint2.h
// 
// 2D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_POINT2_H
#define _RADIANT_MATH3D_POINT2_H

namespace radiant {
   namespace math3d {
      class point3;
      class ipoint3;
      class quaternion;
   }
   namespace math2d {
      enum CoordinateSystem {
         UNDEFINED_COORDINATE_SYSTEM = -1,
         XZ_PLANE = 0,
         XY_PLANE = 1,
         YZ_PLANE = 2,
      };

      class ipoint2
      {
      public:
          // constructor/destructor
          inline ipoint2() {}
          inline ipoint2(int _x, int _y, CoordinateSystem cs);
          ipoint2(const radiant::math3d::point3 &p, CoordinateSystem cs);
          ipoint2(const radiant::math3d::ipoint3 &p, CoordinateSystem cs);
          inline ~ipoint2() {}
    
          ipoint2 ProjectOnto(CoordinateSystem cs, int width) const;

          // accessors
          inline int& operator[](unsigned int i)          { return (&x)[i]; }
          inline int operator[](unsigned int i) const { return (&x)[i]; }

          int length_squared() const;
          CoordinateSystem GetCoordinateSystem() const { return space_; }
          math3d::quaternion GetQuaternion() const;

          // comparison
          bool operator==(const ipoint2& other) const;
          bool operator!=(const ipoint2& other) const;
          bool is_zero() const;

          // manipulators
          inline void set(int _x, int _y);
          void clean();       // sets near-zero elements to 0
          inline void set_zero(); // sets all elements to 0

          ipoint2 abs() const {
             return ipoint2(::abs(x), ::abs(y), space_);
          }
          ipoint2 tangent() const {
             return ipoint2(-y, x, space_);
          }

          // operators
          ipoint2 operator-() const;

          // addition/subtraction
          ipoint2 operator+(const ipoint2& other) const;
          ipoint2& operator+=(const ipoint2& other) {
             x += other.x;
             y += other.y;
             return *this;
          }
          ipoint2 operator-(const ipoint2& other) const;
          friend ipoint2& operator-=(ipoint2& self, const ipoint2& other);


          // scalar multiplication
          ipoint2   operator*(int scalar);
          friend ipoint2    operator*(int scalar, const ipoint2& vector);
          ipoint2&          operator*=(int scalar);
          ipoint2   operator/(int scalar);
          ipoint2&          operator/=(int scalar);

          // dot product
          int               dot(const ipoint2& vector) const;
          friend int        dot(const ipoint2& vector1, const ipoint2& ipoint2);

          // perpendicular and cross product equivalent
          ipoint2 Perpendicular() const { return ipoint2(-y, x, space_); } 
          int               perp_dot(const ipoint2& vector) const; 
          friend int        perp_dot(const ipoint2& vector1, const ipoint2& ipoint2);

          struct Hasher {
             std::size_t operator()(const ipoint2& pt) const {
                std::hash<decltype(pt.x)> h;
                return h(pt.x) ^ h(pt.y);
             }
          };

      public:

          // member variables
          int x, y;
          CoordinateSystem space_;
      };

      typedef ipoint2 Point2d;

      inline void ipoint2::set(int _x, int _y) {
          x = _x; y = _y;
      }

      inline void ipoint2::set_zero() {
          x = y = 0;
      }
      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const ipoint2& source);
   };
};

#endif
