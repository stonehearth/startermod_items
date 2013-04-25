#include "radiant.h"
#include "math3d_collision.h"

using namespace radiant;
using namespace radiant::math3d;

//-------------------------------------------------------------------------------
// @ aabb::aabb()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
aabb::aabb(const aabb& other) : 
    _min(other._min),
    _max(other._max)
{

}   // End of aabb::aabb()


//-------------------------------------------------------------------------------
// @ aabb::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
aabb&
aabb::operator=(const aabb& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    _min = other._min;
    _max = other._max;
    
    return *this;

}   // End of aabb::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& math3d::operator<<(std::ostream& out, const aabb& source)
{
    out << source._min << ' ' << source._max;

    return out;    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ aabb::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
aabb::operator==(const aabb& other) const
{
    if (other._min == _min && other._max == _max)
        return true;

    return false;   
}   // End of aabb::operator==()


//-------------------------------------------------------------------------------
// @ aabb::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
aabb::operator!=(const aabb& other) const
{
    if (other._min != _min || other._max != _max)
        return true;

    return false;

}   // End of aabb::operator!=()


//-------------------------------------------------------------------------------
// @ aabb::set()
//-------------------------------------------------------------------------------
// set AABB based on set of points
//-------------------------------------------------------------------------------
void
aabb::set(const point3* points, unsigned int num_points)
{
    ASSERT(points);

    // compute _minl and _maxl bounds
    _min = points[0];
    _max = points[0];
    for (unsigned int i = 1; i < num_points; ++i)
    {
        if (points[i].x < _min.x)
            _min.x = points[i].x;
        else if (points[i].x > _max.x)
            _max.x = points[i].x;
        if (points[i].y < _min.y)
            _min.y = points[i].y;
        else if (points[i].y > _max.y)
            _max.y = points[i].y;
        if (points[i].z < _min.z)
            _min.z = points[i].z;
        else if (points[i].z > _max.z)
            _max.z = points[i].z;
    }
}   // End of aabb::set()


//-------------------------------------------------------------------------------
// @ aabb::add_point()
//-------------------------------------------------------------------------------
// Add a point to the AABB
//-------------------------------------------------------------------------------
void
aabb::add_point(const point3& point)
{
    if (point.x < _min.x)
        _min.x = point.x;
    else if (point.x > _max.x)
        _max.x = point.x;
    if (point.y < _min.y)
        _min.y = point.y;
    else if (point.y > _max.y)
        _max.y = point.y;
    if (point.z < _min.z)
        _min.z = point.z;
    else if (point.z > _max.z)
        _max.z = point.z;

}   // End of aabb::add_point()


//----------------------------------------------------------------------------
// @ aabb::Intersects()
// ---------------------------------------------------------------------------
// Determine intersection between AABB and AABB
//-----------------------------------------------------------------------------
bool 
aabb::Intersects(const aabb& other) const
{
    // if separated in x direction
    if (_min.x + k_epsilon >= other._max.x || other._min.x + k_epsilon >= _max.x)
        return false;

    // if separated in y direction
    if (_min.y + k_epsilon >= other._max.y || other._min.y + k_epsilon >= _max.y)
        return false;

    // if separated in z direction
    if (_min.z + k_epsilon >= other._max.z || other._min.z + k_epsilon >= _max.z)
        return false;

    return true;
}

bool
aabb::collide(const aabb &b, const vector3 &velocity, vector3 &uv, float &u) const
{
#if 0
   if (intersect(b)) {
      u = 0.0f;
      uv.set_zero();
      return true;
   }
#endif

   uv.x = uv.y = uv.z = 1.0f;
   vector3 u_0(-FLT_MAX, -FLT_MAX, -FLT_MAX);
   vector3 u_1(FLT_MAX, FLT_MAX, FLT_MAX);
   float u0, u1;

   // find the possible first and last timesof overlap along each axis
   for (int i = 0 ; i < 3 ; i++) {
      if (_max[i] <= b._min[i] && velocity[i] < 0) {
         // | A | <-- | B |
         u_0[i] = (_max[i] - b._min[i]) / velocity[i];
         //DLOG(INFO) << " _max[i] " << _max[i] << " <= b._min[i] " << b._min[i] << " on " << ((char)('x' + i)) << " axis.  u is " << u_0[i];
      } else if (b._max[i] <= _min[i] && velocity[i] > 0) {
         // | B | --> | A |
         u_0[i] = (_min[i] - b._max[i]) / velocity[i];
         //DLOG(INFO) << " b._max[i] " << b._max[i] << " <= _min[i] " << _min[i] << " on " << ((char)('x' + i)) << " axis.  u is " << u_0[i];
      }

      if (b._max[i] >= _min[i] && velocity[i] < 0) {
         u_1[i] = (_min[i] - b._max[i]) / velocity[i];
      } else if (_max[i] >= b._min[i] && velocity[i] > 0) {
         u_1[i] = (_max[i] - b._min[i]) / velocity[i];
      }
   }

   //possible first time of overlap
   u0 = std::max(u_0.x, std::max(u_0.y, u_0.z));

   //possible last time of overlap
   u1 = std::min(u_1.x, std::min(u_1.y, u_1.z));

   //they could have only collided if
   //the first time of overlap occurred
   //before the last time of overlap
   if (u1 < u0 || u0 == -FLT_MAX) { // xx: not sure why the test for u0 is needed.  perhaps because we skip the "currently intersecting" test
      return false;
   }
   u = u0;
   uv = u_0;
   return true;
}

bool aabb::find_closest_point(point3 &closest, const ray3& ray) const
{
   float max_s = -FLT_MAX;
   float min_t = FLT_MAX;


   // do tests against three sets of planes
   for (int i = 0; i < 3; ++i) {
      // ray is parallel to plane
      if (math3d::is_zero(ray.direction[i])) {
         // ray passes by box
         if (ray.origin[i] < _min[i] || ray.origin[i] > _max[i]) {
            return false;
         }
      } else {
         // compute intersection parameters and sort
         float s = (_min[i] - ray.origin[i]) / ray.direction[i];
         float t = (_max[i] - ray.origin[i]) / ray.direction[i];
         if (s > t) {
            float temp = s;
            s = t;
            t = temp;
         }

         // adjust min and max values
         if (s > max_s) {
            max_s = s;
         }
         if (t < min_t) {
            min_t = t;
         }
         // check for intersection failure
         if (min_t < 0.0f || max_s > min_t) {
            return false;
         }
      }
   }
   closest = ray.origin + (max_s * ray.direction);

   // done, have intersection
   return true;
}

//----------------------------------------------------------------------------
// @ aabb::Intersects()
// ---------------------------------------------------------------------------
// Determine intersection between AABB and line
//-----------------------------------------------------------------------------
bool
aabb::Intersects(const line3& line) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // do tests against three sets of planes
    for (int i = 0; i < 3; ++i)
    {
        // line is parallel to plane
        if (math3d::is_zero(line.direction[i]))
        {
            // line passes by box
            if (line.origin[i] < _min[i] || line.origin[i] > _max[i])
                return false;
        }
        else
        {
            // compute intersection parameters and sort
            float s = (_min[i] - line.origin[i]) / line.direction[i];
            float t = (_max[i] - line.origin[i]) / line.direction[i];
            if (s > t)
            {
                float temp = s;
                s = t;
                t = temp;
            }

            // adjust std::min and std::max values
            if (s > max_s)
                max_s = s;
            if (t < min_t)
                min_t = t;
            // check for intersection failure
            if (max_s > min_t)
                return false;
        }
    }

    // done, have intersection
    return true;
}


//----------------------------------------------------------------------------
// @ aabb::Intersects()
// ---------------------------------------------------------------------------
// Determine intersection between AABB and ray
//-----------------------------------------------------------------------------
bool
aabb::Intersects(float &d, const ray3& ray) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // do tests against three sets of planes
    for (int i = 0; i < 3; ++i)
    {
        // ray is parallel to plane
        if (math3d::is_zero(ray.direction[i]))
        {
            // ray passes by box
            if (ray.origin[i] < _min[i] || ray.origin[i] > _max[i])
                return false;
        }
        else
        {
            // compute intersection parameters and sort
            float s = (_min[i] - ray.origin[i]) / ray.direction[i];
            float t = (_max[i] - ray.origin[i]) / ray.direction[i];
            if (s > t)
            {
                float temp = s;
                s = t;
                t = temp;
            }

            // adjust std::min and std::max values
            if (s > max_s)
                max_s = s;
            if (t < min_t)
                min_t = t;
            // check for intersection failure
            if (min_t < 0.0f || max_s > min_t)
                return false;
        }
    }
    d = max_s;

    // done, have intersection
    return true;
}


//----------------------------------------------------------------------------
// @ aabb::Intersects()
// ---------------------------------------------------------------------------
// Determine intersection between AABB and line segment
//-----------------------------------------------------------------------------
bool
aabb::Intersects(const line_segment3& segment) const
{
    float max_s = -FLT_MAX;
    float min_t = FLT_MAX;

    // do tests against three sets of planes
    for (int i = 0; i < 3; ++i)
    {
        // segment is parallel to plane
        if (math3d::is_zero(segment.direction[i]))
        {
            // segment passes by box
            if (segment.origin[i] < _min[i] || segment.origin[i] > _max[i])
                return false;
        }
        else
        {
            // compute intersection parameters and sort
            float s = (_min[i] - segment.origin[i]) / segment.direction[i];
            float t = (_max[i] - segment.origin[i]) / segment.direction[i];
            if (s > t)
            {
                float temp = s;
                s = t;
                t = temp;
            }

            // adjust std::min and std::max values
            if (s > max_s)
                max_s = s;
            if (t < min_t)
                min_t = t;
            // check for intersection failure
            if (min_t < 0.0f || max_s > 1.0f || max_s > min_t)
                return false;
        }
    }

    // done, have intersection
    return true;
}   


//----------------------------------------------------------------------------
// @ aabb:classify()
// ---------------------------------------------------------------------------
// Compute signed distance between AABB and plane
//-----------------------------------------------------------------------------
float
aabb::classify(const plane& plane) const
{
    point3 diag_min, diag_max;
    // set std::min/std::max values for x direction
    if (plane.normal.x >= 0.0f)
    {
        diag_min.x = _min.x;
        diag_max.x = _max.x;
    }
    else
    {
        diag_min.x = _min.x;
        diag_max.x = _max.x;
    }

    // set std::min/std::max values for y direction
    if (plane.normal.x >= 0.0f)
    {
        diag_min.y = _min.y;
        diag_max.y = _max.y;
    }
    else
    {
        diag_min.y = _min.y;
        diag_max.y = _max.y;
    }

    // set std::min/std::max values for z direction
    if (plane.normal.z >= 0.0f)
    {
        diag_min.z = _min.z;
        diag_max.z = _max.z;
    }
    else
    {
        diag_min.z = _min.z;
        diag_max.z = _max.z;
    }

    // minimum on positive side of plane, box on positive side
    float test = plane.test(diag_min);
    if (test > 0.0f)
        return test;

    test = plane.test(diag_max);
    // std::min on non-positive side, std::max on non-negative side, intersection
    if (test >= 0.0f)
        return 0.0f;
    // std::max on negative side, box on negative side
    else
        return test;

}   // End of AABB::classify()


//----------------------------------------------------------------------------
// @ ::merge()
// ---------------------------------------------------------------------------
// merge two AABBs together to create a new one
//-----------------------------------------------------------------------------
void
math3d::merge(aabb& result, const aabb& b0, const aabb& b1)
{
    point3 new__min(b0._min);
    point3 new__max(b0._max);

    if (b1._min.x < new__min.x)
        new__min.x = b1._min.x;
    if (b1._min.y < new__min.y)
        new__min.y = b1._min.y;
    if (b1._min.z < new__min.z)
        new__min.z = b1._min.z;

    if (b1._max.x > new__max.x)
        new__max.x = b1._max.x;
    if (b1._max.y > new__max.y)
        new__max.y = b1._max.y;
    if (b1._max.z > new__max.z)
        new__max.z = b1._max.z;

    // set new box
    result._min = new__min;
    result._max = new__max;

}   // End of ::merge()


math3d::ibounds3 aabb::ToBounds() const
{
   return math3d::ibounds3(
      math3d::ipoint3(floor(_min.x + k_epsilon), 
                      floor(_min.y + k_epsilon),
                      floor(_min.z + k_epsilon)),
      math3d::ipoint3(ceil(_max.x + -k_epsilon),
                      ceil(_max.y + -k_epsilon),
                      ceil(_max.z + -k_epsilon)));
}

aabb aabb::rotate(const quaternion& q) const
{
   math3d::point3 min = q.rotate(_min);
   math3d::point3 max = q.rotate(_max);

   aabb result(min, max);
   result.Clean();

   return result;
}

void aabb::Clean()
{
   for (int i = 0; i < 3; i++) {
      if (_min[i] > _max[i]) {
         std::swap(_min[i], _max[i]);
      }
   }
}
