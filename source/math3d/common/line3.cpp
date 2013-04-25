#include "radiant.h"
#include "common.h"
#include "radiant.pb.h"
#include "line3.h"

using namespace radiant;
using namespace radiant::math3d;

//----------------------------------------------------------------------------
// @ line3.cpp
// 
// 3D line class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------

#include "line3.h"
#include "matrix4.h"
#include "quaternion.h"
#include "vector3.h"

//----------------------------------------------------------------------------
//-- Static Variables --------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Functions ---------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// @ line3::line3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
line3::line3() :
    origin(0.0f, 0.0f, 0.0f),
    direction(1.0f, 0.0f, 0.0f)
{
}   // End of line3::line3()


//----------------------------------------------------------------------------
// @ line3::line3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
line3::line3(const point3& o, const vector3& d) :
    origin(o),
    direction(d)
{
    direction.normalize();
}   // End of line3::line3()


//----------------------------------------------------------------------------
// @ line3::line3()
// ---------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------
line3::line3(const line3& other) :
    origin(other.origin),
    direction(other.direction)
{
    direction.normalize();

}   // End of line3::line3()


//----------------------------------------------------------------------------
// @ line3::operator=()
// ---------------------------------------------------------------------------
// Assigment operator
//-----------------------------------------------------------------------------
line3&
line3::operator=(const line3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    origin = other.origin;
    direction = other.direction;
    direction.normalize();

    return *this;

}   // End of line3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Output a text line3.  The format is assumed to be :
// [ vector3, vector3 ]
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const line3& source)
{
    return out << "[" << source.origin << ", " << source.direction << "]";
    
}   // End of operator<<()

//----------------------------------------------------------------------------
// @ line3::operator==()
// ---------------------------------------------------------------------------
// Are two line3's equal?
//----------------------------------------------------------------------------
bool
line3::operator==(const line3& ray) const
{
    return (ray.origin == origin && ray.direction == direction);

}  // End of line3::operator==()


//----------------------------------------------------------------------------
// @ line3::operator!=()
// ---------------------------------------------------------------------------
// Are two line3's not equal?
//----------------------------------------------------------------------------
bool
line3::operator!=(const line3& ray) const
{
    return !(ray.origin == origin && ray.direction == direction);
}  // End of line3::operator!=()


//----------------------------------------------------------------------------
// @ line3::set()
// ---------------------------------------------------------------------------
// Sets the two parameters
//-----------------------------------------------------------------------------
void
line3::set(const point3& o, const vector3& d)
{
    origin = o;
    direction = d;
    direction.normalize();

}   // End of line3::set()


//----------------------------------------------------------------------------
// @ line3::transform()
// ---------------------------------------------------------------------------
// Transforms ray into new space
//-----------------------------------------------------------------------------
line3 
line3::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    line3 line;
    matrix4    transform(rotate);
    transform(0,0) *= scale;
    transform(1,0) *= scale;
    transform(2,0) *= scale;
    transform(0,1) *= scale;
    transform(1,1) *= scale;
    transform(2,1) *= scale;
    transform(0,2) *= scale;
    transform(1,2) *= scale;
    transform(2,2) *= scale;

    line.direction = transform.transform(direction);
    line.direction.normalize();

    transform(0,3) = translate.x;
    transform(1,3) = translate.y;
    transform(2,3) = translate.z;

    line.origin = transform.transform(origin);

    return line;

}   // End of line3::transform()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between lines.
//-----------------------------------------------------------------------------
float math3d::distance_squared(const line3& line0, const line3& line1, 
                       float& s_c, float& t_c)
{
    vector3 w0 = vector3(line0.origin - line1.origin);
    float a = line0.direction.dot(line0.direction);
    float b = line0.direction.dot(line1.direction);
    float c = line1.direction.dot(line1.direction);
    float d = line0.direction.dot(w0);
    float e = line1.direction.dot(w0);
    float denom = a*c - b*b;
    if (math3d::is_zero(denom))
    {
        s_c = 0.0f;
        t_c = e/c;
        vector3 wc = w0 - t_c*line1.direction;
        return wc.dot(wc);
    }
    else
    {
        s_c = ((b*e - c*d)/denom);
        t_c = ((a*e - b*d)/denom);
        vector3 wc = w0 + s_c*line0.direction
                          - t_c*line1.direction;
        return wc.dot(wc);
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between line and point.
//-----------------------------------------------------------------------------
float math3d::distance_squared(const line3& line, const point3& point, float& t_c)
{
    vector3 w = vector3(point - line.origin);
    float vsq = line.direction.dot(line.direction);
    float proj = w.dot(line.direction);
    t_c = proj/vsq; 

    return w.dot(w) - t_c*proj;

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between two lines
//-----------------------------------------------------------------------------
void math3d::closest_points(point3& point0, point3& point1, 
                    const line3& line0, 
                    const line3& line1)
{
    // compute intermediate parameters
    vector3 w0 = vector3(line0.origin - line1.origin);
    float a = line0.direction.dot(line0.direction);
    float b = line0.direction.dot(line1.direction);
    float c = line1.direction.dot(line1.direction);
    float d = line0.direction.dot(w0);
    float e = line1.direction.dot(w0);

    float denom = a*c - b*b;

    if (math3d::is_zero(denom))
    {
        point0 = line0.origin;
        point1 = line1.origin + (e/c)*line1.direction;
    }
    else
    {
        point0 = line0.origin + ((b*e - c*d)/denom)*line0.direction;
        point1 = line1.origin + ((a*e - b*d)/denom)*line1.direction;
    }

}   // End of closest_points()


//----------------------------------------------------------------------------
// @ line3::closest_point()
// ---------------------------------------------------------------------------
// Returns the closest point on line to point.
//-----------------------------------------------------------------------------
point3 line3::closest_point(const point3& point) const
{
    vector3 w = vector3(point - origin);
    float vsq = direction.dot(direction);
    float proj = w.dot(direction);

    return origin + (proj/vsq)*direction;

}   // End of line3::closest_point()
