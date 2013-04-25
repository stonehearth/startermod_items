//===============================================================================
// @ obb.h
// 
// Object-oriented bounding box class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COLLISION_OBB_H
#define _RADIANT_MATH3D_COLLISION_OBB_H

namespace radiant {
   namespace math3d {
      class obb
      {
         public:
             // constructor/destructor
             inline obb() : _center(0.0f, 0.0f, 0.0f), _extents(1.0f, 1.0f, 1.0f) { _rotation.set_identity(); }
             inline ~obb() { }

             // copy operations
             obb(const obb& other);
             obb& operator=(const obb& other);

             // comparison
             bool operator==(const obb& other) const;
             bool operator!=(const obb& other) const;

             // accessors
             const point3& get_center() const { return _center; }
             const matrix3& get_rotation() const { return _rotation; }
             const vector3& get_extents() const { return _extents; }

             // manipulators
             void set(const point3* points, unsigned int num_points);
             inline void set_center(const point3& c) { _center = c; }
             inline void set_rotation(const matrix3& R) { _rotation = R; }
             inline void set_extents(const vector3& h) { _extents = h; }

             // transform!
             obb transform(float scale, const matrix3& _rotation, const vector3& translation) const;
             obb transform(float scale, const quaternion& _rotation, const vector3& translation) const;

             // intersection
             bool intersect(const obb& other) const;
             bool intersect(const line3& line) const;
             bool intersect(const ray3& ray) const;
             bool intersect(const line_segment3& segment) const;

             // signed distance to plane
             float classify(const plane& plane) const;          

         protected:
             point3        _center;
             matrix3       _rotation;
             vector3       _extents;

      };
      std::ostream& operator<<(std::ostream& out, const obb& source);

      void merge(obb& result, const obb& b0, const obb& b1);
   };
};


#endif
