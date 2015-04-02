#ifndef _RADIANT_CSG_COLOR_H
#define _RADIANT_CSG_COLOR_H

#include <ostream>
#include <iomanip>
#include <sstream>
#include "namespace.h"
#include "protocols/forward_defines.h"
#include "dm/dm.h"
#include "point.h"

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

   bool operator==(const Color4& other) const {
      return ToInteger() == other.ToInteger();
   }

   bool operator!=(const Color4& other) const {
      return ToInteger() != other.ToInteger();
   }

   bool operator<(const Color4& other) const {
      return ToInteger() < other.ToInteger();
   }

   void operator*=(float s) {
      s = std::min(std::max(s, 1.0f), 0.0f);
      for (int i = 0; i < 4; i++) {
         (*this)[i] = (int)((*this)[i] * s);
      }
   }

   void LoadValue(const protocol::color& c);
   void SaveValue(protocol::color *c) const;

   Point3f ToHsl() const {
      const Point3f nRGB(r / 255.0, g / 255.0, b / 255.0);
      const double cMax = std::max(std::max(nRGB.x, nRGB.y), nRGB.z);
      const double cMin = std::min(std::min(nRGB.x, nRGB.y), nRGB.z);
      const double delta = cMax - cMin;
      Point3f result(0, 0, (cMax + cMin) / 2.0);

      if (delta != 0) {
         result.y = result.z > 0.5f ? delta / (2.0f - cMax - cMin) : delta / (cMax + cMin);
         if (cMax == nRGB.x) {
            result.x = (nRGB.y - nRGB.z) / delta + (nRGB.y < nRGB.z ? 6.0f : 0.0f);
         } else if (cMax == nRGB.y) {
            result.x = (nRGB.z - nRGB.x) / delta + 2.0f;
         } else {
            result.x = (nRGB.x - nRGB.y) / delta + 4.0f;
         }
         result.x /= 6.0;
      }
      return result;
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
      size_t len = str.length();
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

   struct Hash { 
      inline size_t operator()(Color4 const& c) const {
         return static_cast<size_t>(c.r * 73856093) ^
                static_cast<size_t>(c.g * 19349669) ^
                static_cast<size_t>(c.b * 83492791) ^
                static_cast<size_t>(c.a * 22121887);
      }
   };


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

   bool operator==(const Color4& other) const {
      return ToInteger() == other.ToInteger();
   }

   bool operator!=(const Color4& other) const {
      return ToInteger() != other.ToInteger();
   }

   bool operator<(const Color4& other) const {
      return ToInteger() < other.ToInteger();
   }


   void operator*=(float s) {
      s = std::min(std::max(s, 1.0f), 0.0f);
      for (int i = 0; i < 3; i++) {
         (*this)[i] = (int)((*this)[i] * s);
      }
   }

   void SaveValue(protocol::color *c) const;
   void LoadValue(const protocol::color& c);

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

   struct Hash { 
      inline size_t operator()(Color3 const& c) const {
         return static_cast<size_t>(c.r * 73856093) ^
                static_cast<size_t>(c.g * 19349669) ^
                static_cast<size_t>(c.b * 83492791);
      }
   };

   static Color3 red;
   static Color3 orange;
   static Color3 yellow;
   static Color3 green;
   static Color3 blue;
   static Color3 black;
   static Color3 white;
   static Color3 grey;
   static Color3 brown;
   static Color3 purple;
   static Color3 pink;
};

std::ostream& operator<<(std::ostream& out, const Color3& source);

struct Histogram {
   static Color3 Sample(float f);
};

END_RADIANT_CSG_NAMESPACE

#endif
