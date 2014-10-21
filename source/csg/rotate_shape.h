#ifndef _RADIANT_CSG_ROTATE_SHAPE_H
#define _RADIANT_CSG_ROTATE_SHAPE_H

#include "region.h"
#include "iterators.h"

BEGIN_RADIANT_CSG_NAMESPACE

// Copying operators.

template <typename S, int C, int D>
struct RotatedPoint
{
   Point<S, C> operator()(Point<S, C> const& p);
};

template <typename S, int C>
struct RotatedPoint<S, C, 0>
{
   Point<S, C> operator()(Point<S, C> const& p) {
      return p;
   }
};

template <typename S>
struct RotatedPoint<S, 3, 270>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(-p.z, p.y, p.x);
   }
};

template <typename S>
struct RotatedPoint<S, 3, 180>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(-p.x, p.y, -p.z);
   }
};

template <typename S>
struct RotatedPoint<S, 3, 90>
{
   Point<S, 3> operator()(Point<S, 3> const& p) {
      return Point<S, 3>(p.z, p.y, -p.x);
   }
};

template <typename S>
struct RotatedPoint<S, 2, 270>
{
   Point<S, 2> operator()(Point<S, 2> const& p) {
      return Point<S, 2>(-p.y, p.x);
   }
};

template <typename S>
struct RotatedPoint<S, 2, 180>
{
   Point<S, 2> operator()(Point<S, 2> const& p) {
      return Point<S, 2>(-p.x, -p.y);
   }
};

template <typename S>
struct RotatedPoint<S, 2, 90>
{
   Point<S, 2> operator()(Point<S, 2> const& p) {
      return Point<S, 2>(p.y, -p.x);
   }
};

template <typename S, int C, int D>
struct RotatedCube
{
   Cube<S, C> operator()(Cube<S, C> const& c) {
      Point<S, C> min = RotatedPoint<S, C, D>()(c.min);
      Point<S, C> max = RotatedPoint<S, C, D>()(c.max);
      return Cube<S, C>::Construct(min, max, c.GetTag());
   }
};

template <typename S, int C, int D>
struct RotatedRegion
{
   Region<S, C> operator()(Region<S, C> const& region) {
      Region<S, C> result;
      Region<S, C>::CubeVector const& cubes = region.GetContents();

      // Avoid ::EachCube here to make sure we preserve the type of the input
      for (Cube<S, C> const& c : cubes) {
         result.AddUnique(RotatedCube<S, C, D>()(c));
      }
      return std::move(result);
   }
};

template <typename S, int C>
Region<S, C> Rotated(Region<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotatedRegion<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotatedRegion<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotatedRegion<S, C, 270>()(other);
   }
   ASSERT(false);
   return Region<S, C>();
}

template <typename S, int C>
Cube<S, C> Rotated(Cube<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotatedCube<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotatedCube<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotatedCube<S, C, 270>()(other);
   }
   ASSERT(false);
   return other;
}

template <typename S, int C>
Point<S, C> Rotated(Point<S, C> const &other, int degrees) {
   if (degrees == 0) {
      return other;
   } else if (degrees == 90) {
      return RotatedPoint<S, C, 90>()(other);
   } else if (degrees == 180) {
      return RotatedPoint<S, C, 180>()(other);
   } else if (degrees == 270) {
      return RotatedPoint<S, C, 270>()(other);
   }
   ASSERT(false);
   return other;
}

// In-place operators.

template <typename S, int C, int D>
struct RotatePoint
{
   void operator()(Point<S, C>& p);
};

template <typename S, int C>
struct RotatePoint<S, C, 0>
{
   void operator()(Point<S, C>& p) { }
};

template <typename S>
struct RotatePoint<S, 3, 270>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 3>(-p.z, p.y, p.x);
   }
};

template <typename S>
struct RotatePoint<S, 3, 180>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 3>(-p.x, p.y, -p.z);
   }
};

template <typename S>
struct RotatePoint<S, 3, 90>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 3>(p.z, p.y, -p.x);
   }
};

template <typename S>
struct RotatePoint<S, 2, 270>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 2>(-p.y, p.x);
   }
};

template <typename S>
struct RotatePoint<S, 2, 180>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 2>(-p.x, -p.y);
   }
};

template <typename S>
struct RotatePoint<S, 2, 90>
{
   void operator()(Point<S, 3>& p) {
      p = Point<S, 2>(p.y, -p.x);
   }
};

template <typename S, int C, int D>
struct RotateCube
{
   void operator()(Cube<S, C>& cube) {
      Point<S, C> min = RotatedPoint<S, C, D>()(cube.min);
      Point<S, C> max = RotatedPoint<S, C, D>()(cube.max);
      cube = Cube<S, C>::Construct(min, max, cube.GetTag());
   }
};

template <typename S, int C, int D>
struct RotateRegion
{
   void operator()(Region<S, C>& region) {
      // Avoid ::EachCube here to make sure we preserve the type of the input
      Region<S, C>::CubeVector& cubes = region.GetContents();
      for (Cube<S, C>& c : cubes) {
         RotateCube<S, C, D>()(c);
      }
   }
};

template <typename S, int C>
void Rotate(Region<S, C> &other, int degrees) {
   if (degrees == 0) {
   } else if (degrees == 90) {
      RotateRegion<S, C, 90>()(other);
   } else if (degrees == 180) {
      RotateRegion<S, C, 180>()(other);
   } else if (degrees == 270) {
      RotateRegion<S, C, 270>()(other);
   } else {
      ASSERT(false);
   }
}

template <typename S, int C>
void Rotate(Cube<S, C> &other, int degrees) {
   if (degrees == 0) {
   } else if (degrees == 90) {
      RotateCube<S, C, 90>()(other);
   } else if (degrees == 180) {
      RotateCube<S, C, 180>()(other);
   } else if (degrees == 270) {
      RotateCube<S, C, 270>()(other);
   } else {
      ASSERT(false);
   }
}

template <typename S, int C>
void Rotate(Point<S, C> &other, int degrees) {
   if (degrees == 0) {
   } else if (degrees == 90) {
      RotatePoint<S, C, 90>()(other);
   } else if (degrees == 180) {
      RotatePoint<S, C, 180>()(other);
   } else if (degrees == 270) {
      RotatePoint<S, C, 270>()(other);
   } else {
      ASSERT(false);
   }
}

END_RADIANT_CSG_NAMESPACE

#endif //  _RADIANT_CSG_NAMESPACE_H
