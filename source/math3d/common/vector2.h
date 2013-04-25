//===============================================================================
// @ vector2.h
// 
// 2D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_VECTOR2_H
#define _RADIANT_MATH3D_VECTOR2_H

namespace radiant {
   namespace math3d {
      class vector2
      {
      public:
          // constructor/destructor
          inline vector2() {}
          inline vector2(float _x, float _y) :
              x(_x), y(_y)
          {
          }
          inline ~vector2() {}
    
          // accessors
          inline float& operator[](unsigned int i)          { return (&x)[i]; }
          inline float operator[](unsigned int i) const { return (&x)[i]; }

          float length() const;
          float length_squared() const;

          // comparison
          bool operator==(const vector2& other) const;
          bool operator!=(const vector2& other) const;
          bool is_zero() const;

          // manipulators
          inline void set(float _x, float _y);
          void clean();       // sets near-zero elements to 0
          inline void set_zero(); // sets all elements to 0
          void normalize();   // sets to unit vector

          // operators
          vector2 operator-() const;

          // addition/subtraction
          vector2 operator+(const vector2& other) const;
          friend vector2& operator+=(vector2& self, const vector2& other);
          vector2 operator-(const vector2& other) const;
          friend vector2& operator-=(vector2& self, const vector2& other);


          // scalar multiplication
          vector2   operator*(float scalar);
          friend vector2    operator*(float scalar, const vector2& vector);
          vector2&          operator*=(float scalar);
          vector2   operator/(float scalar);
          vector2&          operator/=(float scalar);

          // dot product
          float               dot(const vector2& vector) const;
          friend float        dot(const vector2& vector1, const vector2& vector2);

          // perpendicular and cross product equivalent
          vector2 Perpendicular() const { return vector2(-y, x); } 
          float               perp_dot(const vector2& vector) const; 
          friend float        perp_dot(const vector2& vector1, const vector2& vector2);

          // useful defaults
          static vector2    unit_x;
          static vector2    unit_y;
          static vector2    origin;
          static vector2    xy;
        
      public:
          // member variables
          float x, y;
      };

      inline void vector2::set(float _x, float _y) {
          x = _x; y = _y;
      }

      inline void vector2::set_zero() {
          x = y = 0.0f;
      }
    
      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const vector2& source);
   };
};

#endif
