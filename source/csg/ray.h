#ifndef _RADIANT_CSG_RAY_H
#define _RADIANT_CSG_RAY_H

#include <ostream>
#include "namespace.h"
#include "point.h"
#include "Quaternion.h"
#include "radiant.pb.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Ray3
{
public:
   // constructor/destructor
   Ray3();
   Ray3(const Point3f& origin, const Point3f& direction);
   inline ~Ray3() {}
    
   // copy operations
   Ray3(const Ray3& other);
   Ray3& operator=(const Ray3& other);

   void SaveValue(protocol::ray3f* msg) const {
      origin.SaveValue(msg->mutable_origin());
      direction.SaveValue(msg->mutable_direction());
   }
   void LoadValue(const protocol::ray3f& msg) {
      origin.LoadValue(msg.origin());
      direction.LoadValue(msg.direction());
   }

   // comparison
   bool operator==(const Ray3& ray) const;
   bool operator!=(const Ray3& ray) const;


   // manipulators
   void set(const Point3f& origin, const Point3f& direction);
   //inline void clean() { origin.clean(); direction.clean(); }

   // transform!
   Ray3 transform(float scale, const Quaternion& rotation, const Point3f& translation) const;

   // distance
   friend float distance_squared(const Ray3& ray0, const Ray3& ray1, float& s_c, float& t_c);
   /*
   friend float distance_squared(const Ray3& ray, const line3& line, float& s_c, float& t_c);
   inline friend float distance_squared(const line3& line, const Ray3& ray, float& s_c, float& t_c) {
      return distance_squared(ray, line, t_c, s_c);
   }
   */
   friend float distance_squared(const Ray3& ray, const Point3f& point, float& t_c);

   // closest points
   friend void closest_points(Point3f& point0, Point3f& point1, 
                              const Ray3& ray0, 
                              const Ray3& ray1);
   /*
   friend void closest_points(Point3f& point0, Point3f& point1, 
                              const Ray3& ray, 
                              const line3& line);
                              */
   Point3f closest_point(const Point3f& point) const;
        
public:
   Point3f  origin;
   Point3f  direction;
};
// text output (for debugging)
std::ostream& operator<<(std::ostream& out, const Ray3& source);

END_RADIANT_CSG_NAMESPACE

#endif
