//===============================================================================
// @ line_segment3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_LINE_SEGMENT3_H
#define _RADIANT_MATH3D_LINE_SEGMENT3_H

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "vector3.h"

//-------------------------------------------------------------------------------
//-- Typedefs, Structs ----------------------------------------------------------
//-------------------------------------------------------------------------------

namespace radiant {
   namespace math3d {
      class vector3;
      class quaternion;
      class line3;
      class ray3;
      class matrix3;

      //-------------------------------------------------------------------------------
      //-- Classes --------------------------------------------------------------------
      //-------------------------------------------------------------------------------

      class line_segment3
      {
      public:
          // constructor/destructor
          line_segment3();
          line_segment3(const point3& endpoint0, const point3& endpoint1);
          inline ~line_segment3() {}
    
          // copy operations
          line_segment3(const line_segment3& other);
          line_segment3& operator=(const line_segment3& other);

          // accessors
          float& operator()(unsigned int i, unsigned int j);
          float  operator()(unsigned int i, unsigned int j) const;

          inline const point3 &get_endpoint_0() const { return origin; }
          inline point3 get_endpoint_1() const { return origin + direction; }
          inline point3 get_center() const { return origin + 0.5f*direction; }

          float length() const;
          float length_squared() const;

          // comparison
          bool operator==(const line_segment3& segment) const;
          bool operator!=(const line_segment3& segment) const;

#if !defined(SWIG)
          void SaveValue(protocol::line *line) const {
             get_endpoint_0().SaveValue(line->mutable_p0());
             get_endpoint_1().SaveValue(line->mutable_p1());
          }
#endif

          // manipulators
          void set(const point3& end0, const point3& end1);
          inline void clean() { origin.clean(); direction.clean(); }

          // transform!
          line_segment3 transform(float scale, const quaternion& rotation, 
                                    const vector3& translation) const;
          line_segment3 transform(float scale, const matrix3& rotation, 
                                    const vector3& translation) const;

          point3 closest_point(const point3& point) const;
        
      public:
          point3 origin;
          vector3 direction;
      };
      // distance
      float distance_squared(const line_segment3& segment0, const line_segment3& segment1,  float& s_c, float& t_c);
      float distance_squared(const line_segment3& segment, const ray3& ray, float& s_c, float& t_c);
      float distance_squared(const line_segment3& segment, const line3& line, float& s_c, float& t_c);
      float distance_squared(const line_segment3& segment, const point3& point, float& t_c);
      inline float distance_squared(const ray3& ray, const line_segment3& segment, float& s_c, float& t_c) {
         return distance_squared(segment, ray, t_c, s_c);
      }
      inline float distance_squared(const line3& line,  const line_segment3& segment, float& s_c, float& t_c) {
         return distance_squared(segment, line, t_c, s_c);
      }

      // closest points
      void closest_points(point3& point0, point3& point1, const line_segment3& segment0,  const line_segment3& segment1);
      void closest_points(point3& point0, point3& point1, const line_segment3& segment, const ray3& ray);
      void closest_points(point3& point0, point3& point1, const line_segment3& segment, const line3& line);

      // text output (for debugging)
      std::ostream& operator<<(std::ostream& out, const line_segment3& source);
   };
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
