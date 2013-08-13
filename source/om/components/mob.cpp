#include "pch.h"
#include "mob.h"

using namespace ::radiant;
using namespace ::radiant::om;

void Mob::Describe(std::ostringstream& os) const
{
   os << "pos:" << GetLocation();
}

void Mob::InitializeRecordFields()
{
   Component::InitializeRecordFields();
   AddRecordField("transform", transform_);
   AddRecordField("aabb", aabb_);
   AddRecordField("flags", flags_);
   AddRecordField("parent", parent_);
   AddRecordField("moving", moving_);

   transform_.Modify().set_zero();
   aabb_.Modify().set_zero();
   flags_ = (INTERPOLATE_MOVEMENT | SELECTABLE);
}

void Mob::MoveTo(const math3d::point3& location)
{
   LOG(INFO) << "moving entity " << GetEntity().GetObjectId() << " to " << location;
   transform_.Modify().position = location;
}

void Mob::MoveToGridAligned(const math3d::ipoint3& location)
{
   MoveTo(math3d::point3(location));
}

void Mob::TurnTo(const math3d::quaternion& orientation)
{
   LOG(INFO) << "turning entity " << GetEntity().GetObjectId() << " to " << orientation;
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
   LOG(INFO) << "turning entity " << GetEntity().GetObjectId() << " to angle " << angle;
   math3d::quaternion q(math3d::vector3::unit_y, angle / 180.0f * k_pi);
   transform_.Modify().orientation = q;
}

void Mob::TurnToFacePoint(const math3d::ipoint3& location)
{
   math3d::point3 position = GetWorldLocation();
   math3d::point3 v = math3d::point3(location) - position;

   float angle = math3d::atan2(v.z, -v.x) - math3d::atan2(-1, 0);

   // xxx - resources are currently loaded looking down the positive z
   // axis.  we really want to look down the negative z axix, so just
   // rotate by an additiona 180 degrees.
   angle += k_pi;
   if (angle >= k_two_pi) {
      angle -= k_two_pi;
   }

   LOG(INFO) << "turning entity " << GetEntity().GetObjectId() << " to face " << location;

   math3d::quaternion q(math3d::vector3::unit_y, angle);
   ASSERT(q.is_unit());
   transform_.Modify().orientation = q;
}

math3d::aabb Mob::GetWorldAABB() const
{
   return *aabb_ + GetWorldLocation();
}

math3d::aabb Mob::GetAABB() const
{
   return aabb_;
}

const math3d::point3 Mob::GetLocation() const
{
   return (*transform_).position;
}

math3d::point3 Mob::GetWorldLocation() const
{

   return GetWorldTransform().position;
}

math3d::transform Mob::GetWorldTransform() const
{
   auto parent = (*parent_).lock();
   if (!parent) {
      return GetTransform();
   }

   const math3d::transform& local = GetTransform();
   math3d::transform world = parent->GetWorldTransform();

   world.position += world.orientation.rotate(math3d::vector3(local.position));
   world.orientation *= local.orientation;

   return world;
}

math3d::ipoint3 Mob::GetWorldGridLocation() const
{
   return math3d::ipoint3(GetWorldLocation());
}

const math3d::quaternion& Mob::GetRotation() const
{
   return (*transform_).orientation;
}

const math3d::transform& Mob::GetTransform() const
{
   return *transform_;
}

math3d::ipoint3 Mob::GetGridLocation() const
{
   return math3d::ipoint3(GetLocation());
}

bool Mob::InterpolateMovement() const
{
   return ((*flags_) & INTERPOLATE_MOVEMENT) != 0;
}

bool Mob::IsSelectable() const
{
   return ((*flags_) & SELECTABLE) != 0;
}

void Mob::SetInterpolateMovement(bool value)
{
   if (value) {
      flags_ = (*flags_) | INTERPOLATE_MOVEMENT;
   } else {
      flags_ = (*flags_) & ~INTERPOLATE_MOVEMENT;
   }
}

void Mob::SetParent(MobPtr parent)
{
   parent_ = parent;
}

void Mob::ExtendObject(json::ConstJsonObject const& obj)
{
   JSONNode const& node = obj.GetNode();
   auto i = node.find("interpolate_movement");
   if (i != node.end()) {
      SetInterpolateMovement(i->as_bool());
   }
}
