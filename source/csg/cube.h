#ifndef _RADIANT_CSG_BOX_H
#define _RADIANT_CSG_BOX_H

#include "namespace.h"
#include "point.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Ray;
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
   Cube(const Point& max_value, int tag = 0);
   Cube(const Point& min_value, const Point& max_value, int tag = 0);

   static Cube Construct(Point min_value, Point max_value, int tag = 0) {
      for (int i = 0; i < C; i++) {
         if (min_value[i] > max_value[i]) {
            std::swap(min_value[i], max_value[i]);
         }
      }
      return Cube(min_value, max_value, tag);
   }

   struct PointIterator {
      PointIterator(const Cube& c, const Point& iter) :
         min(c.GetMin()),
         max(c.GetMax()),
         iter_(iter),
         axis_(0)
      {
      }

      const Point operator*() const {
         return iter_;
      }
      const void operator++() {
         iter_[2]++;
         if (iter_[2] == max[2]) {
            iter_[2] = min[2];
            iter_[0]++;
            if (iter_[0] == max[0]) {
               iter_[2] = min[2];
               iter_[0] = min[0];
               iter_[1]++;
            }
         }
      }
      bool operator!=(const PointIterator& rhs) {
         return iter_ != rhs.iter_;
      }

   private:
      Point    min;
      Point    max;
      Point    iter_;
      int      axis_;
   };

   PointIterator begin() const { return PointIterator(*this, GetMin()); }
   PointIterator end() const { return PointIterator(*this, Point(min[0], max[1], min[2])); }

   S GetArea() const;
   bool IsEmpty() const { return GetArea() == 0; }
   const Point& GetMin() const { return min; }
   const Point& GetMax() const { return max; }
   void SetMin(const Point& min_value) { min = min_value; }
   void SetMax(const Point& max_value) { max = max_value; }
   void Grow(const Point& pt);

   void SetZero() {
      min.SetZero();
      max.SetZero();
   }

   template <class U> void Translate(const U& pt) {
      for (int i = 0; i < C; i++) {
         min[i] += static_cast<S>(pt[i]);
         max[i] += static_cast<S>(pt[i]);
      }
   }
   template <class U> Cube Translated(const U& pt) const {
      Cube result(*this);
      result.Translate(pt);
      return result;
   }

   Cube Scaled(float factor) { return Cube(min.Scaled(factor), max.Scaled(factor)); }
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
      os << "( " << min << " - " << max << " )";
      return os;
   }

   S GetWidth() const { return (GetMax() - GetMin())[0]; }
   S GetHeight() const { return (GetMax() - GetMin())[1]; }

   int GetTag() const { return tag_; }
   void SetTag(int tag) { tag_ = tag; }

public:
   template <class T> void SaveValue(T* msg) const {
      min.SaveValue(msg->mutable_min());
      max.SaveValue(msg->mutable_max());
      msg->set_tag(tag_);
   }
   template <class T> void LoadValue(const T& msg) {
      tag_ = msg.tag();
      min.LoadValue(msg.min());
      max.LoadValue(msg.max());
   }

public:
   Point    min;
   Point    max;

private:
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
typedef Cube<float, 2> Rect2f;
typedef Cube<float, 3> Cube3f;

class Ray3;

bool Cube3Intersects(const Cube3& rgn, const Ray3& ray, float& distance);
bool Cube3Intersects(const Cube3f& rgn, const Ray3& ray, float& distance);

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Cube3, Protocol::cube3i)
IMPLEMENT_DM_EXTENSION(::radiant::csg::Cube3f, Protocol::cube3f)

#endif // _RADIANT_CSG_BOX_H