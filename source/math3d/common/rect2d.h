#ifndef _RADIANT_MATH3D_RECT_2D_H
#define _RADIANT_MATH3D_RECT_2D_H

#include "ipoint2.h"
#include "radiant.pb.h"
#include <functional>

namespace radiant { namespace math3d {
class ibounds3;
class ipoint3;
class color4;
} }

namespace radiant { namespace math2d {

class Region2d;

class Rect2d {
public:
   Rect2d();
   Rect2d(CoordinateSystem cs);
   Rect2d(const Rect2d &other);
   Rect2d(const protocol::rect2 &msg);
   Rect2d(const Point2d& p0, const Point2d& p1);
   Rect2d(int x0, int y0, int x1, int y1, CoordinateSystem cs);
   Rect2d(const math3d::ibounds3 &b, CoordinateSystem cs);
   Rect2d(const math3d::ipoint3& p0, const math3d::ipoint3& p1, CoordinateSystem cs);

   static Rect2d MakeRect(int x0, int y0, int x1, int y1, CoordinateSystem cs);
   static Rect2d MakeRect(const Point2d& p0, const Point2d& p1, CoordinateSystem cs);

   void Encode(protocol::rect2 *model) const;
   void Encode(protocol::quad *quad, const math3d::ipoint3& offset, const math3d::color4& c) const;
   void Decode(const protocol::rect2 &msg);

   Point2d GetP0() const { return p0; }
   Point2d GetP1() const { return p1; }
   int GetHeight() const { return p1.y - p0.y; }
   int GetWidth() const { return p1.x - p0.x; }
   CoordinateSystem GetCoordinateSystem() const { return p0.GetCoordinateSystem(); }

   void SetZero();
   bool Contains(int x, int y, CoordinateSystem) const;
   bool Contains(const ipoint2 &pt) const;
   bool Intersects(const Rect2d &other) const;
   Point2d GetClosestPoint(const Point2d& pt) const;

   Rect2d Union(const Rect2d &other) const;
   Rect2d Inset(int amount) const;
   Rect2d Grow(int amount) const;
   Rect2d ProjectOnto(CoordinateSystem cs, int width) const;
   Rect2d GetVerticalSlice(int y) const;

   int GetArea() const;

   void IterateBorderPoints(std::function <void(Point2d)> fn) const;

   Rect2d operator+(const ipoint2 &pt) const;
   Rect2d operator-(const ipoint2 &pt) const;
   Region2d operator-(const Rect2d &other) const;
   Region2d operator-(const Region2d &other) const;
   bool operator==(const Rect2d &other) const;

   Point2d p0, p1;
};

} }

#endif
