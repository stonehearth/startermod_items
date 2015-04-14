#include "pch.h"
#include "color.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::csg;

Color3 Color3::red(255, 0, 0);
Color3 Color3::orange(255, 165, 0);
Color3 Color3::yellow(255, 239, 0);
Color3 Color3::green(34, 139, 34);
Color3 Color3::blue(0, 0, 205);
Color3 Color3::black(0, 0, 0);
Color3 Color3::white(255, 255, 255);
Color3 Color3::grey(128, 128, 128);
Color3 Color3::brown(139, 69, 19);
Color3 Color3::purple(138, 43, 226);
Color3 Color3::pink(255, 192, 203);

static const csg::Color3 __histogram[] = {
   Color3::red,
   Color3::orange,
   Color3::yellow,
   Color3::green,
   Color3::blue
};

Color3 Histogram::Sample(float f)
{
   f = std::min(std::max(f, 0.0f), 1.0f);

   int range = ARRAYSIZE(__histogram) - 1;
   float position = range * f;
   int offset = (int)position;
   float alpha = std::min(std::max(position - offset, 0.0f), 1.0f);

   const Color3 &first  = __histogram[offset];
   const Color3 &second = __histogram[offset+1];

   Color3 result;
   for (int i = 0; i < 3; i++) {
      result[i] = (unsigned char)((first[i] * (1 - alpha)) + (second[i] * alpha));
   }
   return result;
}

void Color4::LoadValue(const protocol::color& c) {
   r = c.r();
   g = c.g();
   b = c.b();
   a = c.a();
}

void Color4::SaveValue(protocol::color *c) const {
   c->set_r(r);
   c->set_g(g);
   c->set_b(b);
   c->set_a(a);
}

void Color3::SaveValue(protocol::color *c) const {
   c->set_r(r);
   c->set_g(g);
   c->set_b(b);
}

void Color3::LoadValue(const protocol::color& c) {
   r = c.r();
   g = c.g();
   b = c.b();
}

std::ostream& ::radiant::csg::operator<<(std::ostream& out, const Color4 &c)
{
   return (out << '#'
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.r
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.g
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.b
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.a);
}

std::ostream& ::radiant::csg::operator<<(std::ostream& out, const Color3 &c)
{
   return (out << '#'
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.r
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.g
               << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.b);
}


bool Color4::operator==(const Color4& other) const
{
   return ToInteger() == other.ToInteger();
}

bool Color4::operator!=(const Color4& other) const
{
   return ToInteger() != other.ToInteger();
}

bool Color4::operator<(const Color4& other) const
{
   return ToInteger() < other.ToInteger();
}

void Color4::operator*=(float s)
{
   s = std::min(std::max(s, 1.0f), 0.0f);
   for (int i = 0; i < 4; i++) {
      (*this)[i] = (int)((*this)[i] * s);
   }
}

Point3f Color4::ToHsl() const {
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

int Color4::ToInteger() const
{
   return ((unsigned int)r) |
            (((unsigned int)g) << 8) |
            (((unsigned int)b) << 16) |
            (((unsigned int)a) << 24);
}

std::string Color4::ToString() const
{
   return BUILD_STRING(*this);
}

Color4 Color4::FromInteger(unsigned int i)
{
   return Color4(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff, (i >> 24) & 0xff);
}

Color4 Color4::FromString(std::string const& str)
{
   Color4 color(0, 0, 0, 255);
   const char* c = str.c_str();
   size_t len = str.length();
   if (*c == '#') {
      int val = strtol(c + 1, nullptr, 16);
      switch (len - 1) {
      case 6: // #ffcc00
         color = Color4((val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff, 255);
         break;
      case 8: // #ffcc0080
         color = Color4((val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
         break;
      }
   }
   return color;
}


bool Color3::operator==(const Color3& other) const
{
   return ToInteger() == other.ToInteger();
}

bool Color3::operator!=(const Color3& other) const
{
   return ToInteger() != other.ToInteger();
}

bool Color3::operator<(const Color3& other) const
{
   return ToInteger() < other.ToInteger();
}

void Color3::operator*=(float s)
{
   s = std::min(std::max(s, 1.0f), 0.0f);
   for (int i = 0; i < 3; i++) {
      (*this)[i] = (int)((*this)[i] * s);
   }
}

int Color3::ToInteger() const
{
   return ((unsigned int)r) |
          (((unsigned int)g) << 8) |
          (((unsigned int)b) << 16);
}

std::string Color3::ToString() const
{
   return BUILD_STRING(*this);
}

Color3 Color3::FromInteger(unsigned int i)
{
   return Color3(i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff);
}

Color3 Color3::FromString(std::string const& str)
{
   Color4 c = Color4::FromString(str);
   return Color3(c.r, c.g, c.b);
}

bool Color3::IsColor(std::string const& color)
{
   if (color.size() != 7 || color[0] != '#') {
      return false;
   }
   for (int i = 1; i < 7; i++) {
      char ch = color[i];
      if (ch >= '0' && ch <= '9') {
         continue;
      }
      if (ch >= 'A' && ch <= 'F') {
         continue;
      }
      return false;
   }
   return true;
}
