#include "pch.h"
#include "mob.ridl.h"
#include "om/object_formatter/object_formatter.h"
#include "csg/util.h" // xxx: should be in csg/csg.h

using namespace ::radiant;
using namespace ::radiant::om;


std::ostream& operator<<(std::ostream& os, Mob const& o)
{
   return (os << "[Mob]");
}

void Mob::ConstructObject()
{
   transform_ = csg::Transform(csg::Point3f::zero, csg::Quaternion());
   aabb_ = csg::Cube3f::zero;
   interpolate_movement_ = true;
   selectable_ = true;
   moving_ = false;
}

void Mob::MoveTo(const csg::Point3f& location)
{
   LOG(INFO) << "moving entity " << GetEntity().GetObjectId() << " to " << location;
   transform_.Modify([&](csg::Transform& t) {
      t.position = location;
   });
}

void Mob::MoveToGridAligned(const csg::Point3& location)
{
   MoveTo(csg::ToFloat(location));
}

void Mob::TurnTo(csg::Quaternion const& orientation)
{
   NOT_TESTED();
   transform_.Modify([&](csg::Transform& t) {
      t.orientation = orientation;
   });
}

void Mob::TurnToAngle(float angle)
{
   // xxx - resources are currently loaded looking down the positive z
   // axis.  we really want to look down the negative z axix, so just
   // rotate by an additiona 180 degrees.
   angle += 180;
   if (angle >= 360) {
      angle -= 360;
   }
   csg::Quaternion q(csg::Point3f::unitY, angle / 180.0f * csg::k_pi);
   transform_.Modify([&](csg::Transform& t) {
      t.orientation = q;
   });
}

void Mob::TurnToFacePoint(const csg::Point3& location)
{
   csg::Point3f position = GetWorldLocation();
   csg::Point3f v = csg::ToFloat(location) - position;

   float angle = (float)(atan2(v.z, -v.x) - atan2(-1, 0));

   // xxx - resources are currently loaded looking down the positive z
   // axis.  we really want to look down the negative z axix, so just
   // rotate by an additiona 180 degrees.
   angle += csg::k_pi;
   if (angle >= 2 * csg::k_pi) {
      angle -= 2 * csg::k_pi;
   }

   LOG(INFO) << "turning entity " << GetEntity().GetObjectId() << " to face " << location;

   csg::Quaternion q(csg::Point3f::unitY, angle);
   ASSERT(q.is_unit());
   transform_.Modify([&](csg::Transform& t) {
      t.orientation = q;
   });
}

csg::Cube3f Mob::GetWorldAabb() const
{
   return *aabb_ + GetWorldLocation();
}

csg::Point3f Mob::GetLocation() const
{
   return (*transform_).position;
}

csg::Point3f Mob::GetWorldLocation() const
{

   return GetWorldTransform().position;
}

csg::Transform Mob::GetWorldTransform() const
{
   auto parent = (*parent_).lock();
   if (!parent) {
      return GetTransform();
   }

   const csg::Transform& local = GetTransform();
   csg::Transform world = parent->GetWorldTransform();

   world.position += world.orientation.rotate(csg::Point3f(local.position));
   world.orientation *= local.orientation;

   return world;
}

csg::Point3 Mob::GetWorldGridLocation() const
{
   return csg::ToInt(GetWorldLocation());
}

csg::Quaternion Mob::GetRotation() const
{
   return (*transform_).orientation;
}

//Given the orientation of the character, get a location 1 unit in front of him
//TODO: keep testing; not sure if it works from all 360 angles
csg::Point3f Mob::GetLocationInFront() const
{
   NOT_TESTED();
   csg::Quaternion q = (*transform_).orientation;
   return q.rotate(csg::Point3f(0, 0, -1));
}


csg::Point3 Mob::GetGridLocation() const
{
   return csg::ToInt(GetLocation());
}

void Mob::ExtendObject(json::Node const& obj)
{
   LOG(WARNING) << "Expanding mob " << obj.write_formatted();

   SetInterpolateMovement(obj.get<bool>("interpolate_movement", false));
   transform_ = obj.get<csg::Transform>("transform", csg::Transform(csg::Point3f(0, 0, 0), csg::Quaternion(1, 0, 0, 0)));
   
   if (obj.has("parent")) {
      parent_ = ObjectFormatter().GetObject<Mob>(GetStore(), obj.get<std::string>("parent", ""));
   }
}
