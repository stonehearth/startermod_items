#include "radiant.h"
#include "color.h"

using namespace radiant;
using namespace radiant::math3d;

color3 color3::red(255, 0, 0);
color3 color3::orange(255, 165, 0);
color3 color3::yellow(255, 239, 0);
color3 color3::green(34, 139, 34);
color3 color3::blue(0, 0, 205);

static const math3d::color3 __histogram[] = {
   color3::red,
   color3::orange,
   color3::yellow,
   color3::green,
   color3::blue
};

color3 Histogram::Sample(float f)
{
   f = std::min(std::max(f, 0.0f), 1.0f);

   int range = ARRAYSIZE(__histogram) - 1;
   float position = range * f;
   int offset = (int)position;
   float alpha = std::min(std::max(position - offset, 0.0f), 1.0f);

   const color3 &first  = __histogram[offset];
   const color3 &second = __histogram[offset+1];

   color3 result;
   for (int i = 0; i < 3; i++) {
      result[i] = (unsigned char)((first[i] * (1 - alpha)) + (second[i] * alpha));
   }
   return result;
}

std::ostream& ::radiant::math3d::operator<<(std::ostream& out, const color4 &c)
{
   out << "RGBA(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (int)c.a << ")";
   return out;
}

std::ostream& ::radiant::math3d::operator<<(std::ostream& out, const color3 &c)
{
   out << "RGBA(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << ")";
   return out;    
}
