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
   out << "rgba(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << (int)c.a << ")";
   return out;
}

std::ostream& ::radiant::csg::operator<<(std::ostream& out, const Color3 &c)
{
   out << "RGBA(" << (int)c.r << ", " << (int)c.g << ", " << (int)c.b << ", " << ")";
   return out;    
}
