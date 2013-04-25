//===============================================================================
// @ math3d::ipoint3.h
// 
// Description
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH_TRANSFORM_H
#define _RADIANT_MATH_TRANSFORM_H

#include "point3.h"
#include "quaternion.h"
#include "dm/dm.h"

namespace radiant {
   namespace math3d {
      struct transform {
         point3            position;
         quaternion        orientation;

         transform() { }
         transform(const transform &t) : position(t.position), orientation(t.orientation) { }
         transform(const math3d::point3 &p, const quaternion &q) : position(p), orientation(q) { }

         void set_zero() { position.set_zero(); orientation.set_identity(); }

         transform(const protocol::transform &t) : position(t.position()), orientation(t.orientation()) { }

         static decltype(Protocol::transform) extension;

         void SaveValue(protocol::transform *msg) const {
            position.SaveValue(msg->mutable_position());
            orientation.SaveValue(msg->mutable_orientation());
         }

         void LoadValue(const protocol::transform &msg) {
            position.LoadValue(msg.position());
            orientation.LoadValue(msg.orientation());
         }

         bool operator==(const transform &o) { return position == o.position && orientation == o.orientation; }
         bool operator!=(const transform &o) { return position != o.position && orientation != o.orientation; }

         static transform zero;
      };
      transform interpolate(const transform &t0, const transform &t1, float alpha);
      std::ostream& operator<<(std::ostream& out, const transform& source);
   };
};

IMPLEMENT_DM_EXTENSION(::radiant::math3d::transform, Protocol::transform)

#endif
