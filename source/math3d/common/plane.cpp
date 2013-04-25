#include "radiant.h"
#include "common.h"
#include "radiant.pb.h"

using namespace radiant;
using namespace radiant::math3d;

//----------------------------------------------------------------------------
// @ plane.cpp
// 
// 3D plane class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//----------------------------------------------------------------------------
//-- Includes ----------------------------------------------------------------
//----------------------------------------------------------------------------

#include "plane.h"
#include "line3.h"
#include "matrix3.h"
#include "quaternion.h"
#include "vector3.h"
#include "ray3.h"

//----------------------------------------------------------------------------
//-- Static Variables --------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//-- Functions ---------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// @ plane::plane()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
plane::plane() :
    normal(1.0f, 0.0f, 0.0f),
    offset(0.0f)
{
}   // End of plane::plane()


//----------------------------------------------------------------------------
// @ plane::plane()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
plane::plane(float a, float b, float c, float d)
{
    set(a, b, c, d);

}   // End of plane::plane()


//----------------------------------------------------------------------------
// @ plane::plane()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
plane::plane(const vector3& p0, const vector3& p1, const vector3& p2)
{
    set(p0, p1, p2);

}   // End of plane::plane()


//----------------------------------------------------------------------------
// @ plane::plane()
// ---------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------
plane::plane(const plane& other) :
    normal(other.normal),
    offset(other.offset)
{

}   // End of plane::plane()


//----------------------------------------------------------------------------
// @ plane::operator=()
// ---------------------------------------------------------------------------
// Assigment operator
//-----------------------------------------------------------------------------
plane&
plane::operator=(const plane& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    normal = other.normal;
    offset = other.offset;

    return *this;

}   // End of plane::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Output a text plane.  The format is assumed to be :
// [ vector3, vector3 ]
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const plane& source)
{
    return out << "[" << source.normal 
                     << ", " << source.offset << "]";
    
}   // End of operator<<()
    


//----------------------------------------------------------------------------
// @ plane::operator==()
// ---------------------------------------------------------------------------
// Are two plane's equal?
//----------------------------------------------------------------------------
bool
plane::operator==(const plane& plane) const
{
    return (plane.normal == normal && plane.offset == offset);

}  // End of plane::operator==()


//----------------------------------------------------------------------------
// @ plane::operator!=()
// ---------------------------------------------------------------------------
// Are two plane's not equal?
//----------------------------------------------------------------------------
bool
plane::operator!=(const plane& plane) const
{
    return !(plane.normal == normal && plane.offset == offset);
}  // End of plane::operator!=()


//----------------------------------------------------------------------------
// @ plane::set()
// ---------------------------------------------------------------------------
// Sets the parameters
//-----------------------------------------------------------------------------
void
plane::set(float a, float b, float c, float d)
{
    // normalize for cheap distance checks
    float lensq = a*a + b*b + c*c;
    // length of normal had better not be zero
    ASSERT(!math3d::is_zero(lensq));

    // recover gracefully
    if (math3d::is_zero(lensq))
    {
        normal = vector3::unit_x;
        offset = 0.0f;
    }
    else
    {
        float recip = math3d::inv_sqrt(lensq);
        normal.set(a*recip, b*recip, c*recip);
        offset = d*recip;
    }

}   // End of plane::set()


//----------------------------------------------------------------------------
// @ plane::set()
// ---------------------------------------------------------------------------
// Sets the parameters
//-----------------------------------------------------------------------------
void 
plane::set(const vector3& p0, const vector3& p1, const vector3& p2)
{
    // get plane vectors
    vector3 u = p1 - p0;
    vector3 v = p2 - p0;
    vector3 w = u.cross(v);
    
    // normalize for cheap distance checks
    float lensq = w.x*w.x + w.y*w.y + w.z*w.z;
    // length of normal had better not be zero
    ASSERT(!math3d::is_zero(lensq));

    // recover gracefully
    if (math3d::is_zero(lensq))
    {
        normal = vector3::unit_x;
        offset = 0.0f;
    }
    else
    {
        float recip = 1.0f/lensq;
        normal.set(w.x*recip, w.y*recip, w.z*recip);
        offset = -normal.dot(p0);
    }

}   // End of plane::set()


//----------------------------------------------------------------------------
// @ plane::transform()
// ---------------------------------------------------------------------------
// Transforms plane into new space
//-----------------------------------------------------------------------------
plane 
plane::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    plane plane;

    // get rotation matrix
    matrix3    rotmatrix(rotate);

    // transform to get normal
    plane.normal = rotmatrix*normal/scale;
    
    // transform to get offset
    vector3 newTrans = translate*rotmatrix;
    plane.offset = -newTrans.dot(normal)/scale + offset;

    return plane;

}   // End of plane::transform()


//----------------------------------------------------------------------------
// @ plane::closest_point()
// ---------------------------------------------------------------------------
// Returns the closest point on plane to point
//-----------------------------------------------------------------------------
point3 
plane::closest_point(const point3& point) const
{    
    return point - test(point)*normal; 

}   // End of plane::closest_point()


bool plane::intersects(const ray3 &ray, float &t) const
{
   // See http://www.cs.toronto.edu/~smalik/418/tutorial8_ray_primitive_intersections.pdf

   float Vd = normal.dot(ray.direction);
   if (Vd == 0) {
      // parallel to the plane
      return false;
   }
   if (Vd > 0) {
      // normal pointing away from the ray.  ignore backface intersections.
      return false;
   }
   float V0 = -(normal.dot(vector3(ray.origin)) + offset);
   
   float intersect_time = V0 / Vd;
   if (intersect_time < 0) {
      // intersection behind the origin
      return false;
   }
   t = intersect_time;
   return true;
}

