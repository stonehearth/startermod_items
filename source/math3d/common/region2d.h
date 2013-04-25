#ifndef _RADIANT_MATH3D_REGION_2D_H
#define _RADIANT_MATH3D_REGION_2D_H

#include <vector>
#include "rect2d.h"
#include "line2d.h"
#include "radiant.pb.h"
#include "store.pb.h"
#include "dm/dm.h"

namespace radiant { namespace math2d {

class Region2d {
public:
   Region2d();
   Region2d(const Region2d &other);
   Region2d(const Region2d &&other);
   explicit Region2d(math2d::CoordinateSystem cs);
   explicit Region2d(const Rect2d &r);

   void SaveValue(protocol::region2 *rgn) const;
   void Encode(protocol::shapelist *msg, const math3d::ipoint3& offset, const math3d::color4& c) const;
   void LoadValue(const protocol::region2 &msg);

   bool IsEmpty() const;
   bool Contains(int x, int y) const;
   bool Contains(const ipoint2 &pt) const;
   bool Intersects(const Rect2d &r) const;
   bool Intersects(const Region2d &r) const;
   bool PointOnBorder(const Point2d &pt) const;

   // 3D support
   void Set3dPosition(int other) { lastCoord_ = other; }
   int Get3dPosition() const { return lastCoord_; }

   unsigned int GetSize() const;
   const Rect2d &GetRect(unsigned int i) const;

   Rect2d GetBoundingBox() const;
   CoordinateSystem GetCoordinateSystem() const { return space_; }

   Region2d Grow(int amount) const;
   Region2d GrowCardinal(int amount) const;
   Region2d Inset(int amount) const;
   Region2d Offset(const Point2d &amount) const;
   Region2d ProjectOnto(CoordinateSystem cs, int width) const;
   Region2d GetVerticalSlice(int y) const;
   std::vector<Line2d> GetPerimeter() const;

   std::vector<Rect2d>::const_iterator begin() const;
   std::vector<Rect2d>::const_iterator end() const;

   Region2d operator-(const Rect2d &r) const;
   Region2d operator-(const Region2d &other) const;

   bool operator==(const Region2d& other) const;
   bool operator!=(const Region2d& other) const;
   Region2d& operator=(Region2d&& other);
   Region2d& operator-=(const Region2d &r);
   Region2d& operator-=(const Rect2d &r);
   Region2d& operator-=(const Point2d &r);
   Region2d& operator+=(const Rect2d &r);
   Region2d& operator+=(const Point2d &r);

   void Clear();
   void SetCoordinateSystem(CoordinateSystem cs);

protected:
   void Optimize();
   bool MergeRects(Rect2d &a, const Rect2d& b);
   void SubtractRect(const Rect2d& r);
   void AddRect(const Rect2d& r);

protected:
   int               lastCoord_;
   std::vector<Rect2d>    rects_;
   CoordinateSystem  space_;
};
std::ostream& operator<<(std::ostream& out, const Region2d& source);

} }

IMPLEMENT_DM_EXTENSION(::radiant::math2d::Region2d, Protocol::region2)

#endif
