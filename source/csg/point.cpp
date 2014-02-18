#include "pch.h"
#include "point.h"
#include "util.h" // xxx: should be csg/namespace.h
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

const int PointBase<int, 3, Point3>::Dimension = 3;
const int PointBase<int, 2, Point2>::Dimension = 2;
const int PointBase<int, 1, Point1>::Dimension = 1;
const int PointBase<float, 3, Point3f>::Dimension = 3;
const int PointBase<float, 2, Point2f>::Dimension = 2;
const int PointBase<float, 1, Point1f>::Dimension = 1;

const Point3f Point3f::zero(0, 0, 0);
const Point3f Point3f::one(1, 1, 1);
const Point3f Point3f::unitX(1, 0, 0);
const Point3f Point3f::unitY(0, 1, 0);
const Point3f Point3f::unitZ(0, 0, 1);

const Point2f Point2f::zero(0, 0);
const Point2f Point2f::one(1, 1);

const Point3 Point3::zero(0, 0, 0);
const Point3 Point3::one(1, 1, 1);

const Point2 Point2::zero(0, 0);
const Point2 Point2::one(1, 1);

const Point1 Point1::zero(0);
const Point1 Point1::one(1);


template <int C>
Point<float, C> csg::ToFloat(Point<int, C> const& pt) {
   Point<float, C> result;
   for (int i = 0; i < C; i++) {
      result[i] = static_cast<float>(pt[i]);
   }
   return result;
}

template <int C>
Point<float, C> const& csg::ToFloat(Point<float, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> csg::ToInt(Point<float, C> const& pt)
{
   Point<int, C> result;
   for (int i = 0; i < C; i++) {
      float s = pt[i];
      result[i] = static_cast<int>(floor0(s + (s > 0 ? k_epsilon : -k_epsilon)));
   }
   return result;
}

template <int C>
Point<int, C> const& csg::ToInt(Point<int, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> csg::ToClosestInt(Point<float, C> const& pt)
{
   Point<int, C> result;
   for (int i = 0; i < C; i++) {
      float s = pt[i];
      result[i] = static_cast<int>(floor0(s + (s > 0 ? 0.5 : -0.5)));
   }
   return result;
}

template <int C>
Point<int, C> const& csg::ToClosestInt(Point<int, C> const& pt) {
   return pt;
}

template<typename S> S csg::Interpolate(S const& p0, S const& p1, float alpha)
{
   if (alpha <= 0.0f) {
      return p0;
   }
   if (alpha >= 1.0f) {
      return p1;
   }
   S delta = p1 - p0;
   return p0 + (delta * alpha);
}

template float csg::Interpolate(float const&, float const&, float);

#define DEFINE_POINT_CONVERSIONS(C) \
   template Point<float, C> csg::ToFloat(Point<int, C> const&); \
   template Point<float, C> const& csg::ToFloat(Point<float, C> const&); \
   template Point<int, C> const& csg::ToInt(Point<int, C> const&); \
   template Point<int, C> csg::ToInt(Point<float, C> const&); \
   template Point<int, C> const& csg::ToClosestInt(Point<int, C> const&); \
   template Point<int, C> csg::ToClosestInt(Point<float, C> const&); \
   template Point<float, C> csg::Interpolate(Point<float, C> const&, Point<float, C> const&, float); \
   template float csg::Interpolate(float const&, float const&, float);

DEFINE_POINT_CONVERSIONS(2)
DEFINE_POINT_CONVERSIONS(3)

