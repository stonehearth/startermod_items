#include "radiant.h"
#include "math3d_collision.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ obb.cpp
// 
// Oriented bounding box class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "covariance.h"
#include "obb.h"

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ obb::obb()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
obb::obb(const obb& other) : 
    _center(other._center),
    _rotation(other._rotation),
    _extents(other._extents)
{

}   // End of obb::obb()


//-------------------------------------------------------------------------------
// @ obb::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
obb&
obb::operator=(const obb& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    _center = other._center;
    _rotation = other._rotation;
    _extents = other._extents;
    
    return *this;

}   // End of obb::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const obb& source)
{
    out << source.get_center();
    out << ' ' << source.get_extents();
    out << ' ' << source.get_rotation();

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ obb::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
obb::operator==(const obb& other) const
{
    if (other._center == _center && other._rotation == _rotation
         && other._extents == _extents)
        return true;

    return false;   
}   // End of obb::operator==()


//-------------------------------------------------------------------------------
// @ obb::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
obb::operator!=(const obb& other) const
{
    if (other._center != _center || other._rotation != _rotation
         || other._extents != _extents)
        return true;

    return false;

}   // End of obb::operator!=()


//-------------------------------------------------------------------------------
// @ obb::set()
//-------------------------------------------------------------------------------
// set OBB based on set of points
//-------------------------------------------------------------------------------
void
obb::set(const point3* points, unsigned int n_points)
{
    ASSERT(points);

    point3 centroid;

    // compute covariance matrix
    matrix3 C;
    compute_covariance_matrix(C, centroid, points, n_points);

    // get basis vectors
    vector3 basis[3];
    get_real_systemmetric_eigenvectors(basis[0], basis[1], basis[2], C);
    _rotation.set_columns(basis[0], basis[1], basis[2]);

    vector3 min_p(FLT_MAX, FLT_MAX, FLT_MAX);
    vector3 max_p(FLT_MIN, FLT_MIN, FLT_MIN);

    // compute min_p, max_p projections of box on axes
    // for each point do 
    unsigned int i;
    for (i = 0; i < n_points; ++i)
    {
        vector3 diff = vector3(points[i] - centroid);
        for (int j = 0; j < 3; ++j)
        {
            float length = diff.dot(basis[j]);
            if (length > max_p[j])
            {
                max_p[j] = length;
            }
            else if (length < min_p[j])
            {
                min_p[j] = length;
            }
        }
    }

    // compute _center, _extents
    _center = centroid;
    for (i = 0; i < 3; ++i)
    {
        _center += 0.5f*(min_p[i]+max_p[i])*basis[i];
        _extents[i] = 0.5f*(max_p[i]-min_p[i]);
    }

}   // End of obb::set()


//----------------------------------------------------------------------------
// @ obb::transform()
// ---------------------------------------------------------------------------
// Transforms OBB into new space
//-----------------------------------------------------------------------------
obb    
obb::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    obb result;
    matrix3 rotation_matrix(rotate);

    result.set_extents(scale * _extents);
    result.set_center(rotation_matrix * point3(_center + translate));
    result.set_rotation(rotation_matrix * _rotation);

    return result;

}   // End of obb::transform()


//----------------------------------------------------------------------------
// @ obb::transform()
// ---------------------------------------------------------------------------
// Transforms OBB into new space
//-----------------------------------------------------------------------------
obb    
obb::transform(float scale, const matrix3& rotate,  const vector3& translate) const
{
    obb result;

    result.set_extents(scale*_extents);
    result.set_center(rotate*_center + translate);
    result.set_rotation(rotate*_rotation);

    return result;
    
}   // End of obb::transform()


//----------------------------------------------------------------------------
// @ obb::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between OBB and OBB
//
// The efficiency of this could be improved slightly by computing factors only
// as we need them.  For example, we could compute only the first row of R
// before the first axis test, then the second row, etc.  It has been left this
// way because it's clearer.
//-----------------------------------------------------------------------------
bool 
obb::intersect(const obb& other) const
{
    // extent vectors
    const vector3& a = _extents;    
    const vector3& b = other._extents;

    // test factors
    float c_test, a_test, b_test;
    bool parallel_axes = false;

    // transpose of _rotation of B relative to A, i.e. (R_b^T * R_a)^T
    matrix3 r_t = math3d::transpose(_rotation)*other._rotation;

    // absolute value of relative _rotation matrix
    matrix3 r_abs;  
    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = 0; j < 3; ++j)
        {
            r_abs(i,j) = math3d::abs(r_t(i,j));
            // if magnitude of dot product between axes is close to one
            if (r_abs(i,j) + k_epsilon >= 1.0f)
            {
                // then box A and box B have near-parallel axes
                parallel_axes = true;
            }
        }
    }
    
    // relative translation (in A's frame)
    vector3 c = vector3(other._center - _center)*_rotation;

    // separating axis A0
    c_test = math3d::abs(c.x);
    a_test = a.x;
    b_test = b.x*r_abs(0,0)+b.y*r_abs(0,1)+b.z*r_abs(0,2);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A1
    c_test = math3d::abs(c.y);
    a_test = a.y;
    b_test = b.x*r_abs(1,0)+b.y*r_abs(1,1)+b.z*r_abs(1,2);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A2
    c_test = math3d::abs(c.z);
    a_test = a.z;
    b_test = b.x*r_abs(2,0)+b.y*r_abs(2,1)+b.z*r_abs(2,2);
    if (c_test > a_test + b_test)
        return false;

    // separating axis B0
    c_test = math3d::abs(c.x*r_t(0,0) + c.y*r_t(1,0) + c.z*r_t(2,0));
    a_test = a.x*r_abs(0,0)+a.y*r_abs(1,0)+a.z*r_abs(2,0);
    b_test = b.x;
    if (c_test > a_test + b_test)
        return false;

    // separating axis B1
    c_test = math3d::abs(c.x*r_t(0,1) + c.y*r_t(1,1) + c.z*r_t(2,1));
    a_test = a.x*r_abs(0,1)+a.y*r_abs(1,1)+a.z*r_abs(2,1);
    b_test = b.y;
    if (c_test > a_test + b_test)
        return false;

    // separating axis B2
    c_test = math3d::abs(c.x*r_t(0,2) + c.y*r_t(1,2) + c.z*r_t(2,2));
    a_test = a.x*r_abs(0,2)+a.y*r_abs(1,2)+a.z*r_abs(2,2);
    b_test = b.z;
    if (c_test > a_test + b_test)
        return false;

    // if the two boxes have parallel axes, we're done, intersection
    if (parallel_axes)
        return true;

    // separating axis A0 x B0
    c_test = math3d::abs(c.z*r_t(1,0)-c.y*r_t(2,0));
    a_test = a.y*r_abs(2,0) + a.z*r_abs(1,0);
    b_test = b.y*r_abs(0,2) + b.z*r_abs(0,1);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A0 x B1
    c_test = math3d::abs(c.z*r_t(1,1)-c.y*r_t(2,1));
    a_test = a.y*r_abs(2,1) + a.z*r_abs(1,1);
    b_test = b.x*r_abs(0,2) + b.z*r_abs(0,0);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A0 x B2
    c_test = math3d::abs(c.z*r_t(1,2)-c.y*r_t(2,2));
    a_test = a.y*r_abs(2,2) + a.z*r_abs(1,2);
    b_test = b.x*r_abs(0,1) + b.y*r_abs(0,0);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A1 x B0
    c_test = math3d::abs(c.x*r_t(2,0)-c.z*r_t(0,0));
    a_test = a.x*r_abs(2,0) + a.z*r_abs(0,0);
    b_test = b.y*r_abs(1,2) + b.z*r_abs(1,1);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A1 x B1
    c_test = math3d::abs(c.x*r_t(2,1)-c.z*r_t(0,1));
    a_test = a.x*r_abs(2,1) + a.z*r_abs(0,1);
    b_test = b.x*r_abs(1,2) + b.z*r_abs(1,0);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A1 x B2
    c_test = math3d::abs(c.x*r_t(2,2)-c.z*r_t(0,2));
    a_test = a.x*r_abs(2,2) + a.z*r_abs(0,2);
    b_test = b.x*r_abs(1,1) + b.y*r_abs(1,0);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A2 x B0
    c_test = math3d::abs(c.y*r_t(0,0)-c.x*r_t(1,0));
    a_test = a.x*r_abs(1,0) + a.y*r_abs(0,0);
    b_test = b.y*r_abs(2,2) + b.z*r_abs(2,1);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A2 x B1
    c_test = math3d::abs(c.y*r_t(0,1)-c.x*r_t(1,1));
    a_test = a.x*r_abs(1,1) + a.y*r_abs(0,1);
    b_test = b.x*r_abs(2,2) + b.z*r_abs(2,0);
    if (c_test > a_test + b_test)
        return false;

    // separating axis A2 x B2
    c_test = math3d::abs(c.y*r_t(0,2)-c.x*r_t(1,2));
    a_test = a.x*r_abs(1,2) + a.y*r_abs(0,2);
    b_test = b.x*r_abs(2,1) + b.y*r_abs(2,0);
    if (c_test > a_test + b_test)
        return false;

    // all tests failed, have intersection
    return true;
}


//----------------------------------------------------------------------------
// @ obb::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between OBB and line
//-----------------------------------------------------------------------------
bool
obb::intersect(const line3& line) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // compute difference vector
    vector3 diff = vector3(_center - line.origin);

    // for each axis do
    for (int i = 0; i < 3; ++i)
    {
        // get axis i
        vector3 axis = _rotation.get_column(i);

        // project relative vector onto axis
        float e = axis.dot(diff);
        float f = line.direction.dot(axis);

        // ray is parallel to plane
        if (math3d::is_zero(f))
        {
            // ray passes by box
            if (-e - _extents[i] > 0.0f || -e + _extents[i] > 0.0f)
                return false;
            continue;
        }

        float s = (e - _extents[i])/f;
        float t = (e + _extents[i])/f;

        // fix order
        if (s > t)
        {
            float temp = s;
            s = t;
            t = temp;
        }

        // adjust min and max values
        if (s > max_s)
            max_s = s;
        if (t < min_t)
            min_t = t;

        // check for intersection failure
        if (max_s > min_t)
            return false;
    }

    // done, have intersection
    return true;
}


//----------------------------------------------------------------------------
// @ obb::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between OBB and ray
//-----------------------------------------------------------------------------
bool
obb::intersect(const ray3& ray) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // compute difference vector
    vector3 diff = vector3(_center - ray.origin);

    // for each axis do
    for (int i = 0; i < 3; ++i)
    {
        // get axis i
        vector3 axis = _rotation.get_column(i);

        // project relative vector onto axis
        float e = axis.dot(diff);
        float f = ray.direction.dot(axis);

        // ray is parallel to plane
        if (math3d::is_zero(f))
        {
            // ray passes by box
            if (-e - _extents[i] > 0.0f || -e + _extents[i] > 0.0f)
                return false;
            continue;
        }

        float s = (e - _extents[i])/f;
        float t = (e + _extents[i])/f;

        // fix order
        if (s > t)
        {
            float temp = s;
            s = t;
            t = temp;
        }

        // adjust min and max values
        if (s > max_s)
            max_s = s;
        if (t < min_t)
            min_t = t;

        // check for intersection failure
        if (min_t < 0.0f || max_s > min_t)
            return false;
    }

    // done, have intersection
    return true;
}


//----------------------------------------------------------------------------
// @ obb::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between OBB and line segment
//-----------------------------------------------------------------------------
bool
obb::intersect(const line_segment3& segment) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // compute difference vector
    vector3 diff = vector3(_center - segment.origin);

    // for each axis do
    for (int i = 0; i < 3; ++i)
    {
        // get axis i
        vector3 axis = _rotation.get_column(i);

        // project relative vector onto axis
        float e = axis.dot(diff);
        float f = segment.direction.dot(axis);

        // ray is parallel to plane
        if (math3d::is_zero(f))
        {
            // ray passes by box
            if (-e - _extents[i] > 0.0f || -e + _extents[i] > 0.0f)
                return false;
            continue;
        }

        float s = (e - _extents[i])/f;
        float t = (e + _extents[i])/f;

        // fix order
        if (s > t)
        {
            float temp = s;
            s = t;
            t = temp;
        }

        // adjust min and max values
        if (s > max_s)
            max_s = s;
        if (t < min_t)
            min_t = t;

        // check for intersection failure
        if (min_t < 0.0f || max_s > 1.0f || max_s > min_t)
            return false;
    }

    // done, have intersection
    return true;

}   // End of obb::intersect()


//----------------------------------------------------------------------------
// @ obb::classify()
// ---------------------------------------------------------------------------
// Return signed distance between OBB and plane
//-----------------------------------------------------------------------------
float obb::classify(const plane& plane) const
{
    vector3 x_normal = plane.normal*_rotation;
    // maximum extent in direction of plane normal 
    float r = math3d::abs(_extents.x*x_normal.x) 
            + math3d::abs(_extents.y*x_normal.y) 
            + math3d::abs(_extents.z*x_normal.z);
    // signed distance between box _center and plane
    float d = plane.test(_center);

    // return signed distance
    if (math3d::abs(d) < r)
        return 0.0f;
    else if (d < 0.0f)
        return d + r;
    else
        return d - r;

}   // End of obb::classify()


//----------------------------------------------------------------------------
// @ ::merge()
// ---------------------------------------------------------------------------
// merge two OBBs together to create a new one
//-----------------------------------------------------------------------------
void
math3d::merge(obb& result, const obb& b0, const obb& b1)
{
    // new _center is average of original centers
    point3 new_center = (b0.get_center() + b1.get_center())*0.5f;

    // new axes are sum of rotations (as quaternions)
    quaternion q0(b0.get_rotation());
    quaternion q1(b1.get_rotation());
    quaternion q;
    // this shouldn't happen, but just in case
    if (q0.dot(q1) < 0.0f)
        q = q0-q1;
    else
        q = q0+q1;
    q.normalize();
    matrix3 new_rotation(q);

    // new _extents are projections of old _extents on new axes
    // for each axis do
    vector3 new_extents;
    for (int i = 0; i < 3; ++i)
    {
        // get axis i
        vector3 axis = new_rotation.get_column(i);

        // get difference between this box _center and other box _center
        vector3 center_diff = vector3(b0.get_center() - new_center);

        // rotate into box 0's space
        vector3 xAxis = axis*b0.get_rotation();
        // maximum extent in direction of axis
        new_extents[i] = math3d::abs(center_diff.x*axis.x) 
                      + math3d::abs(center_diff.y*axis.y)  
                      + math3d::abs(center_diff.z*axis.z) 
                      + math3d::abs(xAxis.x*b0.get_extents().x) 
                      + math3d::abs(xAxis.y*b0.get_extents().y) 
                      + math3d::abs(xAxis.z*b0.get_extents().z);

        // get difference between this box _center and other box _center
        center_diff = vector3(b1.get_center() - new_center);

        // rotate into box 1's space
        xAxis = axis*b1.get_rotation();
        // maximum extent in direction of axis
        float maxExtent = math3d::abs(center_diff.x*axis.x) 
                      + math3d::abs(center_diff.y*axis.y)  
                      + math3d::abs(center_diff.z*axis.z) 
                      + math3d::abs(xAxis.x*b1.get_extents().x) 
                      + math3d::abs(xAxis.y*b1.get_extents().y) 
                      + math3d::abs(xAxis.z*b1.get_extents().z);
        // if greater than box0's result, use it
        if (maxExtent > new_extents[i])
            new_extents[i] = maxExtent;
    }

    // copy at end, in case user is passing in b0 or b1 as result
    result.set_center(new_center);
    result.set_rotation(new_rotation);
    result.set_extents(new_extents);

}   // End of ::merge()
