#include "pch.h"
#include "csg.h"

using namespace ::radiant;

float csg::k_epsilon = 1.0e-3f;
float csg::k_pi = 3.1415926535897932384626433832795f;

static const float kEpsilon = 1.0e-3f;

bool csg::IsZero(float value)
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
