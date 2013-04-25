//===============================================================================
// @ ray3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_RAY3_H
#define _RADIANT_MATH_RAY3_H

#include "common.h"
#include "vector3.h"
#include "point3.h"
#include "radiant.pb.h"

namespace radiant {
   namespace math3d {

      class line3;
      class quaternion;

      class ray3
      {
      public:
          // constructor/destructor
          ray3();
          ray3(const point3& origin, const vector3& direction);
          inline ~ray3() {}
          ray3(const protocol::ray3& msg) {
             LoadValue(msg);
          }
    
          // copy operations
          ray3(const ray3& other);
          ray3& operator=(const ray3& other);

          void SaveValue(protocol::ray3 *msg) const {
             origin.SaveValue(msg->mutable_origin());
             direction.SaveValue(msg->mutable_direction());
          }
          void LoadValue(const protocol::ray3& msg) {
             origin.LoadValue(msg.origin());
             direction.LoadValue(msg.direction());
          }

          // comparison
          bool operator==(const ray3& ray) const;
          bool operator!=(const ray3& ray) const;


          // manipulators
          void set(const point3& origin, const vector3& direction);
          inline void clean() { origin.clean(); direction.clean(); }

          // transform!
          ray3 transform(float scale, const quaternion& rotation, const vector3& translation) const;

          // distance
          friend float distance_squared(const ray3& ray0, const ray3& ray1, float& s_c, float& t_c);
          friend float distance_squared(const ray3& ray, const line3& line, float& s_c, float& t_c);
          inline friend float distance_squared(const line3& line, const ray3& ray, float& s_c, float& t_c) {
              return distance_squared(ray, line, t_c, s_c);
          }
          friend float distance_squared(const ray3& ray, const point3& point, float& t_c);

          // closest points
          friend void closest_points(point3& point0, point3& point1, 
                                     const ray3& ray0, 
                                     const ray3& ray1);
          friend void closest_points(point3& point0, point3& point1, 
                                     const ray3& ray, 
                                     const line3& line);
          point3 closest_point(const point3& point) const;
        
      public:
          point3   origin;
          vector3  direction;
      };
      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const ray3& source);
   };
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
