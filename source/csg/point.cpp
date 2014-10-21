#include "pch.h"
#include "point.h"
#include "util.h" // xxx: should be csg/namespace.h
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

const Point3f Point3f::zero(0, 0, 0);
const Point3f Point3f::one(1, 1, 1);
const Point3f Point3f::unitX(1, 0, 0);
const Point3f Point3f::unitY(0, 1, 0);
const Point3f Point3f::unitZ(0, 0, 1);

const Point2f Point2f::zero(0, 0);
const Point2f Point2f::one(1, 1);

const Point3 Point3::zero(0, 0, 0);
const Point3 Point3::one(1, 1, 1);
const Point3 Point3::unitX(1, 0, 0);
const Point3 Point3::unitY(0, 1, 0);
const Point3 Point3::unitZ(0, 0, 1);

const Point2 Point2::zero(0, 0);
const Point2 Point2::one(1, 1);

const Point1 Point1::zero(0);
const Point1 Point1::one(1);

const Point1f Point1f::zero(0);
const Point1f Point1f::one(1);


template <int C>
Point<double, C> ToFloatFn(Point<int, C> const& pt);

template <>
Point<double, 1> ToFloatFn(Point<int, 1> const& pt) {
   return Point<double, 1>(static_cast<double>(pt.x));
}

template <>
Point<double, 2> ToFloatFn(Point<int, 2> const& pt) {
   return Point<double, 2>(static_cast<double>(pt.x),
                          static_cast<double>(pt.y));
}

template <>
Point<double, 3> ToFloatFn(Point<int, 3> const& pt) {
   return Point<double, 3>(static_cast<double>(pt.x),
                          static_cast<double>(pt.y),
                          static_cast<double>(pt.z));
}

template <int C>
Point<double, C> csg::ToFloat(Point<int, C> const& pt) {
   return ToFloatFn(pt);
}

template <int C>
Point<double, C> const& csg::ToFloat(Point<double, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> ToIntFn(Point<double, C> const& pt);

template <>
Point<int, 1> ToIntFn(Point<double, 1> const& pt)
{
   return Point<int, 1>(ToInt(pt.x));
}

template <>
Point<int, 2> ToIntFn(Point<double, 2> const& pt)
{
   return Point<int, 2>(ToInt(pt.x),
                        ToInt(pt.y));
}
template <>
Point<int, 3> ToIntFn(Point<double, 3> const& pt)
{
   return Point<int, 3>(ToInt(pt.x),
                        ToInt(pt.y),
                        ToInt(pt.z));
}

template <int C>
Point<int, C> csg::ToInt(Point<double, C> const& pt) {
   return ToIntFn(pt);
}

template <int C>
Point<int, C> const& csg::ToInt(Point<int, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> ToClosestIntFn(Point<double, C> const& pt);

template <>
Point<int, 1> ToClosestIntFn(Point<double, 1> const& pt)
{
   return Point<int, 1>(ToClosestInt(pt.x));
}

template <>
Point<int, 2> ToClosestIntFn(Point<double, 2> const& pt)
{
   return Point<int, 2>(ToClosestInt(pt.x),
                        ToClosestInt(pt.y));
}
template <>
Point<int, 3> ToClosestIntFn(Point<double, 3> const& pt)
{
   return Point<int, 3>(ToClosestInt(pt.x),
                        ToClosestInt(pt.y),
                        ToClosestInt(pt.z));
}

template <int C>
Point<int, C> csg::ToClosestInt(Point<double, C> const& pt) {
   return ToClosestIntFn(pt);
}

template <int C>
Point<int, C> const& csg::ToClosestInt(Point<int, C> const& pt) {
   return pt;
}

template<typename S> S csg::Interpolate(S const& p0, S const& p1, double alpha)
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

template double csg::Interpolate(double const&, double const&, double);

#define DEFINE_POINT_CONVERSIONS(C) \
   template Point<double, C> csg::ToFloat(Point<int, C> const&); \
   template Point<double, C> const& csg::ToFloat(Point<double, C> const&); \
   template Point<int, C> const& csg::ToInt(Point<int, C> const&); \
   template Point<int, C> csg::ToInt(Point<double, C> const&); \
   template Point<int, C> const& csg::ToClosestInt(Point<int, C> const&); \
   template Point<int, C> csg::ToClosestInt(Point<double, C> const&); \
   template Point<double, C> csg::Interpolate(Point<double, C> const&, Point<double, C> const&, double); \
   template double csg::Interpolate(double const&, double const&, double);

template <> Point3 csg::ConvertTo(Point3 const& pt) { return pt; }
template <> Point3f csg::ConvertTo(Point3f const& pt) { return pt; }
template <> Point3f csg::ConvertTo(Point3 const& pt) { return ToFloat(pt); }
template <> Point3 csg::ConvertTo(Point3f const& pt) { return ToInt(pt); }

DEFINE_POINT_CONVERSIONS(1)
DEFINE_POINT_CONVERSIONS(2)
DEFINE_POINT_CONVERSIONS(3)

