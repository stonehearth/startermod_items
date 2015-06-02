#ifndef _RADIANT_CSG_CSG_H
#define _RADIANT_CSG_CSG_H

#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

// xxx: hide these!
extern float k_pi;
extern float k_epsilon;

bool IsZero(float value);
bool IsZero(double value);
bool AreEqual(float v1, float v2);
float Sqrt(float value);
void SinCos(float value, float& sin, float& cos);
void SinCos(double value, double& sin, double& cos);
float Sin(float value);
float Cos(float value);
float InvSqrt(float value);

// Bias: towards zero
template <typename T>
T trunc( const T& value )
{
   T result = std::floor(std::fabs( value ));
   return static_cast<T>((value < 0.0) ? -result : result);
}

template <typename T>
T ToDegrees(T radians)
{
   return radians / k_pi * 180;
}

template <typename T>
T ToRadians(T degrees)
{
   return degrees / 180 * k_pi;
}

void GetAxisAngleNormalized(Quaternion const& q, Point3f& axis, double& angle);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_CSG_H