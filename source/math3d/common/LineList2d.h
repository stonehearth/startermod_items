#ifndef _RADIANT_MATH3D_LINE_LIST_2D_H
#define _RADIANT_MATH3D_LINE_LIST_2D_H

#include <vector>
#include "rect2d.h"
#include "radiant.pb.h"

namespace radiant { namespace math2d {

class LineList2d {
public:
   LineList2d();
   LineList2d(const LineList2d &other);
   LineList2d(const LineList2d &&other);
   explicit LineList2d(math2d::CoordinateSystem cs);
   explicit LineList2d(const Rect2d &r);

   void Encode(protocol::region2 *model) const;
   void Decode(const protocol::region2 &msg);

   bool IsEmpty() const;
   bool Contains(int x, int y, CoordinateSystem c) const;
   bool Contains(const ipoint2 &pt) const;
   bool Intersects(const Rect2d &r) const;
   bool Intersects(const LineList2d &r) const;
   bool PointOnBorder(const Point2d &pt) const;

   unsigned int GetSize() const;
   const Rect2d &GetRect(unsigned int i) const;

   Rect2d GetBoundingBox() const;
   CoordinateSystem GetCoordinateSystem() const { return space_; }

   LineList2d Grow(int amount) const;
   LineList2d GrowCardinal(int amount) const;
   LineList2d Inset(int amount) const;
   LineList2d Offset(const Point2d &amount) const;
   LineList2d ProjectOnto(CoordinateSystem cs, int width) const;
   vector<Point2d> GetPerimeter() const;

   vector<Rect2d>::const_iterator begin() const;
   vector<Rect2d>::const_iterator end() const;

   LineList2d operator-(const Rect2d &r) const;
   LineList2d operator-(const LineList2d &other) const;

   LineList2d& operator=(LineList2d&& other);
   LineList2d& operator-=(const LineList2d &r);
   LineList2d& operator-=(const Rect2d &r);
   LineList2d& operator+=(const Rect2d &r);

   void Clear();
   void SetCoordinateSystem(CoordinateSystem cs);

protected:
   vector<Rect2d>    rects_;
   CoordinateSystem  space_;
};

} }

#endif
