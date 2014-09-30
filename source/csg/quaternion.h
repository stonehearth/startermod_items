#ifndef _RADIANT_CSG_QUATERNION_H
#define _RADIANT_CSG_QUATERNION_H

#include "point.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Matrix3;
class Matrix4;

class Quaternion
{
   public:
      // constructor/destructor
      inline Quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) { }
      inline Quaternion(double _w, double _x, double _y, double _z) : w(_w), x(_x), y(_y), z(_z) { }
      Quaternion(Point3f const& axis, double angle);
      Quaternion(Point3f const& from, Point3f const& to);
      Quaternion(const protocol::quaternion& c) {
         LoadValue(c);
      }
      explicit Quaternion(Point3f const& vector);
      explicit Quaternion(const Matrix3& rotation);
      explicit Quaternion(const Matrix4& rotation);
      inline ~Quaternion() {}

      // copy operations
      Quaternion(const Quaternion& other);
      Quaternion& operator=(const Quaternion& other);

      // accessors
      inline double& operator[](unsigned int i)         { return (&w)[i]; }
      inline double operator[](unsigned int i) const    { return (&w)[i]; }

      double magnitude() const;
      double norm() const;

      void SaveValue(protocol::quaternion *msg) const;
      void LoadValue(const protocol::quaternion &msg);

      // comparison
      bool operator==(const Quaternion& other) const;
      bool operator!=(const Quaternion& other) const;
      bool IsZero() const;
      bool is_unit() const;
      bool is_identity() const;

      // manipulators
      inline void set(double _w, double _x, double _y, double _z) {
         w = _w; x = _x; y = _y; z = _z;
      }
      void set(Point3f const& axis, double angle);
      void set(Point3f const& from, Point3f const& to);
      // void set(const Matrix3& rotation);
      void set(double z_rotation, double y_rotation, double x_rotation); 
      void lookAt(const Point3f& from, const Point3f& target);

      void get_axis_angle(Point3f& axis, double& angle) const;

      void clean();       // sets near-zero elements to 0
      void Normalize();   // sets to unit Quaternion
      inline void SetZero() {
         x = y = z = w = 0.0f;
      }
      inline void SetIdentity() {
         x = y = z = 0.0f;
         w = 1.0f;
      }

      // complex conjugate            
      const Quaternion& conjugate();

      // invert Quaternion            
      const Quaternion& inverse();

      // operators

      // addition/subtraction
      Quaternion operator+(const Quaternion& other) const;
      Quaternion& operator+=(const Quaternion& other);
      Quaternion operator-(const Quaternion& other) const;
      Quaternion& operator-=(const Quaternion& other);

      Quaternion operator-() const;

      // scalar multiplication
      Quaternion&          operator*=(double scalar);

      // Quaternion multiplication
      Quaternion operator*(const Quaternion& other) const;
      Quaternion& operator*=(const Quaternion& other);

      // dot product
      double Dot(const Quaternion& vector) const;

      // vector rotation
      Point3f rotate(Point3f const& vector) const;

      // useful defaults
      static Quaternion   zero;
      static Quaternion   identity;

   public:
      // member variables
      double w, x, y, z;      

};

Quaternion conjugate(const Quaternion& quat);
Quaternion inverse(const Quaternion& quat);
double dot(const Quaternion& vector1, const Quaternion& vector2);

// interpolation
void lerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t);
void slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t);
void approx_slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t);

inline Quaternion operator*(double scalar, const Quaternion& q) {
   return Quaternion(scalar*q.w, scalar*q.x, scalar*q.y, scalar*q.z);
}

inline Quaternion Interpolate(const Quaternion &q0, const Quaternion &q1, double alpha) {
   Quaternion result;
   slerp(result, q0, q1, alpha);
   return result;
}

std::ostream& operator<<(std::ostream& out, const Quaternion& source);

END_RADIANT_CSG_NAMESPACE


#endif
