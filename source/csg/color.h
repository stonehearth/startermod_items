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
   Color4(unsigned int i)  { (*this) = FromInteger(i); }
   Color4(const char* s)  { (*this) = FromString(s); }
   Color4(const protocol::color& c) {
      LoadValue(c);
   }


   unsigned char operator[](int i) const {
      return (&r)[i];
   }

   unsigned char &operator[](int i) {
      return (&r)[i];
   }

   bool operator==(const Color4& other) const;
   bool operator!=(const Color4& other) const;
   bool operator<(const Color4& other) const;
   void operator*=(float s);

   void LoadValue(const protocol::color& c);
   void SaveValue(protocol::color *c) const;

   Point3f ToHsl() const;
   int ToInteger() const;
   std::string ToString() const;
   static Color4 FromInteger(unsigned int i);
   static Color4 FromString(std::string const& str);

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
   Color3(unsigned int i)  { (*this) = FromInteger(i); }
   Color3(const char* s)  { (*this) = FromString(s); }
   Color3(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) { }

   unsigned char operator[](int i) const {
      return (&r)[i];
   }

   unsigned char &operator[](int i) {
      return (&r)[i];
   }

   bool operator==(const Color3& other) const;
   bool operator!=(const Color3& other) const;
   bool operator<(const Color3& other) const;
   void operator*=(float s);

   void SaveValue(protocol::color *c) const;
   void LoadValue(const protocol::color& c);

   int ToInteger() const;
   std::string ToString() const;
   static Color3 FromInteger(unsigned int i);
   static Color3 FromString(std::string const& str);

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
