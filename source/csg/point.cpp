#include "pch.h"
#include "point.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Point3f Point3f::zero(0, 0, 0);
Point3f Point3f::one(1, 1, 1);
Point3f Point3f::unitX(1, 0, 0);
Point3f Point3f::unitY(0, 1, 0);
Point3f Point3f::unitZ(0, 0, 1);

Point3 Point3::zero(0, 0, 0);
Point3 Point3::one(1, 1, 1);

Point2 Point2::zero(0, 0);
Point2 Point2::one(1, 1);

Point1 Point1::zero(0);
Point1 Point1::one(1);

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

