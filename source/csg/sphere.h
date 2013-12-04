#ifndef _RADIANT_CSG_SPHERE_H
#define _RADIANT_CSG_SPHERE_H

#include <ostream>
#include "dm/dm.h"
#include "point.h"
#include "protocols/forward_defines.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Quaternion;
class Matrix3;
class Ray3;

class Sphere
{
public:
      // constructor/destructor
      inline Sphere() : _center(0.0f, 0.0f, 0.0f), _radius(1.0f) { }
      inline Sphere(const Point3f& _center, float _radius) : _center(_center), _radius(_radius){ }
      inline ~Sphere() { }

      void SaveValue(protocol::sphere3f* msg) const;
      void LoadValue(const protocol::sphere3f& msg);

      // copy operations
      Sphere(const Sphere& other);
      Sphere& operator=(const Sphere& other);

      // accessors
      inline const Point3f& get_center() const { return _center; }
      inline float get_radius() const { return _radius; }

      // comparison
      bool operator==(const Sphere& other) const;
      bool operator!=(const Sphere& other) const;

      // manipulators
      inline void set_center(const Point3f& center)  { _center = center; }
      inline void set_radius(float radius)  { _radius = radius; }
      void set(const Point3f* points, unsigned int num_points);
      // void add_point(const Point3f& point);

      // transform!
      Sphere transform(float scale, const Quaternion& _rotation, const Point3f& translation) const;
      Sphere transform(float scale, const Matrix3& rotation, const Point3f& translation) const;

      // intersection
      bool intersect(const Sphere& other) const;
      bool intersect(const Ray3& ray) const;

      // collision parameters
      bool compute_collision(const Sphere& other, 
                           Point3f& collision_normal, 
                           Point3f& collision_point, 
                           float& penetration) const;

protected:
      Point3f        _center;
      float          _radius;
};
std::ostream& operator<<(std::ostream& out, const Sphere& source);
void merge(Sphere& result, const Sphere& s0, const Sphere& s1);

END_RADIANT_CSG_NAMESPACE

#endif
