//----------------------------------------------------------------------------
// @ ray3.cpp
// 
// 3D ray class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------

#include "radiant.h"
#include "radiant.pb.h"
#include "ray3.h"
#include "line3.h"
#include "matrix4.h"
#include "quaternion.h"
#include "vector3.h"

using namespace radiant;
using namespace radiant::math3d;

//----------------------------------------------------------------------------
// @ ray3::ray3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ray3::ray3() :
    origin(0.0f, 0.0f, 0.0f),
    direction(1.0f, 0.0f, 0.0f)
{
}   // End of ray3::ray3()


//----------------------------------------------------------------------------
// @ ray3::ray3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
ray3::ray3(const point3& o, const vector3& d) :
    origin(o),
    direction(d)
{
    direction.normalize();
}   // End of ray3::ray3()


//----------------------------------------------------------------------------
// @ ray3::ray3()
// ---------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------
ray3::ray3(const ray3& other) :
    origin(other.origin),
    direction(other.direction)
{
    direction.normalize();

}   // End of ray3::ray3()


//----------------------------------------------------------------------------
// @ ray3::operator=()
// ---------------------------------------------------------------------------
// Assigment operator
//-----------------------------------------------------------------------------
ray3&
ray3::operator=(const ray3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    origin = other.origin;
    direction = other.direction;
    direction.normalize();

    return *this;
}   // End of ray3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Output a text ray3.  The format is assumed to be :
// [ vector3, vector3 ]
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const ray3& source)
{
    return out << "[" << source.origin 
                     << ", " << source.direction << "]";
    
}   // End of operator<<()
    

//----------------------------------------------------------------------------
// @ ray3::operator==()
// ---------------------------------------------------------------------------
// Are two ray3's equal?
//----------------------------------------------------------------------------
bool
ray3::operator==(const ray3& ray) const
{
    return (ray.origin == origin && ray.direction == direction);

}  // End of ray3::operator==()


//----------------------------------------------------------------------------
// @ ray3::operator!=()
// ---------------------------------------------------------------------------
// Are two ray3's not equal?
//----------------------------------------------------------------------------
bool
ray3::operator!=(const ray3& ray) const
{
    return !(ray.origin == origin && ray.direction == direction);
}  // End of ray3::operator!=()


//----------------------------------------------------------------------------
// @ ray3::set()
// ---------------------------------------------------------------------------
// Sets the two parameters
//-----------------------------------------------------------------------------
void
ray3::set(const point3& o, const vector3& d)
{
    origin = o;
    direction = d;
    direction.normalize();

}   // End of ray3::set()


//----------------------------------------------------------------------------
// @ ray3::transform()
// ---------------------------------------------------------------------------
// Transforms ray into new space
//-----------------------------------------------------------------------------
ray3  
ray3::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    ray3 ray;
    matrix4 transform(rotate);
    transform(0,0) *= scale;
    transform(1,0) *= scale;
    transform(2,0) *= scale;
    transform(0,1) *= scale;
    transform(1,1) *= scale;
    transform(2,1) *= scale;
    transform(0,2) *= scale;
    transform(1,2) *= scale;
    transform(2,2) *= scale;

    ray.direction = transform.transform(direction);
    ray.direction.normalize();

    transform(0,3) = translate.x;
    transform(1,3) = translate.y;
    transform(2,3) = translate.z;

    ray.origin = transform.transform(origin);

    return ray;

}   // End of ray3::transform()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between rays.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
distance_squared(const ray3& ray0, const ray3& ray1, 
                 float& s_c, float& t_c)
{
    // compute intermediate parameters
    vector3 w0 = vector3(ray0.origin - ray1.origin);
    float a = ray0.direction.dot(ray0.direction);
    float b = ray0.direction.dot(ray1.direction);
    float c = ray1.direction.dot(ray1.direction);
    float d = ray0.direction.dot(w0);
    float e = ray1.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (math3d::is_zero(denom))
    {
        // clamp s_c to 0
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // clamp s_c within [0,+inf]
        sd = td = denom;
        sn = b*e - c*d;
        tn = a*e - b*d;
  
        // clamp s_c to 0
        if (sn < 0.0f)
        {
            sn = 0.0f;
            tn = e;
            td = c;
        }
    }

    // clamp t_c within [0,+inf]
    // clamp t_c to 0
    if (tn < 0.0f)
    {
        t_c = 0.0f;
        // clamp s_c to 0
        if (-d < 0.0f)
        {
            s_c = 0.0f;
        }
        else
        {
            s_c = -d/a;
        }
    }
    else
    {
        t_c = tn/td;
        s_c = sn/sd;
    }

    // compute difference vector and distance squared
    vector3 wc = w0 + s_c*ray0.direction - t_c*ray1.direction;
    return wc.dot(wc);

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ray3::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between ray and line.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
distance_squared(const ray3& ray, const line3& line, 
                   float& s_c, float& t_c)
{
    // compute intermediate parameters
    vector3 w0 = vector3(ray.origin - line.origin);
    float a = ray.direction.dot(ray.direction);
    float b = ray.direction.dot(line.direction);
    float c = line.direction.dot(line.direction);
    float d = ray.direction.dot(w0);
    float e = line.direction.dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (math3d::is_zero(denom))
    {
        s_c = 0.0f;
        t_c = e/c;
        // compute difference vector and distance squared
        vector3 wc = w0 - t_c*line.direction;
        return wc.dot(wc);
    }
    else
    {
        // parameters to compute s_c, t_c
        float sn;

        // clamp s_c within [0,1]
        sn = b*e - c*d;
  
        // clamp s_c to 0
        if (sn < 0.0f)
        {
            s_c = 0.0f;
            t_c = e/c;
        }
        // clamp s_c to 1
        else if (sn > denom)
        {
            s_c = 1.0f;
            t_c = (e+b)/c;
        }
        else
        {
            s_c = sn/denom;
            t_c = (a*e - b*d)/denom;
        }

        // compute difference vector and distance squared
        vector3 wc = w0 + s_c*ray.direction - t_c*line.direction;
        return wc.dot(wc);
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between ray and point.
//-----------------------------------------------------------------------------
float distance_squared(const ray3& ray, const point3& point, 
                       float& t_c) 
{
    vector3 w = vector3(point - ray.origin);
    float proj = w.dot(ray.direction);
    // origin is closest point
    if (proj <= 0)
    {
        t_c = 0.0f;
        return w.dot(w);
    }
    // else use normal line check
    else
    {
        float vsq = ray.direction.dot(ray.direction);
        t_c = proj/vsq;
        return w.dot(w) - t_c*proj;
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between two rays
//-----------------------------------------------------------------------------
void closest_points(point3& point0, point3& point1, 
                    const ray3& ray0, 
                    const ray3& ray1)
{
    // compute intermediate parameters
    vector3 w0 = vector3(ray0.origin - ray1.origin);
    float a = ray0.direction.dot(ray0.direction);
    float b = ray0.direction.dot(ray1.direction);
    float c = ray1.direction.dot(ray1.direction);
    float d = ray0.direction.dot(w0);
    float e = ray1.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float s_c, t_c;
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (math3d::is_zero(denom))
    {
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // start by clamping s_c
        sd = td = denom;
        sn = b*e - c*d;
        tn = a*e - b*d;
  
        // clamp s_c to 0
        if (sn < 0.0f)
        {
            sn = 0.0f;
            tn = e;
            td = c;
        }
    }

    // clamp t_c within [0,+inf]
    // clamp t_c to 0
    if (tn < 0.0f)
    {
        t_c = 0.0f;
        // clamp s_c to 0
        if (-d < 0.0f)
        {
            s_c = 0.0f;
        }
        else
        {
            s_c = -d/a;
        }
    }
    else
    {
        t_c = tn/td;
        s_c = sn/sd;
    }

    // compute closest points
    point0 = ray0.origin + s_c*ray0.direction;
    point1 = ray1.origin + t_c*ray1.direction;
}


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between ray and line
//-----------------------------------------------------------------------------
void closest_points(point3& point0, point3& point1, 
                    const ray3& ray, 
                    const line3& line)
{
    // compute intermediate parameters
    vector3 w0 = vector3(ray.origin - line.origin);
    float a = ray.direction.dot(ray.direction);
    float b = ray.direction.dot(line.direction);
    float c = line.direction.dot(line.direction);
    float d = ray.direction.dot(w0);
    float e = line.direction.dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (math3d::is_zero(denom))
    {
        // compute closest points
        point0 = ray.origin;
        point1 = line.origin + (e/c)*line.direction;
    }
    else
    {
        // parameters to compute s_c, t_c
        float sn, s_c, t_c;

        // clamp s_c within [0,1]
        sn = b*e - c*d;
  
        // clamp s_c to 0
        if (sn < 0.0f)
        {
            s_c = 0.0f;
            t_c = e/c;
        }
        // clamp s_c to 1
        else if (sn > denom)
        {
            s_c = 1.0f;
            t_c = (e+b)/c;
        }
        else
        {
            s_c = sn/denom;
            t_c = (a*e - b*d)/denom;
        }

        // compute closest points
        point0 = ray.origin + s_c*ray.direction;
        point1 = line.origin + t_c*line.direction;
    }

}   // End of closest_points()


//----------------------------------------------------------------------------
// @ ray3::closest_point()
// ---------------------------------------------------------------------------
// Returns the closest point on ray to point
//-----------------------------------------------------------------------------
point3 
ray3::closest_point(const point3& point) const
{
    vector3 w = vector3(point - origin);
    float proj = w.dot(direction);
    // endpoint 0 is closest point
    if (proj <= 0.0f)
        return origin;
    else
    {
        float vsq = direction.dot(direction);
        // else somewhere else in ray
        return origin + (proj/vsq)*direction;
    }
}
