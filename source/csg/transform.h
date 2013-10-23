#ifndef _RADIANT_CSG_TRANSFORM_H
#define _RADIANT_CSG_TRANSFORM_H

#include <ostream>
#include "namespace.h"
#include "point.h"
#include "quaternion.h"

BEGIN_RADIANT_CSG_NAMESPACE

struct Transform {
   Point3f            position;
   Quaternion        orientation;

   Transform() { }
   Transform(const Transform &t) : position(t.position), orientation(t.orientation) { }
   Transform(const Point3f &p, const Quaternion &q) : position(p), orientation(q) { }

   void SetZero() { position.SetZero(); orientation.SetIdentity(); }

   void SaveValue(protocol::transform *msg) const {
      position.SaveValue(msg->mutable_position());
      orientation.SaveValue(msg->mutable_orientation());
   }

   void LoadValue(const protocol::transform &msg) {
      position.LoadValue(msg.position());
      orientation.LoadValue(msg.orientation());
   }

   bool operator==(const Transform &o) { return position == o.position && orientation == o.orientation; }
   bool operator!=(const Transform &o) { return position != o.position && orientation != o.orientation; }

   static Transform zero;
};
Transform Interpolate(const Transform &t0, const Transform &t1, float alpha);
std::ostream& operator<<(std::ostream& out, const Transform& source);

END_RADIANT_CSG_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::csg::Transform, Protocol::transform)

#endif
