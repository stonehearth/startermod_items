#ifndef _RADIANT_MATH3D_LINE_2D_H
#define _RADIANT_MATH3D_LINE_2D_H

#include "ipoint2.h"
#include "radiant.pb.h"

namespace radiant { namespace math3d {
class ibounds3;
class ipoint3;
class color4;
} }

namespace radiant { namespace math2d {

class Region2d;

class Line2d {
public:
   Line2d();
   Line2d(const Line2d &other);
   Line2d(const Point2d& p0, const Point2d& p1);
   Line2d(int x0, int y0, int x1, int y1, CoordinateSystem cs);
   Line2d(const math3d::ipoint3& p0, const math3d::ipoint3& p1, CoordinateSystem cs);

   static Line2d MakeLine(int x0, int y0, int x1, int y1, CoordinateSystem cs);
   static Line2d MakeLine(const Point2d& p0, const Point2d& p1, CoordinateSystem cs);

   bool LengthIsZero() const { return p0 == p1; }
   Point2d GetP0() const { return p0; }
   Point2d GetP1() const { return p1; }
   Point2d GetMidpoint() const { return Point2d((p1.x + p0.x) / 2, (p1.y + p0.y) / 2, p0.GetCoordinateSystem()); }
   CoordinateSystem GetCoordinateSystem() const { return p0.GetCoordinateSystem(); }

   void SetZero();
   bool RemoveOverlap(Line2d& other);

   Point2d p0, p1;
};

} }

#endif
