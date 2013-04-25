#include "radiant.h"
#include "line2d.h"
#include "math3d.h"
#include "common.h"

using namespace ::radiant;
using namespace ::radiant::math2d;
namespace protocol = ::radiant::protocol;

Line2d::Line2d()
{
}

Line2d::Line2d(const Line2d &other) :
   p0(other.p0), p1(other.p1)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
   ASSERT(p0.x <= p1.x && p0.y <= p1.y);
}

Line2d::Line2d(const Point2d& _p0, const Point2d& _p1) :
   p0(_p0), p1(_p1)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
}

Line2d::Line2d(int x0, int y0, int x1, int y1, CoordinateSystem cs) :
   p0(x0, y0, cs), p1(x1, y1, cs)
{
   ASSERT(p0.GetCoordinateSystem() == p1.GetCoordinateSystem());
   ASSERT(p0.x <= p1.x && p0.y <= p1.y);
}

Line2d::Line2d(const math3d::ipoint3& _p0, const math3d::ipoint3& _p1, CoordinateSystem cs)
{
   p0 = Point2d(_p0, cs);
   p1 = Point2d(_p1, cs);
}

Line2d Line2d::MakeLine(int x0, int y0, int x1, int y1, CoordinateSystem cs)
{
   if (x0 > x1) {
      std::swap(x0, x1);
   }
   if (y0 > y1) {
      std::swap(y0, y1);
   }
   return Line2d(x0, y0, x1, y1, cs);
}

Line2d Line2d::MakeLine(const Point2d& p0, const Point2d& p1, CoordinateSystem cs)
{
   return MakeLine(p0.x, p0.y, p1.x, p1.y, cs);
}

void Line2d::SetZero()
{
   p0.set_zero();
   p1.set_zero();
}

bool Line2d::RemoveOverlap(Line2d& other)
{
   int x, y;

   if (p0.y == p1.y) {
      x = 0;
      y = 1;
   } else {
      x = 1;
      y = 0;
   }
   // xxx: only works with horizontal and vertical lines at this time.
   ASSERT(p0[y] == p1[y]);

   if (other.p1[y] != p0[y] || other.p1[y] != p0[y]) {
      // Not on the same y coordinate.  No overlap.
      return false;
   }
   if (p0[x] >= other.p1[x]) {
      // off the right side...
      return false;
   }
   if (p1[x] <= other.p0[x]) {
      // off the left side...
      return false;
   }

   // Definitly overlaps! Make sure *this points to the left-most line.
   if (other.p0[x] < p0[x]) {
      std::swap(*this, other);
   }

   // *this will contain the left side of the line.  other the right
   p1 = other.p0;
   other.p0 = p1;

   return true;
}
