//===============================================================================
// @ matrix4.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_MATRIX4_H
#define _RADIANT_MATH_MATRIX4_H


#include "vector3.h"
#include "vector4.h"

namespace radiant {
   namespace math3d {

      class quaternion;
      class matrix3;
      class vector3;

      class matrix4
      {
         public:
             // constructor/destructor
             inline matrix4() { identity(); }
             inline ~matrix4() {}
             explicit matrix4(const quaternion& quat);
             explicit matrix4(const matrix3& matrix);
    
             // copy operations
             matrix4(const matrix4& other);
             matrix4& operator=(const matrix4& other);

             // accessors
             float &operator()(unsigned int i, unsigned int j) { return v[i + 4*j]; }
             float operator()(unsigned int i, unsigned int j) const { return v[i + 4*j]; }
             inline const float* get_float_ptr() { return v; }

             // comparison
             bool operator==(const matrix4& other) const;
             bool operator!=(const matrix4& other) const;
             bool is_zero() const;
             bool is_identity() const;

             // manipulators
             void set_rows(const vector4& row1, const vector4& row2, 
                           const vector4& row3, const vector4& row4); 
             void get_rows(vector4& row1, vector4& row2, vector4& row3, vector4& row4); 

             void set_columns(const vector4& col1, const vector4& col2, 
                              const vector4& col3, const vector4& col4); 
             void get_columns(vector4& col1, vector4& col2, vector4& col3, vector4& col4); 

             void clean();
             void identity();

             matrix4& affine_inverse();

             matrix4& transpose();
             friend matrix4 transpose(const matrix4& mat);
        
             // transformations
             matrix4& translation(const vector3& xlate);
             matrix4& rotation(const matrix3& matrix);
             matrix4& rotation(const quaternion& rotate);
             matrix4& rotation(float z_rotation, float y_rotation, float x_rotation);
             matrix4& rotation(const vector3& axis, float angle);

             matrix4& scaling(const vector3& scale);

             matrix4& rotation_x(float angle);
             matrix4& rotation_y(float angle);
             matrix4& rotation_z(float angle);

             void get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation);
             void get_axis_angle(vector3& axis, float& angle);

             // operators

             // addition and subtraction
             matrix4 operator+(const matrix4& other) const;
             matrix4& operator+=(const matrix4& other);
             matrix4 operator-(const matrix4& other) const;
             matrix4& operator-=(const matrix4& other);

             matrix4 operator-() const;

             // multiplication
             matrix4& operator*=(const matrix4& matrix);
             matrix4 operator*(const matrix4& matrix) const;

             // column vector multiplier
             vector4 operator*(const vector4& vector) const;
             // row vector multiplier
             friend vector4 operator*(const vector4& vector, const matrix4& matrix);

             // scalar multiplication
             matrix4& operator*=(float scalar);
             friend matrix4 operator*(float scalar, const matrix4& matrix);
             matrix4 operator*(float scalar) const;

             // vector3 ops
             vector3 transform(const vector3& point) const;

             // point ops
             point3 transform(const point3& point) const;
             point3 rotate(const point3& point) const;

             // low-level data accessors - implementation-dependent
             operator float*() { return v; }
             operator const float*() const { return v; }

         public:
             // member variables
             float v[16];
      };
      matrix4 affine_inverse(const matrix4& mat);

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const matrix4& source);
   };
};
#endif
