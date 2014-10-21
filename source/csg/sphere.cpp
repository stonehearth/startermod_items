#include "pch.h"
#include "csg.h"
#include "sphere.h"
#include "quaternion.h"
#include "matrix3.h"
#include "ray.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;


Sphere::Sphere(const Sphere& other) : 
    _center(other._center),
    _radius(other._radius)
{

}   // End of Sphere::Sphere()


//-------------------------------------------------------------------------------
// @ Sphere::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
Sphere&
Sphere::operator=(const Sphere& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    _center = other._center;
    _radius = other._radius;
    
    return *this;

}   // End of Sphere::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
csg::operator<<(std::ostream& out, const Sphere& source)
{
    out << source.get_center();
    out << ' ' << source.get_radius();

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ Sphere::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
Sphere::operator==(const Sphere& other) const
{
    if (other._center == _center && other._radius == _radius)
        return true;

    return false;   
}   // End of Sphere::operator==()


//-------------------------------------------------------------------------------
// @ Sphere::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
Sphere::operator!=(const Sphere& other) const
{
    if (other._center != _center || other._radius != _radius)
        return true;

    return false;

}   // End of Sphere::operator!=()


//-------------------------------------------------------------------------------
// @ Sphere::set()
//-------------------------------------------------------------------------------
// set bounding sphere based on set of points
//-------------------------------------------------------------------------------
void
Sphere::set(const Point3f* points, unsigned int num_points)
{
    ASSERT(points);
    // compute minimal and maximal bounds
    Point3f min_p(points[0]), max_p(points[0]);
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
    _center = (min_p + max_p) * 0.5;
    double maxDistance = (_center - points[0]).LengthSquared();
    for (i = 1; i < num_points; ++i)
    {
        double dist = (_center - points[i]).LengthSquared();
        if (dist > maxDistance)
            maxDistance = dist;
    }
    _radius = std::sqrt(maxDistance);
}


//----------------------------------------------------------------------------
// @ Sphere::transform()
// ---------------------------------------------------------------------------
// Transforms sphere into new space
//-----------------------------------------------------------------------------
Sphere    
Sphere::transform(double scale, const Quaternion& rotate, 
    const Point3f& translate) const
{
    return Sphere(rotate.rotate(_center) + translate, _radius*scale);

}   // End of Sphere::transform()


//----------------------------------------------------------------------------
// @ Sphere::transform()
// ---------------------------------------------------------------------------
// Transforms sphere into new space
//-----------------------------------------------------------------------------
Sphere    
Sphere::transform(double scale, const Matrix3& rotate, 
    const Point3f& translate) const
{
    return Sphere(rotate*_center + translate, _radius*scale);

}   // End of Sphere::transform()


//----------------------------------------------------------------------------
// @ Sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and sphere
//-----------------------------------------------------------------------------
bool 
Sphere::intersect(const Sphere& other) const
{
    // do sphere check
    double radius_sum = _radius + other._radius;
    Point3f center_diff = Point3f(other._center - _center); 
    double distance_sq = center_diff.LengthSquared();

    // if distance squared < sum of radii squared, collision!
    return (distance_sq <= radius_sum*radius_sum);
}

//----------------------------------------------------------------------------
// @ Sphere::intersect()
// ---------------------------------------------------------------------------
// Determine intersection between sphere and ray
//-----------------------------------------------------------------------------
bool
Sphere::intersect(const Ray3& ray) const
{
    // compute intermediate values
    Point3f w = Point3f(_center - ray.origin);
    double wsq = w.Dot(w);
    double proj = w.Dot(ray.direction);
    double rsq = _radius*_radius;

    // if sphere behind ray, no intersection
    if (proj < 0.0f && wsq > rsq)
        return false;
    double vsq = ray.direction.Dot(ray.direction);

    // test length of difference vs. _radius
    return (vsq*wsq - proj*proj <= vsq*rsq);
}


//----------------------------------------------------------------------------
// @ ::merge()
// ---------------------------------------------------------------------------
// merge two spheres together to create a new one
//-----------------------------------------------------------------------------
void
csg::merge(Sphere& result, 
       const Sphere& s0, const Sphere& s1)
{
    // get differences between them
    Point3f diff = Point3f(s1.get_center() - s0.get_center());
    double distsq = diff.Dot(diff);
    double radiusdiff = s1.get_radius() - s0.get_radius();

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
    double dist = std::sqrt(distsq);
    double radius = 0.5*(s0.get_radius() + s1.get_radius() + dist);
    Point3f center = s0.get_center();
    if (!csg::IsZero(dist))
        center += diff * ((radius-s0.get_radius())/dist);

    result.set_radius(radius);
    result.set_center(center);

}   // End of ::merge()


//----------------------------------------------------------------------------
// @ Sphere::compute_collision()
// ---------------------------------------------------------------------------
// Compute parameters for collision between sphere and sphere
//-----------------------------------------------------------------------------
bool 
Sphere::compute_collision(const Sphere& other, Point3f& collision_normal, 
                                   Point3f& collision_point, double& penetration) const
{
    // do sphere check
    double radius_sum = _radius + other._radius;
    collision_normal = Point3f(other._center - _center);
    double distance_sq = collision_normal.LengthSquared();
    // if distance squared < sum of radii squared, collision!
    if (distance_sq <= radius_sum*radius_sum)
    {
        // handle collision

        // penetration is distance - radii
        double distance = std::sqrt(distance_sq);
        penetration = radius_sum - distance;
        collision_normal.Normalize();

        // collision point is average of penetration
        collision_point = (_center + collision_normal * _radius) * 0.5
                        + (other._center - collision_normal * other._radius) * 0.5;

        return true;
    }

    return false;

}  // End of ::compute_collision()

void Sphere::SaveValue(protocol::sphere3f* msg) const {
   _center.SaveValue(msg->mutable_center());
   msg->set_radius(_radius);
}

void Sphere::LoadValue(const protocol::sphere3f& msg) {
   _center.LoadValue(msg.center());
   _radius = msg.radius();
}

