#include "pch.h"
#include "csg.h"
#include "point.h"
#include "quaternion.h"

using namespace ::radiant;

float csg::k_epsilon = 1.0e-6f;
float csg::k_pi = 3.1415926535897932384626433832795f;

static const float kEpsilon = 1.0e-6f;

bool csg::IsZero(float value)
{
   return ::fabs(value) < kEpsilon;
}

bool csg::IsZero(double value)
{
   return ::fabs(value) < kEpsilon;
}

bool csg::AreEqual(float v1, float v2)
{
   return IsZero(v1 - v2);
}

float csg::Sqrt(float value)
{
#if defined(IV_APPROXIMATION)
   assert(val >= 0);
   int_or_float workval;
   workval.f = val;
   workval.i -= 0x3f800000; // subtract 127 from biased exponent
   workval.i >>= 1;         // requires signed shift to preserve sign
   workval.i += 0x3f800000; // rebias the new exponent
   workval.i &= 0x7FFFFFFF; // remove sign bit
   return workval.f;
#else
   return sqrtf(value); 
#endif
}

void csg::SinCos(float value, float& sina, float& cosa)
{
   sina = ::sinf(value);
   cosa = ::cosf(value);
}

void csg::SinCos(double value, double& sina, double& cosa)
{
   sina = ::sin(value);
   cosa = ::cos(value);
}

float csg::Sin(float value)
{
   return ::sin(value);
}

float csg::Cos(float value)
{
   return ::cos(value);
}

float csg::InvSqrt(float value)
{
#if defined(IV_APPROXIMATION)
   float valhalf = 0.5f*val;
   int_or_float workval;
   workval.f = val;
   workval.i = 0x5f375a86 - (workval.i>>1); // initial guess y0 with magic number
   workval.f = workval.f*(1.5f-valhalf*workval.f*workval.f);  // Newton step, increases accuracy
   return workval.f;
#else
   return 1.0f/sqrtf(value);
#endif
}

// returns the axis and angle in a normalized orientation
// normalized means:
//    1) the axis points up (in the positive y direction)
//    2) the angle is between 0 and 2*pi radians
void csg::GetAxisAngleNormalized(csg::Quaternion const& q, csg::Point3f& axis, double& angle)
{
   q.get_axis_angle(axis, angle);

   if (axis.y < 0) {
      axis *= -1.0f;
      angle *= -1.0f;
   }

   if (angle < 0) {
      angle += 2*k_pi;
   }
}
