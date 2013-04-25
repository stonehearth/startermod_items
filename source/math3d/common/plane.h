//===============================================================================
// @ plane.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_PLANE_H
#define _RADIANT_MATH_PLANE_H

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "common.h"
#include "vector3.h"


namespace radiant {
   namespace math3d {

      class ray3;
      class line3;
      class vector3;
      class quaternion;

      class plane
      {
         public:
             // constructor/destructor
             plane();
             plane(float a, float b, float c, float d);
             plane(const vector3& p0, const vector3& p1, const vector3& p2);
             inline ~plane() {}
    
             // copy operations
             plane(const plane& other);
             plane& operator=(const plane& other);

             // accessors
             void get(vector3& normal, float& direction) const;

             // comparison
             bool operator==(const plane& ray) const;
             bool operator!=(const plane& ray) const;

             // manipulators
             void set(float a, float b, float c, float d);
             inline void set(const vector3& n, float d) { set(n.x, n.y, n.z, d); }
             void set(const vector3& p0, const vector3& p1, const vector3& p2);

             // transform!
             plane transform(float scale, const quaternion& rotation, 
                                const vector3& translation) const;

             // distance
             inline friend float distance(const plane& plane, const point3& point)
             {
                 return abs(plane.test(point));
             }

             bool intersects(const ray3 &ray, float &t) const;

             // closest point
             point3 closest_point(const point3& point) const;

             // result of plane test
             inline float test(const point3& point) const
             {
                 return normal.dot(vector3(point)) + offset;
             }
        
         public:
             vector3    normal;
             float      offset;
      };
      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const plane& source);
   };
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
