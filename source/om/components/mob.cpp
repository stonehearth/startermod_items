#include "pch.h"
#include "mob.ridl.h"
#include "csg/util.h" // xxx: should be in csg/csg.h

using namespace ::radiant;
using namespace ::radiant::om;

#define M_LOG(level)    LOG(om.mob, level)

std::ostream& operator<<(std::ostream& os, Mob const& o)
{
   return (os << "[Mob]");
}

void Mob::ConstructObject()
{
   Component::ConstructObject();
   transform_ = csg::Transform(csg::Point3f::zero, csg::Quaternion());
   aabb_ = csg::Cube3f::zero;
   interpolate_movement_ = false;
   selectable_ = true;
   moving_ = false;
   mob_collision_type_ = NONE;
}

void Mob::MoveTo(const csg::Point3f& location)
{
   M_LOG(5) << GetEntity() << " moving to " << location;
   transform_.Modify([&](csg::Transform& t) {
      t.position = location;
   });
}

void Mob::MoveToGridAligned(const csg::Point3& location)
{
   MoveTo(csg::ToFloat(location));
}

void Mob::SetRotation(csg::Quaternion const& orientation)
{
   // This seems to be the proper implemenation, but doesn't work at all.  Why?
   // NOT_TESTED();
   transform_.Modify([&](csg::Transform& t) {
      t.orientation = orientation;
   });
}

float Mob::GetFacing() const
{
   csg::Point3f axis;
   float radians;

   (*transform_).orientation.get_axis_angle(axis, radians);

   // this method currently only makes sense for rotations about the y axis
   ASSERT(axis == csg::Point3f::zero || std::abs(std::abs(axis.y) - 1.0f) < csg::k_epsilon);

   return radians / csg::k_pi * 180.0f;
}

void Mob::TurnTo(float angle)
{
   csg::Quaternion q(csg::Point3f::unitY, angle / 180.0f * csg::k_pi);
   transform_.Modify([&](csg::Transform& t) {
      t.orientation = q;
   });
}

void Mob::TurnToFacePoint(const csg::Point3& location)
{
   csg::Point3f position = GetWorldLocation();
   csg::Point3f v = csg::ToFloat(location) - position;
   csg::Point3f forward(0, 0, 1);

   float angle = (float)(atan2(-v.z, v.x) - atan2(-forward.z, forward.x));
   if (angle < 0)  {
      angle += 2 * csg::k_pi;
   }

   M_LOG(5) << "turning entity " << GetEntity().GetObjectId() << " to face " << location;

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
   EntityPtr parent = (*parent_).lock();
   MobPtr mob = nullptr;
   
   if (parent) {
      mob = parent->GetComponent<om::Mob>();
   }
   if (!mob) {
      return GetTransform();
   }

   const csg::Transform& local = GetTransform();
   csg::Transform world = mob->GetWorldTransform();

   world.position += world.orientation.rotate(csg::Point3f(local.position));
   world.orientation *= local.orientation;

   return world;
}

csg::Point3 Mob::GetWorldGridLocation() const
{
   return csg::ToClosestInt(GetWorldLocation());
}

csg::Quaternion Mob::GetRotation() const
{
   return (*transform_).orientation;
}

//Given the orientation of the character, get a location 1 unit in front of him
//TODO: keep testing; not sure if it works from all 360 angles
csg::Point3f Mob::GetLocationInFront() const
{
#if 1
   csg::Quaternion q = (*transform_).orientation;
   csg::Point3f translation = q.rotate(csg::Point3f(0, 0, 1));
   float x = (*transform_).position.x + (float)floor(translation.x + 0.5);
   float z = (*transform_).position.z + (float)floor(translation.z + 0.5);
   return csg::Point3f(x, (*transform_).position.y, z);
#else
   // This seems to be the proper implementation, but doesn't work at all.  Why?
   NOT_TESTED();
   csg::Quaternion q = (*transform_).orientation;
   return q.rotate(csg::Point3f(0, 0, -1));
#endif
}


csg::Point3 Mob::GetGridLocation() const
{
   return csg::ToClosestInt(GetLocation());
}

static std::unordered_map<std::string, Mob::MobCollisionTypes> __str_to_type; // xxx -- would LOVE initializer here..

void Mob::LoadFromJson(json::Node const& obj)
{
   SetInterpolateMovement(obj.get<bool>("interpolate_movement", false));
   transform_ = obj.get<csg::Transform>("transform", csg::Transform(csg::Point3f(0, 0, 0), csg::Quaternion(1, 0, 0, 0)));
   
   if (__str_to_type.empty()) {
      __str_to_type["humanoid"] = HUMANOID;
      __str_to_type["tiny"]  = TINY;
   }

   if (obj.has("parent")) {
      parent_ = GetStore().FetchObject<Entity>(obj.get<std::string>("parent", ""));
   }

   std::string t = obj.get<std::string>("mob_collision_type", "");
   if (!t.empty()) {
      auto i = __str_to_type.find(t);
      if (i != __str_to_type.end()) {
         mob_collision_type_ = i->second;
      }
   }
}

void Mob::SerializeToJson(json::Node& node) const
{
   Component::SerializeToJson(node);

   node.set("transform", GetTransform());
   node.set("entity", GetEntityPtr()->GetStoreAddress());
   node.set("moving", GetMoving());
   node.set("interpolate_movement", GetInterpolateMovement());
   om::EntityPtr parent = GetParent().lock();
   if (parent) {
      node.set("parent", parent->GetStoreAddress());
   }
}

void Mob::SetLocationGridAligned(csg::Point3 const& location)
{
   MoveTo(csg::ToFloat(location));
}

