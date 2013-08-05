#ifndef _RADIANT_CSG_BOX_H
#define _RADIANT_CSG_BOX_H

#include "namespace.h"
#include "math3d.h"
#include "point.h"
#include "math3d.h"

BEGIN_RADIANT_CSG_NAMESPACE

template <typename S, int C> class Region;

template <typename S, int C>
class Cube
{
public:
   typedef S ScalarType;
   typedef Point<S, C> Point;
   typedef Region<S, C> Region;
public:
   static Cube<S, C> zero;
   static Cube<S, C> one;

public:
   Cube();
   Cube(const Point& max, int tag = 0);
   Cube(const Point& min, const Point& max, int tag = 0);
   Cube(const protocol::cube& msg);

   static Cube Construct(Point min, Point max, int tag = 0);

   // nuke these...
   Cube(const math3d::aabb& a) : min_(a._min), max_(a._max) { }
   operator math3d::aabb() const { return math3d::aabb(min_, max_); }

   struct PointIterator {
      PointIterator(const Cube& c, const Point& iter) :
         min_(c.GetMin()),
         max_(c.GetMax()),
         iter_(iter),
         axis_(0)
      {
      }

      const Point operator*() const {
         return iter_;
      }
      const void operator++() {
         iter_[2]++;
         if (iter_[2] == max_[2]) {
            iter_[2] = min_[2];
            iter_[0]++;
            if (iter_[0] == max_[0]) {
               iter_[2] = min_[2];
               iter_[0] = min_[0];
               iter_[1]++;
            }
         }
      }
      bool operator!=(const PointIterator& rhs) {
         return iter_ != rhs.iter_;
      }

   private:
      Point    min_;
      Point    max_;
      Point    iter_;
      int      axis_;
   };

   PointIterator begin() const { return PointIterator(*this, GetMin()); }
   PointIterator end() const { return PointIterator(*this, Point(min_[0], max_[1], min_[2])); }

   // nuke these
   Cube(const math3d::ibounds3& b) : min_(b._min), max_(b._max) { }
   operator math3d::ibounds3() const { return math3d::ibounds3(min_, max_); }

   S GetArea() const;
   bool IsEmpty() const { return GetArea() == 0; }
   const Point& GetMin() const { return min_; }
   const Point& GetMax() const { return max_; }
   void SetMin(const Point& min) { min_ = min; }
   void SetMax(const Point& max) { max_ = max; }

   void Grow(const Point& pt);

   template <class U> void Translate(const U& pt) {
      for (int i = 0; i < C; i++) {
         min_[i] += static_cast<S>(pt[i]);
         max_[i] += static_cast<S>(pt[i]);
      }
   }

   Cube Scale(S factor) { return Cube(min_.Scale(factor), max_.Scale(factor)); }
   Cube ProjectOnto(int axis, S plane) const;

   bool Intersects(const Cube& other) const;
   bool Contains(const Point& pt) const;
   Point GetClosestPoint2(const Point& other, S* dSquared) const;
   Point GetClosestPoint(const Point& other) const { return GetClosestPoint2(other, nullptr); }
   Point GetSize() const { return GetMax() - GetMin(); }

   // optimizing...
   Cube operator&(const Cube& other) const;
   Region operator&(const Region& other) const;
   Cube operator+(const Point& other) const;
   Region operator-(const Cube& other) const;
   Region operator-(const Region& other) const;

   std::ostream& Format(std::ostream& os) const {
      os << "( " << min_ << " - " << max_ << " )";
      return os;
   }

   S GetWidth() const { return (GetMax() - GetMin())[0]; }
   S GetHeight() const { return (GetMax() - GetMin())[1]; }

   int GetTag() const { return tag_; }
   void SetTag(int tag) { tag_ = tag; }

public:
   void SaveValue(protocol::cube* msg) const {
      min_.SaveValue(msg->mutable_min());
      max_.SaveValue(msg->mutable_max());
      msg->set_tag(tag_);
   }
   void LoadValue(const protocol::cube& msg) {
      tag_ = msg.tag();
      min_.LoadValue(msg.min());
      max_.LoadValue(msg.max());
   }

private:
   Point    min_;
   Point    max_;
   int      tag_;
};

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, const Cube<S, C>& in)
{
   return in.Format(os);
}

typedef Cube<int, 1> Line1;
typedef Cube<int, 2> Rect2;
typedef Cube<int, 3> Cube3;
typedef Cube<float, 3> Cube3f;

bool Cube3Intersects(const Cube3& rgn, const math3d::ray3& ray, float& distance);

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Cube3, Protocol::cube)

#endif // _RADIANT_CSG_BOX_H