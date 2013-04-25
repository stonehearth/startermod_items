#include "radiant.h"
#include "radiant.pb.h"
#include "common.h"
#include "line_segment3.h"

using namespace radiant;
using namespace radiant::math3d;

//----------------------------------------------------------------------------
// @ line_segment3.cpp
// 
// 3D line segment class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------

#include "ray3.h"
#include "line3.h"
#include "matrix3.h"
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
// @ line_segment3::line_segment3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
line_segment3::line_segment3() :
    origin(0.0f, 0.0f, 0.0f),
    direction(1.0f, 0.0f, 0.0f)
{
}   // End of line_segment3::line_segment3()


//----------------------------------------------------------------------------
// @ line_segment3::line_segment3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
line_segment3::line_segment3(const point3& endpoint0, const point3& endpoint1) :
    origin(endpoint0),
    direction(endpoint1-endpoint0)
{
}   // End of line_segment3::line_segment3()


//----------------------------------------------------------------------------
// @ line_segment3::line_segment3()
// ---------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------
line_segment3::line_segment3(const line_segment3& other) :
    origin(other.origin),
    direction(other.direction)
{
}   // End of line_segment3::line_segment3()


//----------------------------------------------------------------------------
// @ line_segment3::operator=()
// ---------------------------------------------------------------------------
// Assigment operator
//-----------------------------------------------------------------------------
line_segment3&
line_segment3::operator=(const line_segment3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    origin = other.origin;
    direction = other.direction;

    return *this;

}   // End of line_segment3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Output a text line_segment3.  The format is assumed to be :
// [ vector3, vector3 ]
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const line_segment3& source)
{
    return out << "[" << source.get_endpoint_0() << ", " << source.get_endpoint_1() << "]";
    
}   // End of operator<<()
    

//----------------------------------------------------------------------------
// @ line_segment3::length()
// ---------------------------------------------------------------------------
// Returns the distance between two endpoints
//-----------------------------------------------------------------------------
float
line_segment3::length() const
{
    return direction.length();

}   // End of line_segment3::length()


//----------------------------------------------------------------------------
// @ line_segment3::length_squared()
// ---------------------------------------------------------------------------
// Returns the squared distance between two endpoints
//-----------------------------------------------------------------------------
float
line_segment3::length_squared() const
{
    return direction.length_squared();

}   // End of line_segment3::length_squared()


//----------------------------------------------------------------------------
// @ line_segment3::operator==()
// ---------------------------------------------------------------------------
// Are two line_segment3's equal?
//----------------------------------------------------------------------------
bool
line_segment3::operator==(const line_segment3& segment) const
{
    return ((segment.origin == origin && segment.direction == direction) ||
            (segment.origin == origin+direction && segment.direction == -direction));

}  // End of line_segment3::operator==()


//----------------------------------------------------------------------------
// @ line_segment3::operator!=()
// ---------------------------------------------------------------------------
// Are two line_segment3's not equal?
//----------------------------------------------------------------------------
bool
line_segment3::operator!=(const line_segment3& segment) const
{
    return !((segment.origin == origin && segment.direction == direction) ||
            (segment.origin == origin+direction && segment.direction == -direction));
}  // End of line_segment3::operator!=()


//----------------------------------------------------------------------------
// @ line_segment3::set()
// ---------------------------------------------------------------------------
// sets the two endpoints
//-----------------------------------------------------------------------------
void
line_segment3::set(const point3& endpoint0, const point3& endpoint1)
{
    origin = endpoint0;
    direction = vector3(endpoint1 - endpoint0);

}   // End of line_segment3::set()


//----------------------------------------------------------------------------
// @ line_segment3::transform()
// ---------------------------------------------------------------------------
// Transforms segment into new space
//-----------------------------------------------------------------------------
line_segment3  
line_segment3::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    line_segment3 segment;
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

    segment.direction = transform.transform(direction);

    transform(0,3) = translate.x;
    transform(1,3) = translate.y;
    transform(2,3) = translate.z;

    segment.origin = transform.transform(origin);

    return segment;

}   // End of line_segment3::transform()


//----------------------------------------------------------------------------
// @ line_segment3::transform()
// ---------------------------------------------------------------------------
// Transforms segment into new space
//-----------------------------------------------------------------------------
line_segment3  
line_segment3::transform(float scale, const matrix3& rotate, 
                           const vector3& translate) const
{
    line_segment3 segment;
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

    segment.direction = transform.transform(direction);

    transform(0,3) = translate.x;
    transform(1,3) = translate.y;
    transform(2,3) = translate.z;

    segment.origin = transform.transform(origin);

    return segment;

}   // End of line_segment3::transform()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between two line segments.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
math3d::distance_squared(const line_segment3& segment0, const line_segment3& segment1, 
                 float& s_c, float& t_c)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment0.origin - segment1.origin);
    float a = segment0.direction.dot(segment0.direction);
    float b = segment0.direction.dot(segment1.direction);
    float c = segment1.direction.dot(segment1.direction);
    float d = segment0.direction.dot(w0);
    float e = segment1.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on segment1 to origin0
    if (math3d::is_zero(denom))
    {
        // clamp s_c to 0
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // clamp s_c within [0,1]
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
        // clamp s_c to 1
        else if (sn > sd)
        {
            sn = sd;
            tn = e + b;
            td = c;
        }
    }

    // clamp t_c within [0,1]
    // clamp t_c to 0
    if (tn < 0.0f)
    {
        t_c = 0.0f;
        // clamp s_c to 0
        if (-d < 0.0f)
        {
            s_c = 0.0f;
        }
        // clamp s_c to 1
        else if (-d > a)
        {
            s_c = 1.0f;
        }
        else
        {
            s_c = -d/a;
        }
    }
    // clamp t_c to 1
    else if (tn > td)
    {
        t_c = 1.0f;
        // clamp s_c to 0
        if ((-d+b) < 0.0f)
        {
            s_c = 0.0f;
        }
        // clamp s_c to 1
        else if ((-d+b) > a)
        {
            s_c = 1.0f;
        }
        else
        {
            s_c = (-d+b)/a;
        }
    }
    else
    {
        t_c = tn/td;
        s_c = sn/sd;
    }

    // compute difference vector and distance squared
    vector3 wc = w0 + s_c*segment0.direction - t_c*segment1.direction;
    return wc.dot(wc);

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between line segment and ray.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
math3d::distance_squared(const line_segment3& segment, const ray3& ray,
                 float& s_c, float& t_c)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment.origin - ray.origin);
    float a = segment.direction.dot(segment.direction);
    float b = segment.direction.dot(ray.direction);
    float c = ray.direction.dot(ray.direction);
    float d = segment.direction.dot(w0);
    float e = ray.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on segment1 to origin0
    if (math3d::is_zero(denom))
    {
        // clamp s_c to 0
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // clamp s_c within [0,1]
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
        // clamp s_c to 1
        else if (sn > sd)
        {
            sn = sd;
            tn = e + b;
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
        // clamp s_c to 1
        else if (-d > a)
        {
            s_c = 1.0f;
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
    vector3 wc = w0 + s_c*segment.direction - t_c*ray.direction;
    return wc.dot(wc);

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between line segment and line.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
math3d::distance_squared(const line_segment3& segment, const line3& line,
                 float& s_c, float& t_c)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment.origin - line.origin);
    float a = segment.direction.dot(segment.direction);
    float b = segment.direction.dot(line.direction);
    float c = line.direction.dot(line.direction);
    float d = segment.direction.dot(w0);
    float e = line.direction.dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on segment1 to origin0
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
        vector3 wc = w0 + s_c*segment.direction - t_c*line.direction;
        return wc.dot(wc);
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ line_segment3::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between line segment and point.
//-----------------------------------------------------------------------------
float 
math3d::distance_squared(const line_segment3& segment, const point3& point, 
                 float& t_c) 
{
    vector3 w = vector3(point - segment.origin);
    float proj = w.dot(segment.direction);
    // endpoint 0 is closest point
    if (proj <= 0)
    {
        t_c = 0.0f;
        return w.dot(w);
    }
    else
    {
        float vsq = segment.direction.dot(segment.direction);
        // endpoint 1 is closest point
        if (proj >= vsq)
        {
            t_c = 1.0f;
            return w.dot(w) - 2.0f*proj + vsq;
        }
        // otherwise somewhere else in segment
        else
        {
            t_c = proj/vsq;
            return w.dot(w) - t_c*proj;
        }
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between two line segments.
//-----------------------------------------------------------------------------
void math3d::closest_points(point3& point0, point3& point1, 
                    const line_segment3& segment0, 
                    const line_segment3& segment1)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment0.origin - segment1.origin);
    float a = segment0.direction.dot(segment0.direction);
    float b = segment0.direction.dot(segment1.direction);
    float c = segment1.direction.dot(segment1.direction);
    float d = segment0.direction.dot(w0);
    float e = segment1.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float s_c, t_c;
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on segment1 to origin0
    if (math3d::is_zero(denom))
    {
        // clamp s_c to 0
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // clamp s_c within [0,1]
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
        // clamp s_c to 1
        else if (sn > sd)
        {
            sn = sd;
            tn = e + b;
            td = c;
        }
    }

    // clamp t_c within [0,1]
    // clamp t_c to 0
    if (tn < 0.0f)
    {
        t_c = 0.0f;
        // clamp s_c to 0
        if (-d < 0.0f)
        {
            s_c = 0.0f;
        }
        // clamp s_c to 1
        else if (-d > a)
        {
            s_c = 1.0f;
        }
        else
        {
            s_c = -d/a;
        }
    }
    // clamp t_c to 1
    else if (tn > td)
    {
        t_c = 1.0f;
        // clamp s_c to 0
        if ((-d+b) < 0.0f)
        {
            s_c = 0.0f;
        }
        // clamp s_c to 1
        else if ((-d+b) > a)
        {
            s_c = 1.0f;
        }
        else
        {
            s_c = (-d+b)/a;
        }
    }
    else
    {
        t_c = tn/td;
        s_c = sn/sd;
    }

    // compute closest points
    point0 = segment0.origin + s_c*segment0.direction;
    point1 = segment1.origin + t_c*segment1.direction;

}   // End of closest_points()


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between line segment and ray.
//-----------------------------------------------------------------------------
void math3d::closest_points(point3& point0, point3& point1, 
                    const line_segment3& segment, 
                    const ray3& ray)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment.origin - ray.origin);
    float a = segment.direction.dot(segment.direction);
    float b = segment.direction.dot(ray.direction);
    float c = ray.direction.dot(ray.direction);
    float d = segment.direction.dot(w0);
    float e = ray.direction.dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float s_c, t_c;
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on segment1 to origin0
    if (math3d::is_zero(denom))
    {
        // clamp s_c to 0
        sd = td = c;
        sn = 0.0f;
        tn = e;
    }
    else
    {
        // clamp s_c within [0,1]
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
        // clamp s_c to 1
        else if (sn > sd)
        {
            sn = sd;
            tn = e + b;
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
        // clamp s_c to 1
        else if (-d > a)
        {
            s_c = 1.0f;
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
    point0 = segment.origin + s_c*segment.direction;
    point1 = ray.origin + t_c*ray.direction;

}   // End of closest_points()


//----------------------------------------------------------------------------
// @ line_segment3::closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between line segment and line.
//-----------------------------------------------------------------------------
void closest_points(point3& point0, point3& point1, 
                    const line_segment3& segment, 
                    const line3& line)
{
    // compute intermediate parameters
    vector3 w0 = vector3(segment.origin - line.origin);
    float a = segment.direction.dot(segment.direction);
    float b = segment.direction.dot(line.direction);
    float c = line.direction.dot(line.direction);
    float d = segment.direction.dot(w0);
    float e = line.direction.dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on line to segment origin
    if (math3d::is_zero(denom))
    {
        // compute closest points
        point0 = segment.origin;
        point1 = line.origin + (e/c)*line.direction;
    }
    else
    {
        // parameters to compute s_c, t_c
        float s_c, t_c;
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

        // compute closest points
        point0 = segment.origin + s_c*segment.direction;
        point1 = line.origin + t_c*line.direction;
    }

}   // End of closest_points()



//----------------------------------------------------------------------------
// @ line_segment3::closest_point()
// ---------------------------------------------------------------------------
// Returns the closest point on line segment to point
//-----------------------------------------------------------------------------
point3 line_segment3::closest_point(const point3& point) const
{
    vector3 w = vector3(point - origin);
    float proj = w.dot(direction);
    // endpoint 0 is closest point
    if (proj <= 0.0f)
        return origin;
    else
    {
        float vsq = direction.dot(direction);
        // endpoint 1 is closest point
        if (proj >= vsq)
            return origin + direction;
        // else somewhere else in segment
        else
            return origin + (proj/vsq)*direction;
    }
}


