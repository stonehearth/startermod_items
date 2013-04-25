#include "radiant.h"
#include "radiant.pb.h"
#include <iomanip>
#include "common.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ point3.cpp
// 
// 3D vector class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "radiant.h"
#include "ipoint3.h"
#include "point3.h"
#include "vector3.h"

using namespace radiant;
using namespace radiant::math3d;

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

point3 point3::unit_x(1.0f, 0.0f, 0.0f);
point3 point3::unit_y(0.0f, 1.0f, 0.0f);
point3 point3::unit_z(0.0f, 0.0f, 1.0f);
point3 point3::origin(0.0f, 0.0f, 0.0f);

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ point3::point3()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
point3::point3(const point3& other) : 
    x(other.x),
    y(other.y),
    z(other.z)
{

}   // End of point3::point3()

point3::point3(const ipoint3& other) : 
    x((float)other.x),
    y((float)other.y),
    z((float)other.z)
{

}   // End of point3::point3()

//-------------------------------------------------------------------------------
// @ point3::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
point3&
point3::operator=(const point3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    x = other.x;
    y = other.y;
    z = other.z;
    
    return *this;

}   // End of point3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const point3& source)
{
    out << std::fixed << std::setprecision(4) << '<' << source.x << ", " << source.y << ", " << source.z << '>';

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ point3::length()
//-------------------------------------------------------------------------------
// Vector length
//-------------------------------------------------------------------------------
float 
point3::length() const
{
    return math3d::sqrt(x*x + y*y + z*z);

}   // End of point3::length()


//-------------------------------------------------------------------------------
// @ point3::length_squared()
//-------------------------------------------------------------------------------
// Vector length squared (avoids square root)
//-------------------------------------------------------------------------------
float 
point3::length_squared() const
{
    return (x*x + y*y + z*z);

}   // End of point3::length_squared()


//-------------------------------------------------------------------------------
// @ ::distance()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
point3::distance(const point3& other) const
{
    return math3d::sqrt(distance_squared(other));
}   // End of point3::length()

float 
point3::distance_squared(const point3& other) const
{
    float x_ = x - other.x;
    float y_ = y - other.y;
    float z_ = z - other.z;

    return x_*x_ + y_*y_ + z_*z_;

}   // End of point3::length()

//-------------------------------------------------------------------------------
// @ ::distance()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
math3d::distance(const point3& p0, const point3& p1)
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return math3d::sqrt(x*x + y*y + z*z);

}   // End of point3::length()


//-------------------------------------------------------------------------------
// @ ::distance_squared()
//-------------------------------------------------------------------------------
// Point distance
//-------------------------------------------------------------------------------
float 
math3d::distance_squared(const point3& p0, const point3& p1)
{
    float x = p0.x - p1.x;
    float y = p0.y - p1.y;
    float z = p0.z - p1.z;

    return (x*x + y*y + z*z);

}   // End of ::distance_squared()


//-------------------------------------------------------------------------------
// @ point3::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
point3::operator==(const point3& other) const
{
    if (::math3d::are_equal(other.x, x)
        && ::math3d::are_equal(other.y, y)
        && ::math3d::are_equal(other.z, z))
        return true;

    return false;   
}   // End of point3::operator==()


//-------------------------------------------------------------------------------
// @ point3::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
point3::operator!=(const point3& other) const
{
    if (::math3d::are_equal(other.x, x)
        && ::math3d::are_equal(other.y, y)
        && ::math3d::are_equal(other.z, z))
        return false;

    return true;
}   // End of point3::operator!=()


//-------------------------------------------------------------------------------
// @ vector3math::is_zero()
//-------------------------------------------------------------------------------
// Check for zero vector
//-------------------------------------------------------------------------------
bool 
point3::is_zero() const
{
    return math3d::is_zero(x*x + y*y + z*z);

}   // End of vector3math::is_zero()


//-------------------------------------------------------------------------------
// @ point3::is_unit()
//-------------------------------------------------------------------------------
// Check for unit vector
//-------------------------------------------------------------------------------
bool 
point3::is_unit() const
{
    return math3d::is_zero(1.0f - x*x - y*y - z*z);

}   // End of point3::is_unit()


//-------------------------------------------------------------------------------
// @ point3::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
point3::clean()
{
    if (math3d::is_zero(x))
        x = 0.0f;
    if (math3d::is_zero(y))
        y = 0.0f;
    if (math3d::is_zero(z))
        z = 0.0f;

}   // End of point3::clean()


//-------------------------------------------------------------------------------
// @ point3::normalize()
//-------------------------------------------------------------------------------
// Set to unit vector
//-------------------------------------------------------------------------------
void
point3::normalize()
{
    float lengthsq = x*x + y*y + z*z;

    if (math3d::is_zero(lengthsq))
    {
        set_zero();
    }
    else
    {
        float factor = math3d::inv_sqrt(lengthsq);
        x *= factor;
        y *= factor;
        z *= factor;
    }

}   // End of point3::normalize()


//-------------------------------------------------------------------------------
// @ point3::operator+()
//-------------------------------------------------------------------------------
// Add point to self and return
//-------------------------------------------------------------------------------
point3 
point3::operator+(const point3& other) const
{
    return point3(x + other.x, y + other.y, z + other.z);

}   // End of point3::operator+()


//-------------------------------------------------------------------------------
// @ point3::operator+()
//-------------------------------------------------------------------------------
// Add vector to self and return
//-------------------------------------------------------------------------------
point3 
point3::operator+(const vector3& other) const
{
    return point3(x + other.x, y + other.y, z + other.z);

}   // End of point3::operator+()


//-------------------------------------------------------------------------------
// @ point3::operator+=()
//-------------------------------------------------------------------------------
// Add vector to self, store in self
//-------------------------------------------------------------------------------
point3& 
math3d::operator+=(point3& self, const vector3& other)
{
    self.x += other.x;
    self.y += other.y;
    self.z += other.z;

    return self;

}   // End of point3::operator+=()

//-------------------------------------------------------------------------------
// @ point3::operator+=()
//-------------------------------------------------------------------------------
// Add point to self, store in self
//-------------------------------------------------------------------------------
point3& 
math3d::operator+=(point3& self, const point3& other)
{
    self.x += other.x;
    self.y += other.y;
    self.z += other.z;

    return self;

}   // End of point3::operator+=()


//-------------------------------------------------------------------------------
// @ point3::operator-()
//-------------------------------------------------------------------------------
// Subtract vector from self and return
//-------------------------------------------------------------------------------
point3 
point3::operator-(const point3& other) const
{
    return point3(x - other.x, y - other.y, z - other.z);

}   // End of point3::operator-()


//-------------------------------------------------------------------------------
// @ point3::operator-()
//-------------------------------------------------------------------------------
// Subtract vector from self and return
//-------------------------------------------------------------------------------
point3 
point3::operator-(const vector3& other) const
{
    return point3(x - other.x, y - other.y, z - other.z);

}   // End of point3::operator-()


//-------------------------------------------------------------------------------
// @ point3::operator-=()
//-------------------------------------------------------------------------------
// Subtract vector from self, store in self
//-------------------------------------------------------------------------------
point3& 
math3d::point3::operator-=(const vector3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
}   // End of point3::operator-=()

//-------------------------------------------------------------------------------
// @ point3::operator-=()
//-------------------------------------------------------------------------------
// Subtract vector from self, store in self
//-------------------------------------------------------------------------------
point3& 
math3d::point3::operator-=(const point3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;
}   // End of point3::operator-=()

//-------------------------------------------------------------------------------
// @ point3::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
point3
point3::operator-() const
{
    return point3(-x, -y, -z);
}    // End of point3::operator-()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
point3   
point3::operator*(float scalar)
{
    return point3(scalar*x, scalar*y, scalar*z);

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication
//-------------------------------------------------------------------------------
point3   
math3d::operator*(float scalar, const point3& vector)
{
    return point3(scalar*vector.x, scalar*vector.y, scalar*vector.z);

}   // End of operator*()


//-------------------------------------------------------------------------------
// @ point3::operator*()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
point3&
point3::operator*=(float scalar)
{
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;

}   // End of point3::operator*=()


//-------------------------------------------------------------------------------
// @ operator/()
//-------------------------------------------------------------------------------
// Scalar division
//-------------------------------------------------------------------------------
point3   
point3::operator/(float scalar)
{
    return point3(x/scalar, y/scalar, z/scalar);

}   // End of operator/()


//-------------------------------------------------------------------------------
// @ point3::operator/=()
//-------------------------------------------------------------------------------
// Scalar division by self
//-------------------------------------------------------------------------------
point3&
point3::operator/=(float scalar)
{
    x /= scalar;
    y /= scalar;
    z /= scalar;

    return *this;

}   // End of point3::operator/=()

math3d::point3 math3d::interpolate(const math3d::point3 &p0, const math3d::point3 &p1, float alpha)
{
   if (alpha <= 0.0f) {
      return p0;
   }
   if (alpha >= 1.0f) {
      return p1;
   }
   math3d::point3 delta = p1 - p0;
   return p0 + (delta * alpha);
}
