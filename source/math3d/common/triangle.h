//===============================================================================
// @ triangle.h
// 
// Useful triangle-related routines
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

#ifndef _RADIANT_MATH3D_TRIANGLE_H
#define _RADIANT_MATH3D_TRIANGLE_H

namespace radiant {
   namespace math3d {

      class point2;
      class point3;
      class ray3;
      class plane;

      // 3D versions
      bool is_point_in_triangle(const point3& point, const point3& p0,
                                const point3& p1, const point3& p2);

      void barycentric_coordinates(float &r, float &s, float& t, 
                                   const point3& point, const point3& p0,
                                   const point3& p1, const point3& p2);

      // intersection
      bool triangle_intersect(const point3& p0, const point3& p1, 
                              const point3& p2, const point3& q0, 
                              const point3& q1, const point3& q2);

      bool triangle_intersect(float& t, const point3& p0, const point3& p1, 
                              const point3& p2, const ray3& ray);

      // plane classification
      float triangle_classify(const point3& p0, const point3& p1, 
                              const point3& p2, const plane& plane);

      // 2D versions
      bool is_point_in_triangle(const point2& point, const point2& p0,
                               const point2& p1, const point2& p2);

      void barycentric_coordinates(float &r, float &s, float& t, 
                                   const point2& point, const point2& p0,
                                   const point2& p1, const point2& p2);

      bool triangle_intersect(const point2& p0, const point2& p1, 
                              const point2& p2, const point2& q0, 
                              const point2& q1, const point2& q2);
   };
};

#endif
