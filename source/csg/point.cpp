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
Point<float, C> ToFloatFn(Point<int, C> const& pt);

template <>
Point<float, 1> ToFloatFn(Point<int, 1> const& pt) {
   return Point<float, 1>(static_cast<float>(pt.x));
}

template <>
Point<float, 2> ToFloatFn(Point<int, 2> const& pt) {
   return Point<float, 2>(static_cast<float>(pt.x),
                          static_cast<float>(pt.y));
}

template <>
Point<float, 3> ToFloatFn(Point<int, 3> const& pt) {
   return Point<float, 3>(static_cast<float>(pt.x),
                          static_cast<float>(pt.y),
                          static_cast<float>(pt.z));
}

template <int C>
Point<float, C> csg::ToFloat(Point<int, C> const& pt) {
   return ToFloatFn(pt);
}

template <int C>
Point<float, C> const& csg::ToFloat(Point<float, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> ToIntFn(Point<float, C> const& pt);

template <>
Point<int, 1> ToIntFn(Point<float, 1> const& pt)
{
   return Point<int, 1>(ToInt(pt.x));
}

template <>
Point<int, 2> ToIntFn(Point<float, 2> const& pt)
{
   return Point<int, 2>(ToInt(pt.x),
                        ToInt(pt.y));
}
template <>
Point<int, 3> ToIntFn(Point<float, 3> const& pt)
{
   return Point<int, 3>(ToInt(pt.x),
                        ToInt(pt.y),
                        ToInt(pt.z));
}

template <int C>
Point<int, C> csg::ToInt(Point<float, C> const& pt) {
   return ToIntFn(pt);
}

template <int C>
Point<int, C> const& csg::ToInt(Point<int, C> const& pt) {
   return pt;
}

template <int C>
Point<int, C> ToClosestIntFn(Point<float, C> const& pt);

template <>
Point<int, 1> ToClosestIntFn(Point<float, 1> const& pt)
{
   return Point<int, 1>(ToClosestInt(pt.x));
}

template <>
Point<int, 2> ToClosestIntFn(Point<float, 2> const& pt)
{
   return Point<int, 2>(ToClosestInt(pt.x),
                        ToClosestInt(pt.y));
}
template <>
Point<int, 3> ToClosestIntFn(Point<float, 3> const& pt)
{
   return Point<int, 3>(ToClosestInt(pt.x),
                        ToClosestInt(pt.y),
                        ToClosestInt(pt.z));
}

template <int C>
Point<int, C> csg::ToClosestInt(Point<float, C> const& pt) {
   return ToClosestIntFn(pt);
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

template <> Point3 csg::ConvertTo(Point3 const& pt) { return pt; }
template <> Point3f csg::ConvertTo(Point3f const& pt) { return pt; }
template <> Point3f csg::ConvertTo(Point3 const& pt) { return ToFloat(pt); }
template <> Point3 csg::ConvertTo(Point3f const& pt) { return ToInt(pt); }

DEFINE_POINT_CONVERSIONS(1)
DEFINE_POINT_CONVERSIONS(2)
DEFINE_POINT_CONVERSIONS(3)

