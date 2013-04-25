#include "radiant.h"
#include "radiant.pb.h"
#include "common.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ quaternion.cpp
// 
// 3D quat class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "radiant.h"
#include "quaternion.h"
#include "vector3.h"
#include "point3.h"
#include "matrix3.h"

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

quaternion quaternion::zero(0.0f, 0.0f, 0.0f, 0.0f);
quaternion quaternion::identity(1.0f, 0.0f, 0.0f, 0.0f);

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ quaternion::quaternion()
//-------------------------------------------------------------------------------
// Axis-angle constructor
//-------------------------------------------------------------------------------
quaternion::quaternion(const vector3& axis, float angle)
{
    set(axis, angle);
}   // End of quaternion::quaternion()


//-------------------------------------------------------------------------------
// @ quaternion::quaternion()
//-------------------------------------------------------------------------------
// To-from vector constructor
//-------------------------------------------------------------------------------
quaternion::quaternion(const vector3& from, const vector3& to)
{
    set(from, to);
}   // End of quaternion::quaternion()

//-------------------------------------------------------------------------------
// @ quaternion::quaternion()
//-------------------------------------------------------------------------------
// Vector constructor
//-------------------------------------------------------------------------------
quaternion::quaternion(const vector3& vector)
{
    set(0.0f, vector.x, vector.y, vector.z);
}   // End of quaternion::quaternion()


//-------------------------------------------------------------------------------
// @ quaternion::quaternion()
//-------------------------------------------------------------------------------
// rotation matrix constructor
//-------------------------------------------------------------------------------
quaternion::quaternion(const matrix3& rotation)
{
    float trace = rotation.Guard();
    if (trace > 0.0f)
    {
        float s = math3d::sqrt(trace + 1.0f);
        w = s*0.5f;
        float recip = 0.5f/s;
        x = (rotation(2,1) - rotation(1,2))*recip;
        y = (rotation(0,2) - rotation(2,0))*recip;
        z = (rotation(1,0) - rotation(0,1))*recip;
    }
    else
    {
        unsigned int i = 0;
        if (rotation(1,1) > rotation(0,0))
            i = 1;
        if (rotation(2,2) > rotation(i,i))
            i = 2;
        unsigned int j = (i+1)%3;
        unsigned int k = (j+1)%3;

        float s = math3d::sqrt(rotation(i,i) - rotation(j,j) - rotation(k,k) + 1.0f);

        float* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = 0.5f*s;
        float recip = 0.5f/s;
        w = (rotation(k,j) - rotation(j,k))*recip;
        *apkQuat[j] = (rotation(j,i) + rotation(i,j))*recip;
        *apkQuat[k] = (rotation(k,i) + rotation(i,k))*recip;
    }

}   // End of quaternion::quaternion()


//-------------------------------------------------------------------------------
// @ quaternion::quaternion()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
quaternion::quaternion(const quaternion& other) : 
    w(other.w),
    x(other.x),
    y(other.y),
    z(other.z)
{

}   // End of quaternion::quaternion()


//-------------------------------------------------------------------------------
// @ quaternion::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
quaternion&
quaternion::operator=(const quaternion& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    w = other.w;
    x = other.x;
    y = other.y;
    z = other.z;
    
    return *this;

}   // End of quaternion::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const quaternion& source)
{
    out << '[' << source.w << ',' << source.x << ',' 
        << source.y << ',' << source.z << ']';

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ quaternion::magnitude()
//-------------------------------------------------------------------------------
// Quaternion magnitude (square root of magnitude)
//-------------------------------------------------------------------------------
float 
quaternion::magnitude() const
{
    return math3d::sqrt(w*w + x*x + y*y + z*z);

}   // End of quaternion::magnitude()


//-------------------------------------------------------------------------------
// @ quaternion::norm()
//-------------------------------------------------------------------------------
// Quaternion magnitude
//-------------------------------------------------------------------------------
float 
quaternion::norm() const
{
    return (w*w + x*x + y*y + z*z);

}   // End of quaternion::Norm()


//-------------------------------------------------------------------------------
// @ quaternion::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
quaternion::operator==(const quaternion& other) const
{
    if (math3d::is_zero(other.w - w)
        && math3d::is_zero(other.x - x)
        && math3d::is_zero(other.y - y)
        && math3d::is_zero(other.z - z))
        return true;

    return false;   
}   // End of quaternion::operator==()


//-------------------------------------------------------------------------------
// @ quaternion::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
quaternion::operator!=(const quaternion& other) const
{
    if (math3d::is_zero(other.w - w)
        || math3d::is_zero(other.x - x)
        || math3d::is_zero(other.y - y)
        || math3d::is_zero(other.z - z))
        return false;

    return true;
}   // End of quaternion::operator!=()


//-------------------------------------------------------------------------------
// @ IvQuatmath::is_zero()
//-------------------------------------------------------------------------------
// Check for zero quat
//-------------------------------------------------------------------------------
bool 
quaternion::is_zero() const
{
    return math3d::is_zero(w*w + x*x + y*y + z*z);

}   // End of IvQuatmath::is_zero()


//-------------------------------------------------------------------------------
// @ quaternion::is_unit()
//-------------------------------------------------------------------------------
// Check for unit quat
//-------------------------------------------------------------------------------
bool 
quaternion::is_unit() const
{
    return math3d::is_zero(1.0f - magnitude());

}   // End of quaternion::is_unit()


//-------------------------------------------------------------------------------
// @ quaternion::is_identity()
//-------------------------------------------------------------------------------
// Check for identity quat
//-------------------------------------------------------------------------------
bool 
quaternion::is_identity() const
{
    return math3d::is_zero(1.0f - w)
        && math3d::is_zero(x) 
        && math3d::is_zero(y)
        && math3d::is_zero(z);

}   // End of quaternion::is_identity()


//-------------------------------------------------------------------------------
// @ quaternion::set()
//-------------------------------------------------------------------------------
// Set quaternion based on axis-angle
//-------------------------------------------------------------------------------
void
quaternion::set(const vector3& axis, float angle)
{
    // if axis of rotation is zero vector, just set to identity quat
    float length = axis.length_squared();
    if (math3d::is_zero(length))
    {
        set_identity();
        return;
    }

    // take half-angle
    angle *= 0.5f;

    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    float scaleFactor = sintheta/math3d::sqrt(length);

    w = costheta;
    x = scaleFactor * axis.x;
    y = scaleFactor * axis.y;
    z = scaleFactor * axis.z;

}   // End of quaternion::set()


//-------------------------------------------------------------------------------
// @ quaternion::set()
//-------------------------------------------------------------------------------
// Set quaternion based on start and end vectors
// 
// This is a slightly faster method than that presented in the book, and it 
// doesn't require unit vectors as input.  Found on GameDev.net, in an article by
// minorlogic.  Original source unknown.
//-------------------------------------------------------------------------------
void
quaternion::set(const vector3& from, const vector3& to)
{
   // get axis of rotation
    vector3 axis = from.cross(to);

    // get scaled cos of angle between vectors and set initial quaternion
    set( from.dot(to), axis.x, axis.y, axis.z);
    // quaternion at this point is ||from||*||to||*(cos(theta), r*sin(theta))

    // normalize to remove ||from||*||to|| factor
    normalize();
    // quaternion at this point is (cos(theta), r*sin(theta))
    // what we want is (cos(theta/2), r*sin(theta/2))

    // set up for half angle calculation
    w += 1.0f;

    // now when we normalize, we'll be dividing by sqrt(2*(1+cos(theta))), which is 
    // what we want for r*sin(theta) to give us r*sin(theta/2)  (see pages 487-488)
    // 
    // w will become 
    //                 1+cos(theta)
    //            ----------------------
    //            sqrt(2*(1+cos(theta)))        
    // which simplifies to
    //                cos(theta/2)

    // before we normalize, check if vectors are opposing
    if (w <= k_epsilon)
    {
        // rotate pi radians around orthogonal vector
        // take cross product with x axis
        if (from.z*from.z > from.x*from.x)
            set(0.0f, 0.0f, from.z, -from.y);
        // or take cross product with z axis
        else
            set(0.0f, from.y, -from.x, 0.0f);
    }
   
    // normalize again to get rotation quaternion
    normalize();

}   // End of quaternion::set()


//-------------------------------------------------------------------------------
// @ quaternion::set()
//-------------------------------------------------------------------------------
// Set quaternion based on fixed angles
//-------------------------------------------------------------------------------
void 
quaternion::set(float z_rotation, float y_rotation, float x_rotation) 
{
    z_rotation *= 0.5f;
    y_rotation *= 0.5f;
    x_rotation *= 0.5f;

    // get sines and cosines of half angles
    float Cx, Sx;
    math3d::sincos(x_rotation, Sx, Cx);

    float Cy, Sy;
    math3d::sincos(y_rotation, Sy, Cy);

    float Cz, Sz;
    math3d::sincos(z_rotation, Sz, Cz);

    // multiply it out
    w = Cx*Cy*Cz - Sx*Sy*Sz;
    x = Sx*Cy*Cz + Cx*Sy*Sz;
    y = Cx*Sy*Cz - Sx*Cy*Sz;
    z = Cx*Cy*Sz + Sx*Sy*Cx;

}   // End of quaternion::set()


//-------------------------------------------------------------------------------
// @ quaternion::get_axis_angle()
//-------------------------------------------------------------------------------
// Get axis-angle based on quaternion
//-------------------------------------------------------------------------------
void
quaternion::get_axis_angle(vector3& axis, float& angle)
{
    angle = 2.0f*acosf(w);
    float length = math3d::sqrt(1.0f - w*w);
    if (math3d::is_zero(length))
        axis.set_zero();
    else
    {
        length = 1.0f/length;
        axis.set(x*length, y*length, z*length);
    }

}   // End of quaternion::get_axis_angle()


//-------------------------------------------------------------------------------
// @ quaternion::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
quaternion::clean()
{
    if (math3d::is_zero(w))
        w = 0.0f;
    if (math3d::is_zero(x))
        x = 0.0f;
    if (math3d::is_zero(y))
        y = 0.0f;
    if (math3d::is_zero(z))
        z = 0.0f;

}   // End of quaternion::clean()


//-------------------------------------------------------------------------------
// @ quaternion::normalize()
//-------------------------------------------------------------------------------
// Set to unit quaternion
//-------------------------------------------------------------------------------
void
quaternion::normalize()
{
    float lengthsq = w*w + x*x + y*y + z*z;

    if (math3d::is_zero(lengthsq))
    {
        set_zero();
    }
    else
    {
        float factor = math3d::inv_sqrt(lengthsq);
        w *= factor;
        x *= factor;
        y *= factor;
        z *= factor;
    }

}   // End of quaternion::normalize()


//-------------------------------------------------------------------------------
// @ ::conjugate()
//-------------------------------------------------------------------------------
// Compute complex conjugate
//-------------------------------------------------------------------------------
quaternion 
math3d::conjugate(const quaternion& quat) 
{
    return quaternion(quat.w, -quat.x, -quat.y, -quat.z);

}   // End of conjugate()


//-------------------------------------------------------------------------------
// @ quaternion::conjugate()
//-------------------------------------------------------------------------------
// Set self to complex conjugate
//-------------------------------------------------------------------------------
const quaternion& 
quaternion::conjugate()
{
    x = -x;
    y = -y;
    z = -z;

    return *this;

}   // End of conjugate()


//-------------------------------------------------------------------------------
// @ ::inverse()
//-------------------------------------------------------------------------------
// Compute quaternion inverse
//-------------------------------------------------------------------------------
quaternion 
math3d::inverse(const quaternion& quat)
{
    float magnitude = quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z;
    // if we're the zero quaternion, just return identity
    if (!math3d::is_zero(magnitude))
    {
        ASSERT(false);
        return quaternion();
    }

    float normRecip = 1.0f / magnitude;
    return quaternion(normRecip*quat.w, -normRecip*quat.x, -normRecip*quat.y, 
                   -normRecip*quat.z);

}   // End of inverse()


//-------------------------------------------------------------------------------
// @ quaternion::inverse()
//-------------------------------------------------------------------------------
// Set self to inverse
//-------------------------------------------------------------------------------
const quaternion& 
quaternion::inverse()
{
    float magnitude = w*w + x*x + y*y + z*z;
    // if we're the zero quaternion, just return
    if (math3d::is_zero(magnitude))
        return *this;

    float normRecip = 1.0f / magnitude;
    w = normRecip*w;
    x = -normRecip*x;
    y = -normRecip*y;
    z = -normRecip*z;

    return *this;

}   // End of inverse()


//-------------------------------------------------------------------------------
// @ quaternion::operator+()
//-------------------------------------------------------------------------------
// Add quat to self and return
//-------------------------------------------------------------------------------
quaternion 
quaternion::operator+(const quaternion& other) const
{
    return quaternion(w + other.w, x + other.x, y + other.y, z + other.z);

}   // End of quaternion::operator+()


//-------------------------------------------------------------------------------
// @ quaternion::operator+=()
//-------------------------------------------------------------------------------
// Add quat to self, store in self
//-------------------------------------------------------------------------------
quaternion& 
quaternion::operator+=(const quaternion& other)
{
    w += other.w;
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;

}   // End of quaternion::operator+=()


//-------------------------------------------------------------------------------
// @ quaternion::operator-()
//-------------------------------------------------------------------------------
// Subtract quat from self and return
//-------------------------------------------------------------------------------
quaternion 
quaternion::operator-(const quaternion& other) const
{
    return quaternion(w - other.w, x - other.x, y - other.y, z - other.z);

}   // End of quaternion::operator-()


//-------------------------------------------------------------------------------
// @ quaternion::operator-=()
//-------------------------------------------------------------------------------
// Subtract quat from self, store in self
//-------------------------------------------------------------------------------
quaternion& 
quaternion::operator-=(const quaternion& other)
{
    w -= other.w;
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;

}   // End of quaternion::operator-=()


//-------------------------------------------------------------------------------
// @ quaternion::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
quaternion
quaternion::operator-() const
{
    return quaternion(-w, -x, -y, -z);
}    // End of quaternion::operator-()


//-------------------------------------------------------------------------------
// @ quaternion::operator*=()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
quaternion&
quaternion::operator*=(float scalar)
{
    w *= scalar;
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;

}   // End of quaternion::operator*=()


//-------------------------------------------------------------------------------
// @ quaternion::operator*()
//-------------------------------------------------------------------------------
// Quaternion multiplication
//-------------------------------------------------------------------------------
quaternion  
quaternion::operator*(const quaternion& other) const
{
    return quaternion(w*other.w - x*other.x - y*other.y - z*other.z,
                   w*other.x + x*other.w + y*other.z - z*other.y,
                   w*other.y + y*other.w + z*other.x - x*other.z,
                   w*other.z + z*other.w + x*other.y - y*other.x);

}   // End of quaternion::operator*()


//-------------------------------------------------------------------------------
// @ quaternion::operator*=()
//-------------------------------------------------------------------------------
// Quaternion multiplication by self
//-------------------------------------------------------------------------------
quaternion&
quaternion::operator*=(const quaternion& other)
{
    set(w*other.w - x*other.x - y*other.y - z*other.z,
         w*other.x + x*other.w + y*other.z - z*other.y,
         w*other.y + y*other.w + z*other.x - x*other.z,
         w*other.z + z*other.w + x*other.y - y*other.x);
  
    return *this;

}   // End of quaternion::operator*=()


//-------------------------------------------------------------------------------
// @ quaternion::dot()
//-------------------------------------------------------------------------------
// dot product by self
//-------------------------------------------------------------------------------
float               
quaternion::dot(const quaternion& quat) const
{
    return (w*quat.w + x*quat.x + y*quat.y + z*quat.z);

}   // End of quaternion::dot()


//-------------------------------------------------------------------------------
// @ dot()
//-------------------------------------------------------------------------------
// dot product friend operator
//-------------------------------------------------------------------------------
float               
math3d::dot(const quaternion& quat1, const quaternion& quat2)
{
    return (quat1.w*quat2.w + quat1.x*quat2.x + quat1.y*quat2.y + quat1.z*quat2.z);

}   // End of dot()


//-------------------------------------------------------------------------------
// @ quaternion::rotate()
//-------------------------------------------------------------------------------
// rotate vector by quaternion
// Assumes quaternion is normalized!
//-------------------------------------------------------------------------------
vector3   
quaternion::rotate(const vector3& vector) const
{
    ASSERT(is_unit());

    float v_mult = 2.0f*(x*vector.x + y*vector.y + z*vector.z);
    float cross_mult = 2.0f*w;
    float p_mult = cross_mult*w - 1.0f;

    return vector3(p_mult*vector.x + v_mult*x + cross_mult*(y*vector.z - z*vector.y),
                   p_mult*vector.y + v_mult*y + cross_mult*(z*vector.x - x*vector.z),
                   p_mult*vector.z + v_mult*z + cross_mult*(x*vector.y - y*vector.x));

}   // End of quaternion::rotate()

//-------------------------------------------------------------------------------
// @ quaternion::rotate()
//-------------------------------------------------------------------------------
// rotate point by quaternion
// Assumes quaternion is normalized!
//-------------------------------------------------------------------------------
point3   
quaternion::rotate(const point3& point) const
{
    ASSERT(is_unit());

    float v_mult = 2.0f*(x*point.x + y*point.y + z*point.z);
    float cross_mult = 2.0f*w;
    float p_mult = cross_mult*w - 1.0f;

    return point3(p_mult*point.x + v_mult*x + cross_mult*(y*point.z - z*point.y),
                  p_mult*point.y + v_mult*y + cross_mult*(z*point.x - x*point.z),
                  p_mult*point.z + v_mult*z + cross_mult*(x*point.y - y*point.x));

}   // End of quaternion::rotate()


//-------------------------------------------------------------------------------
// @ lerp()
//-------------------------------------------------------------------------------
// Linearly interpolate two quaternions
// This will always take the shorter path between them
//-------------------------------------------------------------------------------
void 
math3d::lerp(quaternion& result, const quaternion& start, const quaternion& end, float t)
{
    // get cos of "angle" between quaternions
    float cosTheta = start.dot(end);

    // initialize result
    result = t*end;

    // if "angle" between quaternions is less than 90 degrees
    if (cosTheta >= k_epsilon)
    {
        // use standard interpolation
        result += (1.0f-t)*start;
    }
    else
    {
        // otherwise, take the shorter path
        result += (t-1.0f)*start;
    }

}   // End of lerp()


//-------------------------------------------------------------------------------
// @ slerp()
//-------------------------------------------------------------------------------
// Spherical linearly interpolate two quaternions
// This will always take the shorter path between them
//-------------------------------------------------------------------------------
void 
math3d::slerp(quaternion& result, const quaternion& start, const quaternion& end, float t)
{
   ASSERT(t >= 0.0f && t <= 1.0f);

   // get cosine of "angle" between quaternions
   float cosTheta = start.dot(end);
   float startInterp, endInterp;

   // if "angle" between quaternions is less than 90 degrees
   if (cosTheta >= k_epsilon)
   {
      // if angle is greater than zero
      if ((1.0f - cosTheta) > k_epsilon)
      {
         // use standard slerp
         float theta = acosf(cosTheta);
         float recipSinTheta = 1.0f/math3d::sin(theta);

         startInterp = math3d::sin((1.0f - t)*theta)*recipSinTheta;
         endInterp = math3d::sin(t*theta)*recipSinTheta;
      }
      // angle is close to zero
      else
      {
         // use linear interpolation
         startInterp = 1.0f - t;
         endInterp = t;
      }
   }
   // otherwise, take the shorter route
   else
   {
      // if angle is less than 180 degrees
      if ((1.0f + cosTheta) > k_epsilon)
      {
         // use slerp w/negation of start quaternion
         float theta = acosf(-cosTheta);
         float recipSinTheta = 1.0f/math3d::sin(theta);

         startInterp = math3d::sin((t-1.0f)*theta)*recipSinTheta;
         endInterp = math3d::sin(t*theta)*recipSinTheta;
      }
      // angle is close to 180 degrees
      else
      {
         // use lerp w/negation of start quaternion
         startInterp = t - 1.0f;
         endInterp = t;
      }
   }

   result = startInterp*start + endInterp*end;

}   // End of slerp()


//-------------------------------------------------------------------------------
// @ approx_slerp()
//-------------------------------------------------------------------------------
// Approximate spherical linear interpolation of two quaternions
// Based on "Hacking Quaternions", Jonathan Blow, Game Developer, March 2002.
// See Game Developer, February 2004 for an alternate method.
//-------------------------------------------------------------------------------
void 
math3d::approx_slerp(quaternion& result, const quaternion& start, const quaternion& end, float t)
{
    float cosTheta = start.dot(end);

    // correct time by using cosine of angle between quaternions
    float factor = 1.0f - 0.7878088f*cosTheta;
    float k = 0.5069269f;
    factor *= factor;
    k *= factor;

    float b = 2*k;
    float c = -3*k;
    float d = 1 + k;

    t = t*(b*t + c) + d;

    // initialize result
    result = t*end;

    // if "angle" between quaternions is less than 90 degrees
    if (cosTheta >= k_epsilon)
    {
        // use standard interpolation
        result += (1.0f-t)*start;
    }
    else
    {
        // otherwise, take the shorter path
        result += (t-1.0f)*start;
    }

}   // End of approx_slerp()

