//===============================================================================
// @ ipoint3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_IPOINT3_H
#define _RADIANT_MATH_IPOINT3_H

#include "common.h"
#include "point3.h"
#include "vector3.h"
#include "ipoint2.h"
#include "color.h"
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      class ipoint3 {
      public:
         int x, y, z;

         ipoint3() { }
         ipoint3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) { }
         ipoint3(const ipoint3 &pt) : x(pt.x), y(pt.y), z(pt.z) { }
         ipoint3(const radiant::math2d::ipoint2 &p, int extraDimension = 0);
         ipoint3(const radiant::math2d::ipoint2 &p, const math3d::ipoint3& offset);

         explicit ipoint3(const point3 &pt) {
            x = (int)(pt.x + (pt.x < 0 ? -k_epsilon : k_epsilon));
            y = (int)(pt.y + (pt.y < 0 ? -k_epsilon : k_epsilon));
            z = (int)(pt.z + (pt.z < 0 ? -k_epsilon : k_epsilon));
         }
         explicit ipoint3(const vector3 &pt) {
            x = (int)(pt.x + (pt.x < 0 ? -k_epsilon : k_epsilon));
            y = (int)(pt.y + (pt.y < 0 ? -k_epsilon : k_epsilon));
            z = (int)(pt.z + (pt.z < 0 ? -k_epsilon : k_epsilon));
         }

         bool is_zero() const { return x == 0 && y == 0 && z == 0; }
         void set_zero() { x = y = z = 0; }

#if !defined(SWIG)
         ipoint3(const protocol::ipoint3 &p) : x(p.x()), y(p.y()), z(p.z()) { }
         explicit ipoint3(const protocol::point3 &p) : x((int)p.x()), y((int)p.y()), z((int)p.z()) { }

         void SaveValue(protocol::ipoint3 *p) const {
            p->set_x(x);
            p->set_y(y);
            p->set_z(z);
         }
         void SaveValue(protocol::coord *p, const math3d::color4 &c) const {
            p->set_x(x);
            p->set_y(y);
            p->set_z(z);
            c.SaveValue(p->mutable_color());
         }
         void SaveValue(protocol::point3 *p) const {
            p->set_x((float)x);
            p->set_y((float)y);
            p->set_z((float)z);
         }
         void LoadValue(const protocol::ipoint3 &p) {
            x = p.x();
            y = p.y();
            z = p.z();
         }
#endif
         operator size_t() const {
            // xxx: make a wrapper to add this so we don't accidently compare points to sizes.
            return (((x * 863) * y * 971) * z * 991); // bunch-o-primes.  xxx: NO IDEA if this is any good
         }

         ipoint3 operator+(const ipoint3 &pt) const {
            return ipoint3(x + pt.x, y + pt.y, z + pt.z);
         }
         ipoint3 &operator+=(const ipoint3 &pt) {
            x += pt.x;
            y += pt.y;
            z += pt.z;
            return *this;
         }

         ipoint3 operator-(const ipoint3 &pt) const {
            return ipoint3(x - pt.x, y - pt.y, z - pt.z);
         }
         ipoint3 operator*(int scale) const {
            return ipoint3(x * scale, y * scale, z * scale);
         }
         ipoint3 &operator-=(const ipoint3 &pt) {
            x -= pt.x;
            y -= pt.y;
            z -= pt.z;
            return *this;
         }

         float distanceTo(const math3d::ipoint3& other) const {
            return sqrt((float)(*this - other).distanceSquared());
         }

         int distanceSquared() const {
            return (x * x) + (y * y) + (z * z);
         }
         bool is_adjacent_to(const ipoint3& other) const {
            return ((*this) - other).distanceSquared() == 1;
         }

         bool operator!=(const ipoint3 &pt) const {
            return x != pt.x || y != pt.y || z != pt.z;
         }
         bool operator==(const ipoint3 &pt) const {
            return x == pt.x && y == pt.y && z == pt.z;
         }
         bool operator<(const ipoint3 &pt) const {
            if (x != pt.x) {
               return x < pt.x;
            }
            if (y != pt.y) {
               return y < pt.y;
            }
            return z < pt.z;
         }
         bool operator>(const ipoint3 &pt) const {
            if (x != pt.x) {
               return x > pt.x;
            }
            if (y != pt.y) {
               return y > pt.y;
            }
            return z > pt.z;
         }
         bool operator<=(const ipoint3 &pt) const {
            return (*this == pt) || (*this < pt);
         }
         bool operator>=(const ipoint3 &pt) const {
            return (*this == pt) || (*this > pt);
         }
         int &operator[](int i) { return (&x)[i]; }
         int operator[](int i) const { return (&x)[i]; }

         static ipoint3    origin;
         static ipoint3    one;
      };
      std::ostream& operator<<(std::ostream& out, const ipoint3& source);
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::ipoint3, Protocol::ipoint3)

#endif
