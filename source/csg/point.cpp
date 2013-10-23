#include "pch.h"
#include "point.h"
#include "csg/util.h" // xxx: should be csg/namespace.h

using namespace ::radiant;
using namespace ::radiant::csg;

const int PointBase<int, 3, Point3>::Dimension = 3;
const int PointBase<int, 2, Point2>::Dimension = 2;
const int PointBase<int, 1, Point1>::Dimension = 1;
const int PointBase<float, 3, Point3f>::Dimension = 3;
const int PointBase<float, 2, Point2f>::Dimension = 2;
const int PointBase<float, 1, Point1f>::Dimension = 1;

Point3f Point3f::zero(0, 0, 0);
Point3f Point3f::one(1, 1, 1);
Point3f Point3f::unitX(1, 0, 0);
Point3f Point3f::unitY(0, 1, 0);
Point3f Point3f::unitZ(0, 0, 1);

Point2f Point2f::zero(0, 0);
Point2f Point2f::one(1, 1);

Point3 Point3::zero(0, 0, 0);
Point3 Point3::one(1, 1, 1);

Point2 Point2::zero(0, 0);
Point2 Point2::one(1, 1);

Point1 Point1::zero(0);
Point1 Point1::one(1);

Point3 csg::ToInt(Point3f const& pt)
{
   Point3 result;
   for (int i = 0; i < 3; i++) {
      float s = pt[i];
      result[i] = static_cast<int>(s + (s > 0 ? k_epsilon : -k_epsilon));
   }
   return result;
}

Point3f csg::Interpolate(Point3f const& p0, Point3f const& p1, float alpha)
{
   if (alpha <= 0.0f) {
      return p0;
   }
   if (alpha >= 1.0f) {
      return p1;
   }
   Point3f delta = p1 - p0;
   return p0 + (delta * alpha);
}

