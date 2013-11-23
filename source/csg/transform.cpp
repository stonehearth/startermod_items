#include "pch.h"
#include "csg/util.h" // xxx: should be csg/namespace.h
#include "transform.h"
#include "protocols/store.pb.h"
#include "dm/dm_save_impl.h"

using namespace radiant;
using namespace radiant::csg;

csg::Transform csg::Transform::zero(Point3f(0.0f, 0.0f, 0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

csg::Transform csg::Interpolate(const csg::Transform &t0, const csg::Transform &t1, float alpha)
{
   csg::Transform t;
   t.position = csg::Interpolate(t0.position, t1.position, alpha);
   t.orientation = csg::Interpolate(t0.orientation, t1.orientation, alpha);

   return t;
}

std::ostream& csg::operator<<(std::ostream& out, const Transform& source) {
   return out << "(position: " << source.position << " orientation: " << source.orientation << ")";
}

void Transform::SaveValue(protocol::transform *msg) const
{
   position.SaveValue(msg->mutable_position());
   orientation.SaveValue(msg->mutable_orientation());
}

void Transform::LoadValue(const protocol::transform &msg)
{
   position.LoadValue(msg.position());
   orientation.LoadValue(msg.orientation());
}

IMPLEMENT_DM_EXTENSION(::radiant::csg::Transform, Protocol::transform)
