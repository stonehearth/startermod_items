#ifndef _RADIANT_MATH3D_COLOR_H
#define _RADIANT_MATH3D_COLOR_H

#include <algorithm>
#include "radiant.pb.h"
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      class color4 {
      public:
         unsigned char r, g, b, a;
         color4() { }
         color4(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_ = 255) : r(r_), g(g_), b(b_), a(a_) { }
         color4(const protocol::color& c) {
            LoadValue(c);
         }

         unsigned char operator[](int i) const {
            return (&r)[i];
         }

         unsigned char &operator[](int i) {
            return (&r)[i];
         }

         bool operator==(const math3d::color4& other) {
            return r == other.r && g == other.g && b == other.b && a == other.a;
         }

         bool operator!=(const math3d::color4& other) {
            return r != other.r || g != other.g || b != other.b || a != other.a;
         }

         void operator*=(float s) {
            s = std::min(std::max(s, 1.0f), 0.0f);
            for (int i = 0; i < 4; i++) {
               (*this)[i] = (int)((*this)[i] * s);
            }
         }

         void LoadValue(const protocol::color& c) {
            r = c.r();
            g = c.g();
            b = c.b();
            a = c.a();
         }

         void SaveValue(protocol::color *c) const {
            c->set_r(r);
            c->set_g(g);
            c->set_b(b);
            c->set_a(a);
         }

         int ToInteger() const {
            return ((unsigned int)r) |
                   (((unsigned int)g) << 8) |
                   (((unsigned int)b) << 16) |
                   (((unsigned int)a) << 24);
         }
         static color4 FromInteger(unsigned int i) {
            return color4(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff, (i >> 24) & 0xff);
         }
      };

      std::ostream& operator<<(std::ostream& out, const color4 &c);


      class color3 {
      public:
         unsigned char r, g, b;
         color3() { }
         color3(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) { }

         unsigned char operator[](int i) const {
            return (&r)[i];
         }

         unsigned char &operator[](int i) {
            return (&r)[i];
         }

         void operator*=(float s) {
            s = std::min(std::max(s, 1.0f), 0.0f);
            for (int i = 0; i < 3; i++) {
               (*this)[i] = (int)((*this)[i] * s);
            }
         }

         void SaveValue(protocol::color *c) const {
            c->set_r(r);
            c->set_g(g);
            c->set_b(b);
         }

         void LoadValue(const protocol::color& c) {
            r = c.r();
            g = c.g();
            b = c.b();
         }

         int ToInteger() const {
            return ((unsigned int)r) |
                   (((unsigned int)g) << 8) |
                   (((unsigned int)b) << 16);
         }
         static color3 FromInteger(unsigned int i) {
            return color3(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff);
         }

         static color3 red;
         static color3 orange;
         static color3 yellow;
         static color3 green;
         static color3 blue;
      };
      std::ostream& operator<<(std::ostream& out, const color3& source);

      struct Histogram {
         static color3 Sample(float f);
      };
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::color3, Protocol::color)
IMPLEMENT_DM_EXTENSION(::radiant::math3d::color4, Protocol::color)

#endif
