#ifndef _RADIANT_CSG_CSG_H
#define _RADIANT_CSG_CSG_H

#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

// xxx: hide these!
extern float k_pi;
extern float k_epsilon;

bool IsZero(float value);
bool AreEqual(float v1, float v2);
float Sqrt(float value);
void SinCos(float value, float& sin, float& cos);
float Sin(float value);
float Cos(float value);
float InvSqrt(float value);

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_CSG_H