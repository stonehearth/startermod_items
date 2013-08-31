#ifndef _RADIANT_CSG_MATRIX4_H
#define _RADIANT_CSG_MATRIX4_H

#include <ostream>
#include "point.h"
#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Matrix3;
class Quaternion;

class Matrix4
{
public:
      // constructor/destructor
      inline Matrix4() { identity(); }
      inline ~Matrix4() {}
      explicit Matrix4(const Quaternion& quat);
      explicit Matrix4(const Matrix3& matrix);
    
      // copy operations
      Matrix4(const Matrix4& other);
      Matrix4& operator=(const Matrix4& other);

      // accessors
      float &operator()(unsigned int i, unsigned int j) { return v[i + 4*j]; }
      float operator()(unsigned int i, unsigned int j) const { return v[i + 4*j]; }
      inline const float* get_float_ptr() { return v; }

      // comparison
      bool operator==(const Matrix4& other) const;
      bool operator!=(const Matrix4& other) const;
      bool IsZero() const;
      bool is_identity() const;

      // manipulators
      void set_rotation_bases(const Point3f& forward, const Point3f& up, const Point3f& left);
      void set_rows(const Point4f& row1, const Point4f& row2, 
                  const Point4f& row3, const Point4f& row4); 
      void get_rows(Point4f& row1, Point4f& row2, Point4f& row3, Point4f& row4); 

      void set_columns(const Point4f& col1, const Point4f& col2, 
                     const Point4f& col3, const Point4f& col4); 
      void get_columns(Point4f& col1, Point4f& col2, Point4f& col3, Point4f& col4); 

      void clean();
      void identity();

      Matrix4& affine_inverse();

      Matrix4& transpose();
      friend Matrix4 transpose(const Matrix4& mat);
        
      // transformations
      Matrix4& translation(const Point3f& xlate);
      Matrix4& rotation(const Matrix3& matrix);
      Matrix4& rotation(const Quaternion& rotate);
      Matrix4& rotation(float z_rotation, float y_rotation, float x_rotation);
      Matrix4& rotation(const Point3f& axis, float angle);

      Matrix4& scaling(const Point3f& scale);

      Matrix4& rotation_x(float angle);
      Matrix4& rotation_y(float angle);
      Matrix4& rotation_z(float angle);

      void get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation);
      void get_axis_angle(Point3f& axis, float& angle);

      // operators

      // addition and subtraction
      Matrix4 operator+(const Matrix4& other) const;
      Matrix4& operator+=(const Matrix4& other);
      Matrix4 operator-(const Matrix4& other) const;
      Matrix4& operator-=(const Matrix4& other);

      Matrix4 operator-() const;

      // multiplication
      Matrix4& operator*=(const Matrix4& matrix);
      Matrix4 operator*(const Matrix4& matrix) const;

      // column vector multiplier
      Point4f operator*(const Point4f& vector) const;
      // row vector multiplier
      friend Point4f operator*(const Point4f& vector, const Matrix4& matrix);

      // scalar multiplication
      Matrix4& operator*=(float scalar);
      friend Matrix4 operator*(float scalar, const Matrix4& matrix);
      Matrix4 operator*(float scalar) const;

      // point ops
      Point3f transform(const Point3f& point) const;
      Point3f rotate(const Point3f& point) const;

      // low-level data accessors - implementation-dependent
      operator float*() { return v; }
      operator const float*() const { return v; }

      void fill(const float *values);
      void set_translation(const Point3f &translation);
      Point3f get_translation();

public:
      // member variables
      float v[16];
};
Matrix4 affine_inverse(const Matrix4& mat);

// text output (for debugging)
std::ostream& operator<<(std::ostream& out, const Matrix4& source);

END_RADIANT_CSG_NAMESPACE

#endif
