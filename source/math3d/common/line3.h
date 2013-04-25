//===============================================================================
// @ line3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_LINE_H
#define _RADIANT_MATH_LINE_H

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "vector3.h"

//-------------------------------------------------------------------------------
//-- Typedefs, Structs ----------------------------------------------------------
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
//-- Classes --------------------------------------------------------------------
//-------------------------------------------------------------------------------

namespace radiant {
   namespace math3d {
      class vector3;
      class quaternion;

      class line3
      {
      public:
          // constructor/destructor
          line3();
          line3(const point3& origin, const vector3& direction);
          inline ~line3() {}
    
          // copy operations
          line3(const line3& other);
          line3& operator=(const line3& other);

          // comparison
          bool operator==(const line3& line) const;
          bool operator!=(const line3& line) const;

          // manipulators
          void set(const point3& origin, const vector3& direction);
          inline void clean() { origin.clean(); direction.clean(); }

          // transform!
          line3 transform(float scale, const quaternion& rotation, 
                                    const vector3& translation) const;

          point3 closest_point(const point3& point) const;
        
      public:
          point3  origin;
          vector3 direction;

      };

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const line3& source);

      // distance
      float distance_squared(const line3& line, const point3& point, float &t_c);
      float distance_squared(const line3& line0, const line3& line1, float& s_c, float& t_c);
      inline float distance(const line3& line0, const line3& line1, float& s_c, float& t_c) {
         return sqrt(distance_squared(line0, line1, s_c, t_c));
      }
      inline float distance(const line3& line, const point3& point, float &t_c) {
         return sqrt(distance_squared(line, point, t_c));
      }

      // closest points
      void closest_points(point3& point0, point3& point1, const line3& line0, const line3& line1);
   };
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
