//===============================================================================
// @ capsule.h
// 
// Capsule class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COLLISION_CAPSULE_H
#define _RADIANT_MATH3D_COLLISION_CAPSULE_H

namespace radiant {
   namespace math3d {
      class capsule
      {
      public:
          // constructor/destructor
          inline capsule() { }
          inline capsule(const point3& endpoint0, const point3& endpoint1, float r) :_segment(endpoint0, endpoint1), _radius(r) { }
          inline capsule(const line_segment3& segment, float r) : _segment(segment), _radius(r) { }
          inline ~capsule() { }

          // copy operations
          capsule(const capsule& other);
          capsule& operator=(const capsule& other);


          // accessors
          const line_segment3& get_segment() const { return _segment; }
          const float& get_radius() const { return _radius; }
          float length() const;
          float length_squared() const;

          // comparison
          bool operator==(const capsule& other) const;
          bool operator!=(const capsule& other) const;

          // manipulators
          inline void set_radius(float radius)  { _radius = radius; }
          inline void set_segment(const point3 &p0, const point3 &p1) { _segment.set(p0, p1); }

          void set(const point3* points, unsigned int num_points);

          // transform!
          capsule transform(float scale, const quaternion& _rotation,  const vector3& translation) const;
          capsule transform(float scale, const matrix3& _rotation, const vector3& translation) const;

          // intersection
          bool intersect(const capsule& other) const;
          bool intersect(const line3& line) const;
          bool intersect(const ray3& ray) const;
          bool intersect(const line_segment3& _segment) const;

          // signed distance to plane
          float classify(const plane& plane) const;

          // collision parameters
          bool compute_collision(const capsule& other, 
                                 vector3& collision_normal, 
                                 point3& collision_point, 
                                 float& penetration) const;

      protected:
          line_segment3   _segment;
          float           _radius;
      };
      std::ostream& operator<<(std::ostream& out, const capsule& source);
      void merge(capsule& result, const capsule& b0, const capsule& b1);
   };
};

//-------------------------------------------------------------------------------
//-- Inlines --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Externs --------------------------------------------------------------------
//-------------------------------------------------------------------------------

#endif
