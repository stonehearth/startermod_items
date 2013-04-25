#include "radiant.h"
#include "common.h"

using namespace radiant;
using namespace radiant::math3d;

//-------------------------------------------------------------------------------
// @ polynomial_sin_quadrant()
//-------------------------------------------------------------------------------
// One quadrant of a polynomial approximation of sin
//-------------------------------------------------------------------------------
static inline float polynomial_sin_quadrant(float a)
{

    // sin(a) ~ a - 0.16666 a^3 + 0.0083143 a^5 - 0.00018542 a^7 

    return a * (1.0f + a * a * (-0.16666f 
                                 + a * a * (0.0083143f 
                                            - a * a * 0.00018542f))); 
}

//-------------------------------------------------------------------------------
// @ math3d::fast_sin_cos()
//-------------------------------------------------------------------------------
// Returns the floating-point sine and cosine of the argument.  This method uses
// a polynomial approximation
//-------------------------------------------------------------------------------
void math3d::fast_sin_cos(float a, float& sina, float& cosa)
{
    // The concepts in this function are based on Robin Green's GDC 2003 session
    // "Faster Math Functions".  The slides are available online from Sony R&D at
    // http://www.research.scea.com/gdc2003/fast-math-functions.html

    // work with a non-negative angle - sin is odd and cos is even, so this is
    // easy to do
    bool bNegate = false;
    if (a < 0.0f)
    {
        a = -a;
        bNegate = true;
    }

    // The polynomial approximation requires that the angle "a" be decomposed as
    // a = Pi/2 * intA + floatA,
    // where intA is an integer and floatA is a float in the range [0, Pi/2]
    // Because sin and cos have a period of 2Pi, we will use both intA and floatA
    // to compute sina and cosa.
    const float kTwoOverPI = 1.0f / k_half_pi;
    float floatA = kTwoOverPI * a;
    int intA = (int)floatA;

    // This next step (getting the floating-point remainder of floatA would 
    // normally involve subtracting two nearly equal values, which could lead
    // to catastrophic cancelation.  
    // Instead, we break Pi/2 into two parts that add to Pi/2.  
    // One is a rational that single-precision can represent exactly, 
    // while the other is the remainder.  We do the subtraction in two stages 
    // to avoid cancelling.
    const float kRationalHalfPI = 201 / 128.0f;
    const float kRemainderHalfPI = 4.8382679e-4f;

    floatA = (a - kRationalHalfPI * intA) - kRemainderHalfPI * intA;

    // same trick to slide the angle over by Pi/2
    float floatAMinusHalfPi = (floatA - kRationalHalfPI) - kRemainderHalfPI;

    // need to switch, based on the quadrant of Pi, so we compute a fast "mod 4"
    // of intA
    switch (intA & 3)
    {
    case 0: // 0 - Pi/2
        sina = polynomial_sin_quadrant(floatA);
        cosa = polynomial_sin_quadrant(-floatAMinusHalfPi);
        break;
    case 1: // Pi/2 - Pi
        sina = polynomial_sin_quadrant(-floatAMinusHalfPi);
        cosa = polynomial_sin_quadrant(-floatA);
        break;
    case 2: // Pi - 3Pi/2
        sina = polynomial_sin_quadrant(-floatA);
        cosa = polynomial_sin_quadrant(floatAMinusHalfPi);
        break;
    case 3: // 3Pi/2 - 2Pi
        sina = polynomial_sin_quadrant(floatAMinusHalfPi);
        cosa = polynomial_sin_quadrant(floatA);
        break;
    };

    if (bNegate)
        sina = -sina;

}  // End of math3d::fast_sin_cos


