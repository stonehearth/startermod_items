#ifndef _RADIANT_CSG_BOX_H
#define _RADIANT_CSG_BOX_H

#include "namespace.h"
#include "point.h"

BEGIN_RADIANT_CSG_NAMESPACE

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
         axis_(0)
      {
         if (c.GetArea() <= 0) {
            iter_ = end;
            return;
         }

         ASSERT(iter == end || c.Contains(iter));
         if (!c.Contains(iter)) {
            iter_ = end;
            return;
         }
         iter_ = iter;
      }

      const Point operator*() const {
         return iter_;
      }

      const void operator++() {
         if (iter_ != end) {
            iter_.z++;
            if (iter_.z == max.z) {
               iter_.z = min.z;
               iter_.x++;
               if (iter_.x == max.x) {
                  iter_.z = min.z;
                  iter_.x = min.x;
                  iter_.y++;
                  if (iter_.y == max.y) {
                     iter_ = end;
                  }
               }
            }
         }
      }
      bool operator!=(const PointIterator& rhs) {
         return iter_ != rhs.iter_;
      }

   public:
      static   Point3 end;

   private:
      Point    min;
      Point    max;
      Point    iter_;
      int      axis_;
   };

   PointIterator begin() const { return PointIterator(*this, GetMin()); }
   PointIterator end() const { return PointIterator(*this, PointIterator::end); }

   S GetArea() const;
   bool IsEmpty() const { return GetArea() == 0; }
   const Point& GetMin() const { return min; }
   const Point& GetMax() const { return max; }
   void SetMin(const Point& min_value) { min = min_value; }
   void SetMax(const Point& max_value) { max = max_value; }
   void Grow(const Point& pt);
   void Grow(const Cube& cube); 
   bool CombineWith(const Cube& cube);

   void SetZero() {
      min.SetZero();
      max.SetZero();
   }

   void Translate(const Point& pt) {
      min += pt;
      max += pt;
   }

   Cube Translated(const Point& pt) const {
      Cube result(*this);
      result.Translate(pt);
      return result;
   }

   Cube Inflated(Point amount) const {
      return Cube(min - amount, max + amount, GetTag());
   }

   Cube Scaled(float factor) const { return Cube(min.Scaled(factor), max.Scaled(factor)); }
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
   Cube operator-() const;
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

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_BOX_H