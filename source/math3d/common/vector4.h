//===============================================================================
// @ vector4.h
// 
// 4D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_VECTOR4_H
#define _RADIANT_MATH_VECTOR4_H

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Typedefs, Structs ----------------------------------------------------------
//-------------------------------------------------------------------------------

namespace radiant {
   namespace math3d {
      class matrix4;

      //-------------------------------------------------------------------------------
      //-- Classes --------------------------------------------------------------------
      //-------------------------------------------------------------------------------

      class vector4
      {
          friend class matrix4;
    
      public:
          // constructor/destructor
          inline vector4() {}
          inline vector4(float _x, float _y, float _z, float _w) :
              x(_x), y(_y), z(_z), w(_w)
          {
          }
          inline ~vector4() {}

          // copy operations
          vector4(const vector4& other);
          vector4& operator=(const vector4& other);

          // accessors
          inline float& operator[](unsigned int i)         { return (&x)[i]; }
          inline float operator[](unsigned int i) const    { return (&x)[i]; }

          float length() const;
          float length_squared() const;

          // comparison
          bool operator==(const vector4& other) const;
          bool operator!=(const vector4& other) const;
          bool is_zero() const;
          bool is_unit() const;

          // manipulators
          inline void set(float _x, float _y, float _z, float _w);
          void clean();       // sets near-zero elements to 0
          inline void set_zero(); // sets all elements to 0
          void normalize();   // sets to unit vector

          // operators

          // addition/subtraction
          vector4 operator+(const vector4& other) const;
          vector4& operator+=(const vector4& other);
          vector4 operator-(const vector4& other) const;
          vector4& operator-=(const vector4& other);

          // scalar multiplication
          vector4    operator*(float scalar);
          friend vector4    operator*(float scalar, const vector4& vector);
          vector4&          operator*=(float scalar);
          vector4    operator/(float scalar);
          vector4&          operator/=(float scalar);

          // dot product
          float              dot(const vector4& vector) const;
          friend float       dot(const vector4& vector1, const vector4& vector2);

          // matrix products
          friend vector4 operator*(const vector4& vector, const matrix4& mat);

          // useful defaults
          static vector4    unit_x;
          static vector4    unit_y;
          static vector4    unit_z;
          static vector4    unit_w;
          static vector4    origin;
        
      public:
          // member variables
          float x, y, z, w;
      };
      inline void  vector4::set(float _x, float _y, float _z, float _w) {
         x = _x; y = _y; z = _z; w = _w; 
      }
      inline void vector4::set_zero() {
         x = y = z = w = 0.0f;
      }

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const vector4& source);
   };
};


#endif
