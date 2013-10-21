#ifndef _RADIANT_CSG_COLOR_H
#define _RADIANT_CSG_COLOR_H

#include <ostream>
#include <iomanip>
#include <sstream>
#include "namespace.h"
#include "dm/dm.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Color4 {
public:
   unsigned char r, g, b, a;
   Color4() { }
   Color4(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_ = 255) : r(r_), g(g_), b(b_), a(a_) { }
   Color4(const protocol::color& c) {
      LoadValue(c);
   }


   unsigned char operator[](int i) const {
      return (&r)[i];
   }

   unsigned char &operator[](int i) {
      return (&r)[i];
   }

   bool operator==(const Color4& other) {
      return r == other.r && g == other.g && b == other.b && a == other.a;
   }

   bool operator!=(const Color4& other) {
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
   std::string ToString() const {
      std::stringstream out;
      out << std::hex << std::setfill('0') << std::setw(2) << '#' << r << g << b << a;
      return out.str();
   }
   static Color4 FromInteger(unsigned int i) {
      return Color4(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff, (i >> 24) & 0xff);
   }
   static Color4 FromString(std::string const& str) {
      const char* c = str.c_str();
      int len = str.length();
      if (*c == '#') {
         int val = strtol(c + 1, nullptr, 16);
         switch (len - 1) {
         case 6: // #ffcc00
            return Color4((val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff, 255);
         case 8: // #ffcc0080
            return Color4((val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
         }
      }
      return Color4(0, 0, 0, 255);
   }
};

std::ostream& operator<<(std::ostream& out, const Color4 &c);


class Color3 {
public:
   unsigned char r, g, b;
   Color3() { }
   Color3(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) { }

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
   std::string ToString() const {
      std::stringstream out;
      out << std::hex << std::setfill('0') << std::setw(2) << '#' << r << g << b;
      return out.str();
   }
   static Color3 FromInteger(unsigned int i) {
      return Color3(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff);
   }
   static Color3 FromString(std::string const& str) {
      Color4 c = Color4::FromString(str);
      return Color3(c.r, c.g, c.b);
   }

   static Color3 red;
   static Color3 orange;
   static Color3 yellow;
   static Color3 green;
   static Color3 blue;
   static Color3 black;
   static Color3 white;
};
std::ostream& operator<<(std::ostream& out, const Color3& source);

struct Histogram {
   static Color3 Sample(float f);
};

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Color3, Protocol::color)
IMPLEMENT_DM_EXTENSION(::radiant::csg::Color4, Protocol::color)

#endif
