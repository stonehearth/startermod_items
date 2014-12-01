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
   enum { Dimension = C };
   typedef Point<S, C> Point;
   typedef Region<S, C> Region;

public:
   static Cube<S, C> zero;
   static Cube<S, C> one;

public:
   Cube();
   Cube(Point const& min_value, int tag = 0);
   Cube(Point const& min_value, Point const& max_value, int tag = 0);

   static Cube Construct(Point min_value, Point max_value, int tag = 0) {
      for (int i = 0; i < C; i++) {
         if (min_value[i] > max_value[i]) {
            std::swap(min_value[i], max_value[i]);
         }
      }
      return Cube(min_value, max_value, tag);
   }

   S GetArea() const;
   bool IsEmpty() const { return GetArea() == 0; }
   Point const& GetMin() const { return min; }
   Point const& GetMax() const { return max; }
   void SetMin(Point const& min_value) { min = min_value; }
   void SetMax(Point const& max_value) { max = max_value; }
   void Grow(Point const& pt);
   void Grow(Cube const& cube); 
   bool CombineWith(Cube const& cube);

   void SetZero() {
      min.SetZero();
      max.SetZero();
   }

   void Translate(Point const& pt) {
      min += pt;
      max += pt;
   }

   Cube Translated(Point const& pt) const {
      Cube result(*this);
      result.Translate(pt);
      return result;
   }

   Cube Inflated(Point const& amount) const;
   Cube Scaled(double factor) const { return Cube(min.Scaled(factor), max.Scaled(factor)); }
   Cube ProjectOnto(int axis, S plane) const;
   Cube Intersected(Cube const& other) const;
   Region GetBorder() const;

   bool Intersects(Cube const& other) const;
   bool Contains(Point const& pt) const;
   Point GetClosestPoint(Point const& other) const;
   Point GetSize() const { return GetMax() - GetMin(); }
   inline double SquaredDistanceTo(Point const& other) const;
   inline double SquaredDistanceTo(Cube const& other) const;
   double DistanceTo(Point const& other) const;
   double DistanceTo(Cube const& other) const;

   // optimizing...
   bool operator==(Cube const& other) const;
   bool operator!=(Cube const& other) const;
   Cube operator&(Cube const& other) const;
   Region operator&(Region const& other) const;
   Cube operator+(Point const& other) const;
   Cube operator-() const;
   Region operator-(Cube const& other) const;
   Region operator-(Region const& other) const;

   std::ostream& Format(std::ostream& os) const {
      os << "(" << min << " - " << max << ")";
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
Point<double, C> GetCentroid(Cube<S, C> const& cube);

template <typename S, int C>
std::ostream& operator<<(std::ostream& os, Cube<S, C> const& in)
{
   return in.Format(os);
}

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_BOX_H