#include "pch.h"
#include "matrix4.h"
#include "ray.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;


//----------------------------------------------------------------------------
// @ Ray3::Ray3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Ray3::Ray3() :
    origin(0.0f, 0.0f, 0.0f),
    direction(1.0f, 0.0f, 0.0f)
{
}   // End of Ray3::Ray3()


//----------------------------------------------------------------------------
// @ Ray3::Ray3()
// ---------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Ray3::Ray3(const Point3f& o, const Point3f& d) :
    origin(o),
    direction(d)
{
    direction.Normalize();
}   // End of Ray3::Ray3()


//----------------------------------------------------------------------------
// @ Ray3::Ray3()
// ---------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------
Ray3::Ray3(const Ray3& other) :
    origin(other.origin),
    direction(other.direction)
{
    direction.Normalize();

}   // End of Ray3::Ray3()


//----------------------------------------------------------------------------
// @ Ray3::operator=()
// ---------------------------------------------------------------------------
// Assigment operator
//-----------------------------------------------------------------------------
Ray3&
Ray3::operator=(const Ray3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    origin = other.origin;
    direction = other.direction;
    direction.Normalize();

    return *this;
}   // End of Ray3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Output a text Ray3.  The format is assumed to be :
// [ Point3f, Point3f ]
//-------------------------------------------------------------------------------
std::ostream& 
csg::operator<<(std::ostream& out, const Ray3& source)
{
    return out << "[" << source.origin 
                     << ", " << source.direction << "]";
    
}   // End of operator<<()
    

//----------------------------------------------------------------------------
// @ Ray3::operator==()
// ---------------------------------------------------------------------------
// Are two Ray3's equal?
//----------------------------------------------------------------------------
bool
Ray3::operator==(const Ray3& ray) const
{
    return (ray.origin == origin && ray.direction == direction);

}  // End of Ray3::operator==()


//----------------------------------------------------------------------------
// @ Ray3::operator!=()
// ---------------------------------------------------------------------------
// Are two Ray3's not equal?
//----------------------------------------------------------------------------
bool
Ray3::operator!=(const Ray3& ray) const
{
    return !(ray.origin == origin && ray.direction == direction);
}  // End of Ray3::operator!=()


//----------------------------------------------------------------------------
// @ Ray3::set()
// ---------------------------------------------------------------------------
// Sets the two parameters
//-----------------------------------------------------------------------------
void
Ray3::set(const Point3f& o, const Point3f& d)
{
    origin = o;
    direction = d;
    direction.Normalize();

}   // End of Ray3::set()


//----------------------------------------------------------------------------
// @ Ray3::transform()
// ---------------------------------------------------------------------------
// Transforms ray into new space
//-----------------------------------------------------------------------------
Ray3  
Ray3::transform(float scale, const Quaternion& rotate, const Point3f& translate) const
{
    Ray3 ray;
    Matrix4 transform(rotate);
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
    ray.direction.Normalize();

    transform(0,3) = translate.x;
    transform(1,3) = translate.y;
    transform(2,3) = translate.z;

    ray.origin = transform.transform(origin);

    return ray;

}   // End of Ray3::transform()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between rays.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
distance_squared(const Ray3& ray0, const Ray3& ray1, 
                 float& s_c, float& t_c)
{
    // compute intermediate parameters
    Point3f w0 = Point3f(ray0.origin - ray1.origin);
    float a = ray0.direction.Dot(ray0.direction);
    float b = ray0.direction.Dot(ray1.direction);
    float c = ray1.direction.Dot(ray1.direction);
    float d = ray0.direction.Dot(w0);
    float e = ray1.direction.Dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (IsZero(denom))
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
    Point3f wc = w0 + ray0.direction*s_c - ray1.direction*t_c;
    return wc.Dot(wc);

}   // End of ::distance_squared()


#if 0
//----------------------------------------------------------------------------
// @ Ray3::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between ray and line.
// Based on article and code by Dan Sunday at www.geometryalgorithms.com
//-----------------------------------------------------------------------------
float
distance_squared(const Ray3& ray, const line3& line, 
                   float& s_c, float& t_c)
{
    // compute intermediate parameters
    Point3f w0 = Point3f(ray.origin - line.origin);
    float a = ray.direction.Dot(ray.direction);
    float b = ray.direction.Dot(line.direction);
    float c = line.direction.Dot(line.direction);
    float d = ray.direction.Dot(w0);
    float e = line.direction.Dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (IsZero(denom))
    {
        s_c = 0.0f;
        t_c = e/c;
        // compute difference vector and distance squared
        Point3f wc = w0 - t_c*line.direction;
        return wc.Dot(wc);
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
        Point3f wc = w0 + s_c*ray.direction - t_c*line.direction;
        return wc.Dot(wc);
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ ::distance_squared()
// ---------------------------------------------------------------------------
// Returns the distance squared between ray and point.
//-----------------------------------------------------------------------------
float distance_squared(const Ray3& ray, const Point3f& point, 
                       float& t_c) 
{
    Point3f w = Point3f(point - ray.origin);
    float proj = w.Dot(ray.direction);
    // origin is closest point
    if (proj <= 0)
    {
        t_c = 0.0f;
        return w.Dot(w);
    }
    // else use normal line check
    else
    {
        float vsq = ray.direction.Dot(ray.direction);
        t_c = proj/vsq;
        return w.Dot(w) - t_c*proj;
    }

}   // End of ::distance_squared()


//----------------------------------------------------------------------------
// @ closest_points()
// ---------------------------------------------------------------------------
// Returns the closest points between two rays
//-----------------------------------------------------------------------------
void closest_points(Point3f& point0, Point3f& point1, 
                    const Ray3& ray0, 
                    const Ray3& ray1)
{
    // compute intermediate parameters
    Point3f w0 = Point3f(ray0.origin - ray1.origin);
    float a = ray0.direction.Dot(ray0.direction);
    float b = ray0.direction.Dot(ray1.direction);
    float c = ray1.direction.Dot(ray1.direction);
    float d = ray0.direction.Dot(w0);
    float e = ray1.direction.Dot(w0);

    float denom = a*c - b*b;
    // parameters to compute s_c, t_c
    float s_c, t_c;
    float sn, sd, tn, td;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (IsZero(denom))
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
void closest_points(Point3f& point0, Point3f& point1, 
                    const Ray3& ray, 
                    const line3& line)
{
    // compute intermediate parameters
    Point3f w0 = Point3f(ray.origin - line.origin);
    float a = ray.direction.Dot(ray.direction);
    float b = ray.direction.Dot(line.direction);
    float c = line.direction.Dot(line.direction);
    float d = ray.direction.Dot(w0);
    float e = line.direction.Dot(w0);

    float denom = a*c - b*b;

    // if denom is zero, try finding closest point on ray1 to origin0
    if (IsZero(denom))
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

#endif

//----------------------------------------------------------------------------
// @ Ray3::closest_point()
// ---------------------------------------------------------------------------
// Returns the closest point on ray to point
//-----------------------------------------------------------------------------
Point3f 
Ray3::closest_point(const Point3f& point) const
{
    Point3f w = Point3f(point - origin);
    float proj = w.Dot(direction);
    // endpoint 0 is closest point
    if (proj <= 0.0f)
        return origin;
    else
    {
        float vsq = direction.Dot(direction);
        // else somewhere else in ray
        return origin + direction*(proj/vsq);
    }
}

void Ray3::SaveValue(protocol::ray3f* msg) const
{
   origin.SaveValue(msg->mutable_origin());
   direction.SaveValue(msg->mutable_direction());
}

void Ray3::LoadValue(const protocol::ray3f& msg)
{
   origin.LoadValue(msg.origin());
   direction.LoadValue(msg.direction());
}
