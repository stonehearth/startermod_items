//===============================================================================
// @ math.h
// 
// Base math macros and functions
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COMMON_H
#define _RADIANT_MATH3D_COMMON_H

#if !defined(SWIG)
#include <math.h>
#include <assert.h>
#endif

#define k_epsilon    1.0e-3f

#define k_pi         3.1415926535897932384626433832795f
#define k_half_pi    1.5707963267948966192313216916398f
#define k_two_pi     2.0f*k_pi

#define ERROR_OUT(x)    ASSERT(false && (x))

#ifdef PLATFORM_OSX
#define sqrtf sqrt
#define sinf sin
#define cosf cos
#define fabsf fabs
#define tanf tan
#define atan2f atan2
#define acosf acos
#endif

#undef IV_APPROXIMATION   // commented out for the moment
// 
// These routines are from Lomont, "Floating Point Tricks," _Game Gems 6_
// See also www.lomont.org
//
typedef union {float f; int i;} int_or_float;
namespace radiant {
   namespace math3d {      

      inline int ceil(float val) {
         return (int)::ceil(val);
      }

      inline int floor(float val) {
         return (int)::floor(val);
      }

      inline float sqrt(float val)       
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
            return sqrtf(val); 
      #endif
      }

      inline float inv_sqrt(float val) 
      { 
      #if defined(IV_APPROXIMATION)
          float valhalf = 0.5f*val;
          int_or_float workval;
          workval.f = val;
          workval.i = 0x5f375a86 - (workval.i>>1); // initial guess y0 with magic number
          workval.f = workval.f*(1.5f-valhalf*workval.f*workval.f);  // Newton step, increases accuracy
          return workval.f;
      #else
          return 1.0f/sqrtf(val);
      #endif
      } // InvSqrt

      inline float IvAbs(float f)           { return fabsf(f); }

      //-------------------------------------------------------------------------------
      //-- Classes --------------------------------------------------------------------
      //-------------------------------------------------------------------------------

      //-------------------------------------------------------------------------------
      //-- Functions --------------------------------------------------------------------
      //-------------------------------------------------------------------------------
      extern void fast_sin_cos(float a, float& sina, float& cosa);

      //-------------------------------------------------------------------------------
      //-- Inlines --------------------------------------------------------------------
      //-------------------------------------------------------------------------------

      inline float abs(float a) {
          return ::fabs(a);
      }

      //-------------------------------------------------------------------------------
      // @ is_zero()
      //-------------------------------------------------------------------------------
      // Is this floating point value close to zero?
      //-------------------------------------------------------------------------------
      inline bool is_zero(float a) 
      {
          return (::fabsf(a) < k_epsilon);

      }   // End of is_zero()

      //-------------------------------------------------------------------------------
      // @ are_equal()
      //-------------------------------------------------------------------------------
      // Are these floating point values close to equal?
      //-------------------------------------------------------------------------------
      inline bool are_equal(float a, float b) 
      {
          return (is_zero(a-b));

      }   // End of are_equal()


      //-------------------------------------------------------------------------------
      // @ sin()
      //-------------------------------------------------------------------------------
      // Returns the floating-point sine of the argument
      //-------------------------------------------------------------------------------
      inline float sin(float a)
      {
          return ::sinf(a);

      }  // End of sin

      //-------------------------------------------------------------------------------
      inline float atan2(float a, float b)
      {
          return ::atan2(a, b);

      }  // End of sin


      //-------------------------------------------------------------------------------
      // @ cos()
      //-------------------------------------------------------------------------------
      // Returns the floating-point cosine of the argument
      //-------------------------------------------------------------------------------
      inline float cos(float a)
      {
          return ::cosf(a);

      }  // End of cos


      //-------------------------------------------------------------------------------
      // @ tan()
      //-------------------------------------------------------------------------------
      // Returns the floating-point tangent of the argument
      //-------------------------------------------------------------------------------
      inline float tan(float a)
      {
          return ::tanf(a);

      }  // End of tan


      //-------------------------------------------------------------------------------
      // @ sincos()
      //-------------------------------------------------------------------------------
      // Returns the floating-point sine and cosine of the argument
      //-------------------------------------------------------------------------------
      inline void sincos(float a, float& sina, float& cosa)
      {
          sina = ::sinf(a);
          cosa = ::cosf(a);

      }  // End of sincos
   };
};

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
