#ifndef _RADIANT_CSG_ROTATE_SHAPE_H
#define _RADIANT_CSG_ROTATE_SHAPE_H

#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE
   
template <typename S, int C, int D>
struct RotatePoint
{
   Point<S, C> operator()(Point<S, C> const& p);
};

template <typename S, int C>
struct RotatePoint<S, C, 0>
{
   Point<S, C> operator()(Point<S, C> const& p) {
      return p;
   }
};

template <typename S>
struct RotatePoint<S, 3, 270>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(-p.z, p.y, p.x);
   }
};

template <typename S>
struct RotatePoint<S, 3, 180>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(-p.x, p.y, -p.z);
   }
};

template <typename S>
struct RotatePoint<S, 3, 90>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(p.z, p.y, -p.x);
   }
};

template <typename S, int C, int D>
struct RotateCube
{
   Cube<S, C> operator()(Cube<S, C> const& c) {
      Point<S, C> min = RotatePoint<S, C, D>()(c.min);
      Point<S, C> max = RotatePoint<S, C, D>()(c.max);
      return Cube<S, C>::Construct(min, max, c.GetTag());
   }
};

template <typename S, int C, int D>
struct RotateRegion
{
   Region<S, C> operator()(Region<S, C> const& region) {
      Region<S, C> result;
      for (Cube<S, C> c : region) {
         LOG_(0) << "            rotating by " << D << " ->  in:" << c << " out:" << RotateCube<S, C, D>()(c);
         result.AddUnique(RotateCube<S, C, D>()(c));
      }
      return std::move(result);
   }
};

template <typename S, int C>
Region<S, C> Rotated(Region<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotateRegion<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotateRegion<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotateRegion<S, C, 270>()(other);
   }
   ASSERT(false);
   return Region<S, C>();
}

template <typename S, int C>
Cube<S, C> Rotated(Cube<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotateCube<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotateCube<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotateCube<S, C, 270>()(other);
   }
   ASSERT(false);
   return other;
}

template <typename S, int C>
Point<S, C> Rotated(Point<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotatePoint<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotatePoint<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotatePoint<S, C, 270>()(other);
   }
   ASSERT(false);
   return other;
}

END_RADIANT_CSG_NAMESPACE

#endif //  _RADIANT_CSG_NAMESPACE_H
