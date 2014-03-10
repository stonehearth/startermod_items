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

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_CSG_H