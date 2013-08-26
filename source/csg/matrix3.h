#ifndef _RADIANT_CSG_MATRIX3_H
#define _RADIANT_CSG_MATRIX3_H

#include <ostream>
#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Quaternion;

class Matrix3
{
      friend class Matrix4;
public:
      // constructor/destructor
      inline Matrix3() {}
      inline ~Matrix3() {}
      explicit Matrix3(const Quaternion& quat);
    
      // copy operations
      Matrix3(const Matrix3& other);
      Matrix3& operator=(const Matrix3& other);

      // accessors
      inline float& operator()(unsigned int i, unsigned int j) {
         return v[i + 3*j];
      }
      inline float operator()(unsigned int i, unsigned int j) const {
         return v[i + 3*j];
      }

      // comparison
      bool operator==(const Matrix3& other) const;
      bool operator!=(const Matrix3& other) const;
      bool IsZero() const;
      bool is_identity() const;

      // manipulators
      void set_rows(const Point3f& row1, const Point3f& row2, const Point3f& row3); 
      void get_rows(Point3f& row1, Point3f& row2, Point3f& row3) const; 
      Point3f GetRow(unsigned int i) const; 

      void set_columns(const Point3f& col1, const Point3f& col2, const Point3f& col3); 
      void get_columns(Point3f& col1, Point3f& col2, Point3f& col3) const; 
      Point3f get_column(unsigned int i) const; 

      void clean();
      void SetIdentity();

      Matrix3& inverse();
      Matrix3& transpose();

      // useful computations
      Matrix3 Adjoint() const;
      float determinant() const;
      float Guard() const;
        
      // transformations
      Matrix3& rotation(const Quaternion& rotate);
      Matrix3& rotation(float z_rotation, float y_rotation, float x_rotation);
      Matrix3& rotation(const Point3f& axis, float angle);

      Matrix3& scaling(const Point3f& scale);

      Matrix3& rotation_x(float angle);
      Matrix3& rotation_y(float angle);
      Matrix3& rotation_z(float angle);

      void get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation);
      void get_axis_angle(Point3f& axis, float& angle);

      // operators

      // addition and subtraction
      Matrix3 operator+(const Matrix3& other) const;
      Matrix3& operator+=(const Matrix3& other);
      Matrix3 operator-(const Matrix3& other) const;
      Matrix3& operator-=(const Matrix3& other);

      Matrix3 operator-() const;

      // multiplication
      Matrix3& operator*=(const Matrix3& matrix);
      Matrix3 operator*(const Matrix3& matrix) const;

      // column vector multiplier
      Point3f operator*(const Point3f& vector) const;

      // row vector multiplier
      friend Point3f operator*(const Point3f& vector, const Matrix3& matrix);

      Matrix3& operator*=(float scalar);
      friend Matrix3 operator*(float scalar, const Matrix3& matrix);
      Matrix3 operator*(float scalar) const;

      // low-level data accessors - implementation-dependent
      operator float*() { return v; }
      operator const float*() const { return v; }

public:
      // member variables
      float v[9];
        
private:
};
Matrix3 inverse(const Matrix3& mat);
Matrix3 transpose(const Matrix3& mat);

Point3f operator*(const Point3f& vector, const Matrix3& mat);
Point3f operator*(const Point3f& point, const Matrix3& mat);

// text output (for debugging)
std::ostream& operator<<(std::ostream& out, const Matrix3& source);

END_RADIANT_CSG_NAMESPACE

#endif
