#include "pch.h"
#include "mob.h"
#include "csg/util.h" // xxx: should be in csg/csg.h

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

   transform_.Modify().SetZero();
   aabb_.Modify().SetZero();
   flags_ = (INTERPOLATE_MOVEMENT | SELECTABLE);
   if (!IsRemoteRecord()) {
      moving_ = false;
   }
}

void Mob::MoveTo(const csg::Point3f& location)
{
   LOG(INFO) << "moving entity " << GetEntity().GetObjectId() << " to " << location;
   transform_.Modify().position = location;
}

void Mob::MoveToGridAligned(const csg::Point3& location)
{
   MoveTo(csg::ToFloat(location));
}

void Mob::TurnTo(const csg::Quaternion& orientation)
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
   csg::Quaternion q(csg::Point3f::unitY, angle / 180.0f * csg::k_pi);
   transform_.Modify().orientation = q;
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
   transform_.Modify().orientation = q;
}

csg::Cube3f Mob::GetWorldAABB() const
{
   return *aabb_ + GetWorldLocation();
}

csg::Cube3f Mob::GetAABB() const
{
   return aabb_;
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

csg::Transform Mob::GetTransform() const
{
   return *transform_;
}

csg::Point3 Mob::GetGridLocation() const
{
   return csg::ToInt(GetLocation());
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

void Mob::ExtendObject(json::Node const& obj)
{
   LOG(WARNING) << "Expanding mob " << obj.write_formatted();

   SetInterpolateMovement(obj.get<bool>("interpolate_movement", false));
   transform_ = obj.get<csg::Transform>("transform", csg::Transform(csg::Point3f(0, 0, 0), csg::Quaternion(1, 0, 0, 0)));
   
   if (obj.has("parent")) {
      parent_ = ObjectFormatter().GetObject<Mob>(GetStore(), obj.get<std::string>("parent", ""));
   }
}

Mob& Mob::SetMoving(bool m)
{
   moving_ = m;
   return *this;
}

Mob& Mob::SetTransform(csg::Transform const& t)
{
   transform_ = t;
   return *this;
}

