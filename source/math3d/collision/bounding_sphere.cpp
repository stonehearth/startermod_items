#include "radiant.h"
#include "math3d_collision.h"

using namespace radiant;
using namespace radiant::math3d;


//===============================================================================
// @ bounding_sphere.cpp
// 
// sphere collision class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
// @ bounding_sphere::bounding_sphere()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
bounding_sphere::bounding_sphere(const bounding_sphere& other) : 
    _center(other._center),
    _radius(other._radius)
{

}   // End of bounding_sphere::bounding_sphere()


//-------------------------------------------------------------------------------
// @ bounding_sphere::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
bounding_sphere&
bounding_sphere::operator=(const bounding_sphere& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    _center = other._center;
    _radius = other._radius;
    
    return *this;

}   // End of bounding_sphere::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const bounding_sphere& source)
{
    out << source.get_center();
    out << ' ' << source.get_radius();

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ bounding_sphere::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
bounding_sphere::operator==(const bounding_sphere& other) const
{
    if (other._center == _center && other._radius == _radius)
        return true;

    return false;   
}   // End of bounding_sphere::operator==()


//-------------------------------------------------------------------------------
// @ bounding_sphere::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
bounding_sphere::operator!=(const bounding_sphere& other) const
{
    if (other._center != _center || other._radius != _radius)
        return true;

    return false;

}   // End of bounding_sphere::operator!=()


//-------------------------------------------------------------------------------
// @ bounding_sphere::set()
//-------------------------------------------------------------------------------
// set bounding sphere based on set of points
//-------------------------------------------------------------------------------
void
bounding_sphere::set(const point3* points, unsigned int num_points)
{
    ASSERT(points);
    // compute minimal and maximal bounds
    point3 min_p(points[0]), max_p(points[0]);
    unsigned int i;

    for (i = 1; i < num_points; ++i) {
        if (points[i].x < min_p.x)
            min_p.x = points[i].x;
        else if (points[i].x > max_p.x)
            max_p.x = points[i].x;
        if (points[i].y < min_p.y)
            min_p.y = points[i].y;
        else if (points[i].y > max_p.y)
            max_p.y = points[i].y;
        if (points[i].z < min_p.z)
            min_p.z = points[i].z;
        else if (points[i].z > max_p.z)
            max_p.z = points[i].z;
    }
    // compute _center and _radius
    _center = 0.5f * (min_p + max_p);
    float maxDistance = math3d::distance_squared(_center, points[0]);
    for (i = 1; i < num_points; ++i)
    {
        float dist = math3d::distance_squared(_center, points[i]);
        if (dist > maxDistance)
            maxDistance = dist;
    }
    _radius = ::math3d::sqrt(maxDistance);
}


//----------------------------------------------------------------------------
// @ bounding_sphere::transform()
// ---------------------------------------------------------------------------
// Transforms sphere into new space
//-----------------------------------------------------------------------------
bounding_sphere    
bounding_sphere::transform(float scale, const quaternion& rotate, 
    const vector3& translate) const
{
    return bounding_sphere(rotate.rotate(_center) + translate, _radius*scale);

}   // End of bounding_sphere::transform()


//----------------------------------------------------------------------------
// @ bounding_sphere::transform()
// ---------------------------------------------------------------------------
// Transforms sphere into new space
//-----------------------------------------------------------------------------
bounding_sphere    
bounding_sphere::transform(float scale, const matrix3& rotate, 
    const vector3& translate) const
{
    return bounding_sphere(rotate*_center + translate, _radius*scale);

}   // End of bounding_sphere::transform()


//----------------------------------------------------------------------------
// @ bounding_sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and sphere
//-----------------------------------------------------------------------------
bool 
bounding_sphere::intersect(const bounding_sphere& other) const
{
    // do sphere check
    float radius_sum = _radius + other._radius;
    vector3 center_diff = vector3(other._center - _center); 
    float distance_sq = center_diff.length_squared();

    // if distance squared < sum of radii squared, collision!
    return (distance_sq <= radius_sum*radius_sum);
}


//----------------------------------------------------------------------------
// @ bounding_sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and line
//-----------------------------------------------------------------------------
bool
bounding_sphere::intersect(const line3& line) const
{
    // compute intermediate values
    vector3 w = vector3(_center - line.origin);
    float wsq = w.dot(w);
    float proj = w.dot(line.direction);
    float rsq = _radius*_radius;
    float vsq = line.direction.dot(line.direction);

    // test length of difference vs. _radius
    return (vsq*wsq - proj*proj <= vsq*rsq);
}


//----------------------------------------------------------------------------
// @ bounding_sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and ray
//-----------------------------------------------------------------------------
bool
bounding_sphere::intersect(const ray3& ray) const
{
    // compute intermediate values
    vector3 w = vector3(_center - ray.origin);
    float wsq = w.dot(w);
    float proj = w.dot(ray.direction);
    float rsq = _radius*_radius;

    // if sphere behind ray, no intersection
    if (proj < 0.0f && wsq > rsq)
        return false;
    float vsq = ray.direction.dot(ray.direction);

    // test length of difference vs. _radius
    return (vsq*wsq - proj*proj <= vsq*rsq);
}


//----------------------------------------------------------------------------
// @ bounding_sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and line _segment
//-----------------------------------------------------------------------------
bool
bounding_sphere::intersect(const line_segment3& _segment) const
{
    // compute intermediate values
    vector3 w = vector3(_center - _segment.origin);
    float wsq = w.dot(w);
    float proj = w.dot(_segment.direction);
    float rsq = _radius*_radius;

    // if sphere outside _segment, no intersection
    if ((proj < 0.0f || proj > 1.0f) && wsq > rsq)
        return false;
    float vsq = _segment.direction.dot(_segment.direction);

    // test length of difference vs. _radius
    return (vsq*wsq - proj*proj <= vsq*rsq);
}


//----------------------------------------------------------------------------
// @ bounding_sphere::classify()
// ---------------------------------------------------------------------------
// Compute signed distance between sphere and plane
//-----------------------------------------------------------------------------
float
bounding_sphere::classify(const plane& plane) const
{
    float distance = plane.test(_center);
    if (distance > _radius)
    {
        return distance-_radius;
    }
    else if (distance < -_radius)
    {
        return distance+_radius;
    }
    else
    {
        return 0.0f;
    }

}   // End of bounding_sphere::classify()



//----------------------------------------------------------------------------
// @ ::merge()
// ---------------------------------------------------------------------------
// merge two spheres together to create a new one
//-----------------------------------------------------------------------------
void
math3d::merge(bounding_sphere& result, 
       const bounding_sphere& s0, const bounding_sphere& s1)
{
    // get differences between them
    vector3 diff = vector3(s1.get_center() - s0.get_center());
    float distsq = diff.dot(diff);
    float radiusdiff = s1.get_radius() - s0.get_radius();

    // if one sphere inside other
    if (distsq <= radiusdiff*radiusdiff)
    {
        if (s0.get_radius() > s1.get_radius())
            result = s0;
        else
            result = s1;
        return;
    }

    // build new sphere
    float dist = ::math3d::sqrt(distsq);
    float radius = 0.5f*(s0.get_radius() + s1.get_radius() + dist);
    point3 center = s0.get_center();
    if (!math3d::is_zero(dist))
        center += ((radius-s0.get_radius())/dist)*diff;

    result.set_radius(radius);
    result.set_center(center);

}   // End of ::merge()


//----------------------------------------------------------------------------
// @ bounding_sphere::compute_collision()
// ---------------------------------------------------------------------------
// Compute parameters for collision between sphere and sphere
//-----------------------------------------------------------------------------
bool 
bounding_sphere::compute_collision(const bounding_sphere& other, vector3& collision_normal, 
                                   point3& collision_point, float& penetration) const
{
    // do sphere check
    float radius_sum = _radius + other._radius;
    collision_normal = vector3(other._center - _center);
    float distance_sq = collision_normal.length_squared();
    // if distance squared < sum of radii squared, collision!
    if (distance_sq <= radius_sum*radius_sum)
    {
        // handle collision

        // penetration is distance - radii
        float distance = ::math3d::sqrt(distance_sq);
        penetration = radius_sum - distance;
        collision_normal.normalize();

        // collision point is average of penetration
        collision_point = 0.5f*(_center + _radius*collision_normal) 
                        + 0.5f*(other._center - other._radius*collision_normal);

        return true;
    }

    return false;

}  // End of ::compute_collision()
