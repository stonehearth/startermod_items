//===============================================================================
// @ bounding_sphere.h
// 
// Bounding sphere class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COLLISION_BOUNDING_SPHERE_H
#define _RADIANT_MATH3D_COLLISION_BOUNDING_SPHERE_H

#include "math3d.h"
#include "radiant.pb.h"

namespace radiant {
   namespace math3d {
      class bounding_sphere
      {
      public:
          // constructor/destructor
          inline bounding_sphere() : _center(0.0f, 0.0f, 0.0f), _radius(1.0f) { }
          inline bounding_sphere(const point3& _center, float _radius) : _center(_center), _radius(_radius){ }
          inline ~bounding_sphere() { }

          void SaveValue(protocol::sphere* msg) const {
             _center.SaveValue(msg->mutable_center());
             msg->set_radius(_radius);
          }
          void LoadValue(const protocol::sphere& msg) {
             _center.LoadValue(msg.center());
             _radius = msg.radius();
          }

          // copy operations
          bounding_sphere(const bounding_sphere& other);
          bounding_sphere& operator=(const bounding_sphere& other);

          // accessors
          inline const point3& get_center() const { return _center; }
          inline float get_radius() const { return _radius; }

          // comparison
          bool operator==(const bounding_sphere& other) const;
          bool operator!=(const bounding_sphere& other) const;

          // manipulators
          inline void set_center(const point3& center)  { _center = center; }
          inline void set_radius(float radius)  { _radius = radius; }
          void set(const point3* points, unsigned int num_points);
          // void add_point(const point3& point);

          // transform!
          bounding_sphere transform(float scale, const quaternion& _rotation, const vector3& translation) const;
          bounding_sphere transform(float scale, const matrix3& rotation, const vector3& translation) const;

          // intersection
          bool intersect(const bounding_sphere& other) const;
          bool intersect(const line3& line) const;
          bool intersect(const ray3& ray) const;
          bool intersect(const line_segment3& segment) const;

          // signed distance to plane
          float classify(const plane& plane) const;

          // collision parameters
          bool compute_collision(const bounding_sphere& other, 
                                 vector3& collision_normal, 
                                 point3& collision_point, 
                                 float& penetration) const;

      protected:
          point3          _center;
          float           _radius;
      };
      std::ostream& operator<<(std::ostream& out, const bounding_sphere& source);
      void merge(bounding_sphere& result, const bounding_sphere& s0, const bounding_sphere& s1);
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::bounding_sphere, Protocol::sphere)

#endif
