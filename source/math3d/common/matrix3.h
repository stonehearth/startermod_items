//===============================================================================
// @ matrix3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_MATRIX3_H
#define _RADIANT_MATH_MATRIX3_H

#include "vector3.h"


namespace radiant {
   namespace math3d {
      class quaternion;

      class matrix3
      {
             friend class matrix4;
         public:
             // constructor/destructor
             inline matrix3() {}
             inline ~matrix3() {}
             explicit matrix3(const quaternion& quat);
    
             // copy operations
             matrix3(const matrix3& other);
             matrix3& operator=(const matrix3& other);

             // accessors
             inline float& operator()(unsigned int i, unsigned int j) {
                return v[i + 3*j];
             }
             inline float operator()(unsigned int i, unsigned int j) const {
                return v[i + 3*j];
             }

             // comparison
             bool operator==(const matrix3& other) const;
             bool operator!=(const matrix3& other) const;
             bool is_zero() const;
             bool is_identity() const;

             // manipulators
             void set_rows(const vector3& row1, const vector3& row2, const vector3& row3); 
             void get_rows(vector3& row1, vector3& row2, vector3& row3) const; 
             vector3 GetRow(unsigned int i) const; 

             void set_columns(const vector3& col1, const vector3& col2, const vector3& col3); 
             void get_columns(vector3& col1, vector3& col2, vector3& col3) const; 
             vector3 get_column(unsigned int i) const; 

             void clean();
             void set_identity();

             matrix3& inverse();
             matrix3& transpose();

             // useful computations
             matrix3 Adjoint() const;
             float determinant() const;
             float Guard() const;
        
             // transformations
             matrix3& rotation(const quaternion& rotate);
             matrix3& rotation(float z_rotation, float y_rotation, float x_rotation);
             matrix3& rotation(const vector3& axis, float angle);

             matrix3& scaling(const vector3& scale);

             matrix3& rotation_x(float angle);
             matrix3& rotation_y(float angle);
             matrix3& rotation_z(float angle);

             void get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation);
             void get_axis_angle(vector3& axis, float& angle);

             // operators

             // addition and subtraction
             matrix3 operator+(const matrix3& other) const;
             matrix3& operator+=(const matrix3& other);
             matrix3 operator-(const matrix3& other) const;
             matrix3& operator-=(const matrix3& other);

             matrix3 operator-() const;

             // multiplication
             matrix3& operator*=(const matrix3& matrix);
             matrix3 operator*(const matrix3& matrix) const;

             // column vector multiplier
             vector3 operator*(const vector3& vector) const;
             point3 operator*(const point3& vector) const;

             // row vector multiplier
             friend vector3 operator*(const vector3& vector, const matrix3& matrix);

             matrix3& operator*=(float scalar);
             friend matrix3 operator*(float scalar, const matrix3& matrix);
             matrix3 operator*(float scalar) const;

             // low-level data accessors - implementation-dependent
             operator float*() { return v; }
             operator const float*() const { return v; }

         public:
             // member variables
             float v[9];
        
         private:
      };
      matrix3 inverse(const matrix3& mat);
      matrix3 transpose(const matrix3& mat);

      vector3 operator*(const vector3& vector, const matrix3& mat);
      point3 operator*(const point3& point, const matrix3& mat);

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const matrix3& source);
   };
};

#endif
