//===============================================================================
// @ point2.h
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
      class point2
      {
      public:
          // constructor/destructor
          inline point2() {}
          inline point2(float _x, float _y) :
              x(_x), y(_y)
          {
          }
          inline ~point2() {}
    
   
          // accessors
          inline float& operator[](unsigned int i)          { return (&x)[i]; }
          inline float operator[](unsigned int i) const { return (&x)[i]; }

          float length() const;
          float length_squared() const;

          // comparison
          bool operator==(const point2& other) const;
          bool operator!=(const point2& other) const;
          bool is_zero() const;

          // manipulators
          inline void set(float _x, float _y);
          void clean();       // sets near-zero elements to 0
          inline void set_zero(); // sets all elements to 0
          void normalize();   // sets to unit vector

          // operators
          point2 operator-() const;

          // addition/subtraction
          point2 operator+(const point2& other) const;
          friend point2& operator+=(point2& self, const point2& other);
          point2 operator-(const point2& other) const;
          friend point2& operator-=(point2& self, const point2& other);


          // scalar multiplication
          point2   operator*(float scalar);
          friend point2    operator*(float scalar, const point2& vector);
          point2&          operator*=(float scalar);
          point2   operator/(float scalar);
          point2&          operator/=(float scalar);

          // dot product
          float               dot(const point2& vector) const;
          friend float        dot(const point2& vector1, const point2& point2);

          // perpendicular and cross product equivalent
          point2 Perpendicular() const { return point2(-y, x); } 
          float               perp_dot(const point2& vector) const; 
          friend float        perp_dot(const point2& vector1, const point2& point2);

          // useful defaults
          static point2    unit_x;
          static point2    unit_y;
          static point2    origin;
          static point2    xy;
        
          struct Hasher {
             std::size_t operator()(const point2& pt) const {
                std::hash<decltype(pt.x)> h;
                return h(pt.x) ^ h(pt.y);
             }
          };

      public:
          // member variables
          float x, y;
      };

      inline void point2::set(float _x, float _y) {
          x = _x; y = _y;
      }

      inline void point2::set_zero() {
          x = y = 0.0f;
      }
      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const point2& source);
   };
};

#endif
