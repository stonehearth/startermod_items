//===============================================================================
// @ quaternion.h
// 
// Quaternion class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_QUATERNION_H
#define _RADIANT_MATH_QUATERNION_H

namespace radiant {
   namespace math3d {
      class point3;
      class vector3;
      class matrix3;

      class quaternion
      {
         public:
            // constructor/destructor
            inline quaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) { }
            inline quaternion(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) { }
            quaternion(const vector3& axis, float angle);
            quaternion(const vector3& from, const vector3& to);
            explicit quaternion(const vector3& vector);
            explicit quaternion(const matrix3& rotation);
            inline ~quaternion() {}

            // copy operations
            quaternion(const quaternion& other);
            quaternion& operator=(const quaternion& other);

            // accessors
            inline float& operator[](unsigned int i)         { return (&w)[i]; }
            inline float operator[](unsigned int i) const    { return (&w)[i]; }

            float magnitude() const;
            float norm() const;

#if !defined(SWIG)
            quaternion(const protocol::quaternion &p) : x(p.x()), y(p.y()), z(p.z()), w(p.w()) { }

            void SaveValue(protocol::quaternion *msg) const {
               msg->set_x(x);
               msg->set_y(y);
               msg->set_z(z);
               msg->set_w(w);
            }
            void LoadValue(const protocol::quaternion &msg) {
               x = msg.x();
               y = msg.y();
               z = msg.z();
               w = msg.w();
            }
#endif

            // comparison
            bool operator==(const quaternion& other) const;
            bool operator!=(const quaternion& other) const;
            bool is_zero() const;
            bool is_unit() const;
            bool is_identity() const;

            // manipulators
            inline void set(float _w, float _x, float _y, float _z) {
               w = _w; x = _x; y = _y; z = _z;
            }
            void set(const vector3& axis, float angle);
            void set(const vector3& from, const vector3& to);
            // void set(const matrix3& rotation);
            void set(float z_rotation, float y_rotation, float x_rotation); 

            void get_axis_angle(vector3& axis, float& angle);

            void clean();       // sets near-zero elements to 0
            void normalize();   // sets to unit quaternion
            inline void set_zero() {
               x = y = z = w = 0.0f;
            }
            inline void set_identity() {
               x = y = z = 0.0f;
               w = 1.0f;
            }

            // complex conjugate            
            const quaternion& conjugate();

            // invert quaternion            
            const quaternion& inverse();

            // operators

            // addition/subtraction
            quaternion operator+(const quaternion& other) const;
            quaternion& operator+=(const quaternion& other);
            quaternion operator-(const quaternion& other) const;
            quaternion& operator-=(const quaternion& other);

            quaternion operator-() const;

            // scalar multiplication
            quaternion&          operator*=(float scalar);

            // quaternion multiplication
            quaternion operator*(const quaternion& other) const;
            quaternion& operator*=(const quaternion& other);

            // dot product
            float dot(const quaternion& vector) const;

            // vector rotation
            vector3 rotate(const vector3& vector) const;
            point3 rotate(const point3& vector) const;


            // useful defaults
            static quaternion   zero;
            static quaternion   identity;

         public:
            // member variables
            float w, x, y, z;      

      };

      quaternion conjugate(const quaternion& quat);
      quaternion inverse(const quaternion& quat);
      float dot(const quaternion& vector1, const quaternion& vector2);

      // interpolation
      void lerp(quaternion& result, const quaternion& start, const quaternion& end, float t);
      void slerp(quaternion& result, const quaternion& start, const quaternion& end, float t);
      void approx_slerp(quaternion& result, const quaternion& start, const quaternion& end, float t);

      inline quaternion operator*(float scalar, const quaternion& q) {
         return quaternion(scalar*q.w, scalar*q.x, scalar*q.y, scalar*q.z);
      }

      inline quaternion interpolate(const quaternion &q0, const quaternion &q1, float alpha) {
         quaternion result;
         slerp(result, q0, q1, alpha);
         return result;
      }

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const quaternion& source);
   };

};


#endif
