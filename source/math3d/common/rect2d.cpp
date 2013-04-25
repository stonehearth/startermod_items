#include "radiant.h"
#include "rect2d.h"
#include "region2d.h"
#include "math3d.h"
#include "common.h"
#include "aabb.h"

using namespace ::radiant;
using namespace ::radiant::math2d;
namespace protocol = ::radiant::protocol;

Rect2d::Rect2d()
{
}

Rect2d::Rect2d(CoordinateSystem cs)
{
   SetZero();
   p0.space_ = cs;
   p1.space_ = cs;
}

Rect2d::Rect2d(const Rect2d &other) :
   p0(other.p0), p1(other.p1)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
   ASSERT(p0.x <= p1.x && p0.y <= p1.y);
}

Rect2d::Rect2d(const Point2d& _p0, const Point2d& _p1) :
   p0(_p0), p1(_p1)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
}

Rect2d::Rect2d(int x0, int y0, int x1, int y1, CoordinateSystem cs) :
   p0(x0, y0, cs), p1(x1, y1, cs)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
   ASSERT(p0.x <= p1.x && p0.y <= p1.y);
}

Rect2d::Rect2d(const radiant::math3d::ibounds3 &b, CoordinateSystem cs)
{
   p0 = Point2d(b._min, cs);
   p1 = Point2d(b._max, cs);
}

Rect2d::Rect2d(const math3d::ipoint3& _p0, const math3d::ipoint3& _p1, CoordinateSystem cs)
{
   p0 = Point2d(_p0, cs);
   p1 = Point2d(_p1, cs);
}

Rect2d::Rect2d(const protocol::rect2 &msg)
{
   Decode(msg);
}

Rect2d Rect2d::MakeRect(int x0, int y0, int x1, int y1, CoordinateSystem cs)
{
   if (x0 > x1) {
      std::swap(x0, x1);
   }
   if (y0 > y1) {
      std::swap(y0, y1);
   }
   return Rect2d(x0, y0, x1, y1, cs);
}

Rect2d Rect2d::MakeRect(const Point2d& p0, const Point2d& p1, CoordinateSystem cs)
{
   return MakeRect(p0.x, p0.y, p1.x, p1.y, cs);
}

void Rect2d::SetZero()
{
   p0.set_zero();
   p1.set_zero();
}

void Rect2d::Encode(protocol::rect2 *msg) const
{
   msg->set_x0(p0.x);
   msg->set_y0(p0.y);
   msg->set_x1(p1.x);
   msg->set_y1(p1.y);
   msg->set_cs(GetCoordinateSystem());
}

void Rect2d::Encode(protocol::quad *quad, const math3d::ipoint3& offset, const math3d::color4& c) const
{
   CoordinateSystem cs = GetCoordinateSystem();
   math3d::ipoint3(Point2d(p0.x, p0.y, cs), offset).SaveValue(quad->mutable_p0());
   math3d::ipoint3(Point2d(p0.x, p1.y, cs), offset).SaveValue(quad->mutable_p1());
   math3d::ipoint3(Point2d(p1.x, p1.y, cs), offset).SaveValue(quad->mutable_p2());
   math3d::ipoint3(Point2d(p1.x, p0.y, cs), offset).SaveValue(quad->mutable_p3());
   c.SaveValue(quad->mutable_color());
}

void Rect2d::Decode(const protocol::rect2 &msg)
{
   p0.x = msg.x0();
   p0.y = msg.y0();
   p0.space_ = (CoordinateSystem)msg.cs();
   p1.x = msg.x1();
   p1.y = msg.y1();
   p1.space_ = (CoordinateSystem)msg.cs();
}

bool Rect2d::Contains(int x, int y, CoordinateSystem cs) const
{
   ASSERT(cs == GetCoordinateSystem());
   return x >= p0.x && y >= p0.y && x < p1.x && y < p1.y;
}

bool Rect2d::Contains(const ipoint2 &pt) const
{
   ASSERT(pt.GetCoordinateSystem() == GetCoordinateSystem());
   return Contains(pt.x, pt.y, pt.GetCoordinateSystem());
}

bool Rect2d::Intersects(const Rect2d &other) const
{
   ASSERT(GetCoordinateSystem() == other.GetCoordinateSystem());
   return other.p0.x < p1.x && other.p1.x > p0.x && other.p0.y < p1.y && other.p1.y > p0.y;
}

Point2d Rect2d::GetClosestPoint(const Point2d& dst) const
{
   int x = std::max(std::min(dst.x, p1.x - 1), p0.x);
   int y = std::max(std::min(dst.y, p1.y - 1), p0.y);

   return Point2d(x, y, GetCoordinateSystem());
}

Rect2d Rect2d::Union(const Rect2d &other) const
{
   ASSERT(GetCoordinateSystem() == other.GetCoordinateSystem());

   int ix0 = std::min(p0.x, other.p0.x);
   int iy0 = std::min(p0.y, other.p0.y);
   int ix1 = std::max(p1.x, other.p1.x);
   int iy1 = std::max(p1.y, other.p1.y);

   return Rect2d(ix0, iy0, ix1, iy1, GetCoordinateSystem());
}

Rect2d Rect2d::operator+(const ipoint2 &pt) const
{
   ASSERT(GetCoordinateSystem() == pt.GetCoordinateSystem());
   Rect2d other(*this);

   other.p0.x += pt.x;
   other.p1.x += pt.x;
   other.p0.y += pt.y;
   other.p1.y += pt.y;

   return other;
}

Rect2d Rect2d::operator-(const ipoint2 &pt) const
{
   ASSERT(GetCoordinateSystem() == pt.GetCoordinateSystem());
   Rect2d other(*this);

   other.p0.x -= pt.x;
   other.p1.x -= pt.x;
   other.p0.y -= pt.y;
   other.p1.y -= pt.y;

   return other;
}

Region2d Rect2d::operator-(const Region2d &other) const
{
   ASSERT(GetCoordinateSystem() == other.GetCoordinateSystem());

   return Region2d(*this) - other;
}

Region2d Rect2d::operator-(const Rect2d &other) const
{
   ASSERT(GetCoordinateSystem() == other.GetCoordinateSystem());


   int ix0 = std::max(p0.x, other.p0.x);
   int iy0 = std::max(p0.y, other.p0.y);
   int ix1 = std::min(p1.x, other.p1.x);
   int iy1 = std::min(p1.y, other.p1.y);
   CoordinateSystem cs = GetCoordinateSystem();

   if (ix0 >= ix1 || iy0 >= iy1) {
      return Region2d(*this);
   }

   Region2d result(cs);

   if (ix0 == p0.x) {
      if (ix1 == p1.x) {  
         // entire horizonal span is covered...
         if (iy0 == p0.y) {
            if (iy1 == p1.y) {
               //...cutting out the whole thing!             
            } else {
               // ...leaving 1 box on the bottom.
               result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
            }
         } else {
            if (iy1 == p1.y) {
               // ...leaving 1 box on the top.
               result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
            } else {
               // ...leaving 2 boxes.
               result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
               result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
            }
         }
      } else {
         // left horizonal region is covered...
         if (iy0 == p0.y) {
            if (iy1 == p1.y) {
               // ...leaving 1 box on the right
               result += Rect2d(ix1, p0.y, p1.x, p1.y, cs);
            } else {
               // ...snipping out the top left corner
               result += Rect2d(ix1, p0.y, p1.x, iy1, cs);
               result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
            }
         } else if (iy1 == p1.y) {
            // ...snipping out the bottom left corner
            result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
            result += Rect2d(ix1, iy0, p1.x, p1.y, cs);
         } else {
            // ... leaving a reverse C type pattern on the right
            result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
            result += Rect2d(ix1, iy0, p1.x, iy1, cs);
            result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
         }
      }
   } else if (ix1 == p1.x) {
      // right horizontal region is covered...
      if (iy0 == p0.y) {
         if (iy1 == p1.y) {
            // ...leaving 1 box on the left
            result += Rect2d(p0.x, p0.y, ix0, p1.y, cs);
         } else {
            // ...snipping out the top right corner
            result += Rect2d(p0.x, p0.y, ix0, iy1, cs);
            result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
         }
      } else {
         if (iy1 == p1.y) {
            // ...snipping out the bottom right corner
            result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
            result += Rect2d(p0.x, iy0, ix0, p1.y, cs);
         } else {
            // ...leaving a C type pattern
            result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
            result += Rect2d(p0.x, iy0, ix0, iy1, cs);
            result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
         }
      }
   } else {
      // middle region is covered...
      if (p0.y == iy0) {
         if (p1.y == iy1) {
            // ...cutting the box in half vertically
            result += Rect2d(p0.x, p0.y, ix0, p1.y, cs);
            result += Rect2d(ix1, p0.y, p1.x, p1.y, cs);
         } else {
            // ... leaving a U shaped pattern.
            result += Rect2d(p0.x, p0.y, ix0, iy1, cs);
            result += Rect2d(ix1, p0.y, p1.x, iy1, cs);
            result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
         }
      } else if (p1.y == iy1) {
         // ...leaving an upside-down U pattern
         result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
         result += Rect2d(p0.x, iy0, ix0, iy1, cs);
         result += Rect2d(ix1, iy0, p1.x, p1.y, cs);
      } else {
         // ...cutting a hole in the middle, leaving an O shape.
         result += Rect2d(p0.x, p0.y, p1.x, iy0, cs);
         result += Rect2d(p0.x, iy0, ix0, iy1, cs);
         result += Rect2d(ix1, iy0, p1.x, iy1, cs);
         result += Rect2d(p0.x, iy1, p1.x, p1.y, cs);
      }
   }
   return result;
}

bool Rect2d::operator==(const Rect2d &other) const
{
   ASSERT(GetCoordinateSystem() == other.GetCoordinateSystem());
   return p0.x == other.p0.x && p0.y == other.p0.y && p1.x == other.p1.x && p1.y == other.p1.y;
}

Rect2d Rect2d::Inset(int amount) const
{
   int _x0 = p0.x + amount;
   int _y0 = p0.y + amount;
   int _x1 = std::max(_x0, p1.x - amount);
   int _y1 = std::max(_y0, p1.y - amount);

   return Rect2d(_x0, _y0, _x1, _y1, GetCoordinateSystem());
}

Rect2d Rect2d::Grow(int amount) const
{
   int _x0 = p0.x - amount;
   int _y0 = p0.y - amount;
   int _x1 = p1.x + amount;
   int _y1 = p1.y + amount;

   return Rect2d(_x0, _y0, _x1, _y1, GetCoordinateSystem());
}

Rect2d Rect2d::ProjectOnto(CoordinateSystem cs, int width) const
{
   if (cs == GetCoordinateSystem()) {
      return *this;
   }
   return MakeRect(p0.ProjectOnto(cs, 0), p1.ProjectOnto(cs, width), cs);
}

Rect2d Rect2d::GetVerticalSlice(int y) const
{   
   auto cs = GetCoordinateSystem();
   ASSERT(cs != math2d::XZ_PLANE);

   // "y" is the y coordinate in both the YZ and XY coodinate systems
   if (y < p0.y || y >= p1.y) {
      return Rect2d(0, 0, 0, 0, math2d::XZ_PLANE);
   }
   if (cs == math2d::YZ_PLANE) {
      return Rect2d(0, p0.x, 0, p1.x, math2d::XZ_PLANE);
   }
   return Rect2d(p0.x, 0, p1.x, 1, math2d::XZ_PLANE);
}

void Rect2d::IterateBorderPoints(std::function <void(Point2d)> fn) const
{
   CoordinateSystem cs = GetCoordinateSystem();
   for (int x = p0.x; x <= p1.x; x++) {
      fn(Point2d(x, p0.y, GetCoordinateSystem()));
      fn(Point2d(x, p1.y - 1, cs));
   }
   for (int y = p0.y + 1; y < p1.y - 1; y++) {
      fn(Point2d(p0.x, y, GetCoordinateSystem()));
      fn(Point2d(p1.x - 1, y, cs));
   }
}

int Rect2d::GetArea() const
{
   return (p1.x - p0.x) * (p1.y - p0.y);
}
