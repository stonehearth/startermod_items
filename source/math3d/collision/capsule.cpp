#include "radiant.h"
#include "math3d_collision.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ capsule.cpp
// 
// Capsule collision class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
// @ capsule::capsule()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
capsule::capsule(const capsule& other) : 
    _segment(other._segment),
    _radius(other._radius)
{

}   // End of capsule::capsule()


//-------------------------------------------------------------------------------
// @ capsule::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
capsule&
capsule::operator=(const capsule& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    _segment = other._segment;
    _radius = other._radius;
    
    return *this;

}   // End of capsule::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const capsule& source)
{
    out << source.get_segment();
    out << ' ' << source.get_radius();

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ capsule::length()
//-------------------------------------------------------------------------------
// Vector length
//-------------------------------------------------------------------------------
float 
capsule::length() const
{
    return _segment.length() + 2.0f*_radius;

}   // End of capsule::length()


//-------------------------------------------------------------------------------
// @ capsule::length_squared()
//-------------------------------------------------------------------------------
// Capsule length squared (avoids square root)
//-------------------------------------------------------------------------------
float 
capsule::length_squared() const
{
    return _segment.length_squared();

}   // End of capsule::length_squared()

//-------------------------------------------------------------------------------
// @ capsule::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
capsule::operator==(const capsule& other) const
{
    if (other._segment == _segment && other._radius == _radius)
        return true;

    return false;   
}   // End of capsule::operator==()


//-------------------------------------------------------------------------------
// @ capsule::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
capsule::operator!=(const capsule& other) const
{
    if (other._segment == _segment && other._radius == _radius)
        return false;

    return true;
}   // End of capsule::operator!=()


//-------------------------------------------------------------------------------
// @ capsule::set()
//-------------------------------------------------------------------------------
// set capsule based on set of points
//-------------------------------------------------------------------------------
void
capsule::set(const point3* points, unsigned int n_points)
{
    ASSERT(points);
    point3 centroid;

    // compute covariance matrix
    matrix3 C;
    compute_covariance_matrix(C, centroid, points, n_points);

    // get main axis
    vector3 w, u, v;
    get_real_systemmetric_eigenvectors(w, u, v, C);

    float max_dist_sq = 0.0f;

    // for each point do
    unsigned int i;
    for (i = 0; i < n_points; ++i)
    {
        // compute _radius
        vector3 diff = vector3(points[i]-centroid);
        float proj = diff.dot(w);

        float dist_sq = diff.dot(diff) - proj*proj;
        if (dist_sq > max_dist_sq)
          max_dist_sq = dist_sq;
    }

    _radius = ::math3d::sqrt(max_dist_sq);

    // now set endcaps
    // for each point do
    float t0 = FLT_MAX;
    float t1 = FLT_MIN;
    for (i = 0; i < n_points; ++i)
    {
        vector3 localTrans = vector3(points[i]-centroid);
        float u_coord = localTrans.dot(u);
        float v_coord = localTrans.dot(v);
        float w_coord = localTrans.dot(w);

        float radical = max_dist_sq - u_coord*u_coord - v_coord*v_coord;
        if (radical > k_epsilon)
            radical = ::math3d::sqrt(radical);
        else
            radical = 0.0f;

        float test = w_coord + radical;
        if (test < t0)
            t0 = test;

        test = w_coord - radical;
        if (test > t1)
            t1 = test;
    }

    // set up _segment
    if (t0 < t1)
    {
        _segment.set(centroid + t0*w, centroid + t1*w);
    }
    else
    {
        // is sphere
        point3 center = centroid + 0.5f*(t0+t1)*w;
        _segment.set(center, center);
    }

}   // End of capsule::set()


//----------------------------------------------------------------------------
// @ capsule::transform()
// ---------------------------------------------------------------------------
// Transforms _segment into new space
//-----------------------------------------------------------------------------
capsule   
capsule::transform(float scale, const quaternion& rotate, const vector3& translate) const
{
    return capsule(_segment.transform(scale, rotate, translate), _radius*scale);

}   // End of capsule::transform()


//----------------------------------------------------------------------------
// @ capsule::transform()
// ---------------------------------------------------------------------------
// Transforms _segment into new space
//-----------------------------------------------------------------------------
capsule   
capsule::transform(float scale, const matrix3& rotate,  const vector3& translate) const
{
    return capsule(_segment.transform(scale, rotate, translate), _radius*scale);

}   // End of capsule::transform()


//----------------------------------------------------------------------------
// @ capsule::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between capsule and capsule
//-----------------------------------------------------------------------------
bool 
capsule::intersect(const capsule& other) const
{
    float radius_sum = _radius + other._radius;

    // if colliding
    float s, t;
    float distance_sq = math3d::distance_squared(_segment, other._segment, s, t); 

    return (distance_sq <= radius_sum*radius_sum);
}


//----------------------------------------------------------------------------
// @ capsule::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between capsule and line
//-----------------------------------------------------------------------------
bool
capsule::intersect(const line3& line) const
{
    // test distance between line and _segment vs. _radius
    float s_c, t_c;
    return (math3d::distance_squared(_segment, line, s_c, t_c) <= _radius*_radius);
}


//----------------------------------------------------------------------------
// @ capsule::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between capsule and ray
//-----------------------------------------------------------------------------
bool
capsule::intersect(const ray3& ray) const
{
    // test distance between ray and _segment vs. _radius
    float s_c, t_c;
    return (math3d::distance_squared(_segment, ray, s_c, t_c) <= _radius*_radius);

}


//----------------------------------------------------------------------------
// @ capsule::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between capsule and line _segment
//-----------------------------------------------------------------------------
bool
capsule::intersect(const line_segment3& _segment) const
{
    // test distance between _segment and _segment vs. _radius
    float s_c, t_c;
    return (math3d::distance_squared(_segment, _segment, s_c, t_c) <= _radius*_radius);

}


//----------------------------------------------------------------------------
// @ capsule::classify()
// ---------------------------------------------------------------------------
// Return signed distance between capsule and plane
//-----------------------------------------------------------------------------
float
capsule::classify(const plane& plane) const
{
    float s0 = plane.test(_segment.get_endpoint_0());
    float s1 = plane.test(_segment.get_endpoint_1());

    // points on opposite sides or intersecting plane
    if (s0*s1 <= 0.0f)
        return 0.0f;

    // intersect if either endpoint is within _radius distance of plane
    if(math3d::abs(s0) <= _radius || math3d::abs(s1) <= _radius)
        return 0.0f;

    // return signed distance
    return (math3d::abs(s0) < math3d::abs(s1) ? s0 : s1);
}


//----------------------------------------------------------------------------
// @ ::merge()
// ---------------------------------------------------------------------------
// merge two capsules together to create a new one
// See _3D Game Engine Design_ (Eberly) for more details
//-----------------------------------------------------------------------------
void
math3d::merge(capsule& result, const capsule& b0, const capsule& b1)
{
    // 1) get line through _center

    // origin is average of centers
    point3 origin = 0.5f*(b0.get_segment().get_center() + b1.get_segment().get_center());
    
    // direction is average of directions
    vector3 direction0 = b0.get_segment().direction;
    direction0.normalize();
    vector3 direction1 = b1.get_segment().direction;
    direction1.normalize();
    if (direction0.dot(direction1) < 0.0f)
        direction1 = -direction1;
    vector3 direction = direction0 + direction1;

    // create line
    line3 line(origin, direction);

    // 2) compute _radius

    // _radius will be the maximum of the distance to each endpoint, 
    // plus the corresponding capsule _radius
    float t;
    float radius = math3d::distance(line, b0.get_segment().get_endpoint_0(), t) + b0.get_radius();
    float tempRadius = math3d::distance(line, b0.get_segment().get_endpoint_1(), t) + b0.get_radius();
    if (tempRadius > radius)
        radius = tempRadius;
    tempRadius = math3d::distance(line, b1.get_segment().get_endpoint_0(), t) + b1.get_radius();
    if (tempRadius > radius)
        radius = tempRadius;
    tempRadius = math3d::distance(line, b1.get_segment().get_endpoint_1(), t) + b1.get_radius();
    if (tempRadius > radius)
        radius = tempRadius;

    // 3) compute parameters of endpoints on line

    // we compute a sphere at each original endpoint, and set the endcaps of the
    // new capsule around them, minimizing size

    // capsule 0, endpoint 0
    float radiusDiffSq = radius - b0.get_radius();
    radiusDiffSq *= radiusDiffSq;
    vector3 origin_diff = vector3(line.origin - b0.get_segment().get_endpoint_0());
    // compute coefficients for quadratic
    float halfb = line.direction.dot(origin_diff);
    float c = origin_diff.dot(origin_diff) - radiusDiffSq;
    // this is sqrt(b^2 - 4ac)/2, but a lot of terms have cancelled out
    float radical = halfb*halfb - c;
    if (radical > k_epsilon)
        radical = ::math3d::sqrt(radical);
    else
        radical = 0.0f;
    // and this is adding -b/2 +/- the above
    float t0 = -halfb - radical;
    float t1 = -halfb + radical;

    // capsule 0, endpoint 1
    origin_diff = vector3(line.origin - b0.get_segment().get_endpoint_1());
    halfb = line.direction.dot(origin_diff);
    c = origin_diff.dot(origin_diff) - radiusDiffSq;
    radical = halfb*halfb - c;
    if (radical > k_epsilon)
        radical = ::math3d::sqrt(radical);
    else
        radical = 0.0f;
    float param = -halfb - radical;
    if (param < t0)
        t0 = param;
    param = -halfb + radical;
    if (param > t1)
        t1 = param;

    // capsule 1, endpoint 0
    radiusDiffSq = radius - b1.get_radius();
    radiusDiffSq *= radiusDiffSq;
    origin_diff = vector3(line.origin - b1.get_segment().get_endpoint_0());
    halfb = line.direction.dot(origin_diff);
    c = origin_diff.dot(origin_diff) - radiusDiffSq;
    radical = halfb*halfb - c;
    if (radical > k_epsilon)
        radical = ::math3d::sqrt(radical);
    else
        radical = 0.0f;
    param = -halfb - radical;
    if (param < t0)
        t0 = param;
    param = -halfb + radical;
    if (param > t1)
        t1 = param;

     // capsule 1, endpoint 1
    origin_diff = vector3(line.origin - b1.get_segment().get_endpoint_1());
    halfb = line.direction.dot(origin_diff);
    c = origin_diff.dot(origin_diff) - radiusDiffSq;
    radical = halfb*halfb - c;
    if (radical > k_epsilon)
        radical = ::math3d::sqrt(radical);
    else
        radical = 0.0f;
    param = -halfb - radical;
    if (param < t0)
        t0 = param;
    param = -halfb + radical;
    if (param > t1)
        t1 = param;

    // set capsule
    result.set_radius(radius);
    if (t0 < t1)
    {
        result.set_segment(line.origin + t0*line.direction, 
                           line.origin + t1*line.direction);
    }
    else
    {
        // is sphere
        point3 center = line.origin + 0.5f*(t0+t1)*line.direction;
        result.set_segment(center, center);
    }

}   // End of ::merge()



//----------------------------------------------------------------------------
// @ capsule::compute_collision()
// ---------------------------------------------------------------------------
// Compute parameters for collision between capsule and capsule
//-----------------------------------------------------------------------------
bool 
capsule::compute_collision(const capsule& other, vector3& collision_normal, 
                           point3& collision_point, float& penetration) const
{
    float radius_sum = _radius + other._radius;

    // if colliding
    float s, t;
    float distance_sq = math3d::distance_squared(_segment, other._segment, s, t); 

    if (distance_sq <= radius_sum*radius_sum)
    {
        // calculate our values
        point3 nearPoint0 = (1.0f-s)*_segment.get_endpoint_0() + s*_segment.get_endpoint_1();
        point3 nearPoint1 = (1.0f-t)*other._segment.get_endpoint_0() + t*other._segment.get_endpoint_1();

        collision_normal = vector3(nearPoint1 - nearPoint0);

        // penetration is distance - radii
        float distance = ::math3d::sqrt(distance_sq);
        penetration = radius_sum - distance;
        collision_normal.normalize();

        // collision point is average of penetration
        // weighted towards lighter object
        float t = 0.5f;//mMass/(mMass + other->mMass);
        collision_point = (1.0f-t)*(nearPoint0 + _radius*collision_normal)
                        + t*(nearPoint1 - other._radius*collision_normal);

        return true;
    }

    return false;
}


