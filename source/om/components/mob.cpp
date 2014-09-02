#include "pch.h"
#include "mob.ridl.h"
#include "csg/matrix4.h"
#include "csg/util.h" // xxx: should be in csg/csg.h
#include "csg/cube.h"

using namespace ::radiant;
using namespace ::radiant::om;

#define M_LOG(level)    LOG(om.mob, level)

static csg::Region3 NoCollisionShape;
static csg::Region3 TitanCollisionShape;
static csg::Region3 TrivialCollisionShape;
static csg::Region3 HumanoidCollisionShape;


class ConstructRegions {
public:
   ConstructRegions() {
      TrivialCollisionShape.Add(csg::Cube3::one);
      HumanoidCollisionShape.Add(csg::Cube3(csg::Point3::zero, csg::Point3(1, 3, 1)));
      TitanCollisionShape.Add(csg::Cube3(csg::Point3(-3, 1, -3), csg::Point3(4, 7, 4)));
      TitanCollisionShape.Add(csg::Cube3(csg::Point3(-2, 0, -2), csg::Point3(3, 8, 3)));
   }
};
static ConstructRegions constructRegions;

std::ostream& operator<<(std::ostream& os, Mob const& o)
{
   return (os << "[Mob]");
}

void Mob::ConstructObject()
{
   Component::ConstructObject();
   transform_ = csg::Transform(csg::Point3f::zero, csg::Quaternion());
   velocity_ = csg::Transform(csg::Point3f::zero, csg::Quaternion());
   model_origin_ = csg::Point3f::zero;
   region_origin_ = csg::Point3f::zero;
   align_to_grid_flags_ = 0;
   interpolate_movement_ = false;
   in_free_motion_ = 0;
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

void Mob::TurnToFacePoint(csg::Point3f const& location)
{
   csg::Point3f position = GetWorldLocation();
   csg::Point3f v = location - position;
   csg::Point3f forward(0, 0, -1);

   if (v.x == 0 && v.z == 0) {
      // location and position are at the same XZ location, so keep our current facing.
      return;
   }

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
   MobPtr mob;
   
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
   csg::Point3f const& position = (*transform_).position;
   csg::Quaternion q = (*transform_).orientation;
   return position + q.rotate(csg::Point3f(0, 0, -1));
}


csg::Point3 Mob::GetGridLocation() const
{
   return csg::ToClosestInt(GetLocation());
}

static std::unordered_map<std::string, Mob::MobCollisionTypes> __str_to_type; // xxx -- would LOVE initializer here..
static std::unordered_map<Mob::MobCollisionTypes, std::string> __type_to_str; // xxx -- would LOVE initializer here..

void Mob::LoadFromJson(json::Node const& obj)
{
   SetInterpolateMovement(obj.get<bool>("interpolate_movement", false));
   transform_ = obj.get<csg::Transform>("transform", csg::Transform(csg::Point3f(0, 0, 0), csg::Quaternion(1, 0, 0, 0)));

   // The model_origin defines the position and center of rotation for an entity.
   // The location of the entity (via Mob->GetWorldLocation()) is the location of the model origin.
   // The rotation of the entity is the rotation of the entity around the model_origin.
   model_origin_ = obj.get<csg::Point3f>("model_origin", csg::Point3f::zero);

   // The region_origin defines the position and center of rotation for a region attached to an entity.
   // For 1:1 scale models, this is almost always the same as the model_origin and need not be specified.
   // For scaled models, we use this to align the region to the model, because entity regions must be specified
   // in integer coordinates, and the scaled model usually requires much finer grained positioning.
   region_origin_ = obj.get<csg::Point3f>("region_origin", csg::Point3f::zero);

   int align_to_grid_flags = 0;
   for (json::Node entry : obj.get_node("align_to_grid")) {
      if (entry.type() == JSON_STRING) {
         std::string f = entry.as<std::string>();
         if (f == "x") {
            align_to_grid_flags |= AlignToGrid::X;
         } else if (f == "y") {
            align_to_grid_flags |= AlignToGrid::Y;
         } else if (f == "z") {
            align_to_grid_flags |= AlignToGrid::Z;
         }
      }
   }
   align_to_grid_flags_ = align_to_grid_flags;

   if (__str_to_type.empty()) {
      __str_to_type["humanoid"] = HUMANOID;
      __str_to_type["tiny"]  = TINY;
      __str_to_type["clutter"]  = CLUTTER;
      __str_to_type["titan"]  = TITAN;
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

   if (__type_to_str.empty()) {
      __type_to_str[HUMANOID] = "humanoid";
      __type_to_str[TINY] = "tiny";
      __type_to_str[CLUTTER] = "clutter";
      __type_to_str[TITAN] = "titan";
   }
   node.set("transform", GetTransform());
   node.set("model_origin", GetModelOrigin());
   node.set("region_origin", GetRegionOrigin());
   node.set("axis_alignment_flags", GetAlignToGridFlags());
   node.set("entity", GetEntityPtr()->GetStoreAddress());
   node.set("interpolate_movement", GetInterpolateMovement());
   node.set("in_free_motion", GetInFreeMotion());
   node.set("mob_collision_type", __type_to_str[GetMobCollisionType()]);
   om::EntityPtr parent = GetParent().lock();
   if (parent) {
      node.set("parent", parent->GetStoreAddress());
   }
}

void Mob::SetLocationGridAligned(csg::Point3 const& location)
{
   MoveTo(csg::ToFloat(location));
}

/*
 * -- Mob::GetMobCollisionRegion
 *
 * Return the size of the collision box in the entity's local coordinate system
 * given its mob collision type value.
 *
 */
csg::Region3 const& Mob::GetMobCollisionRegion() const
{
   switch (*mob_collision_type_) {
   case Mob::NONE:
      return NoCollisionShape;
   case Mob::TINY:
   case Mob::CLUTTER:
      return TrivialCollisionShape;
      break;
   case Mob::HUMANOID:
      return HumanoidCollisionShape;
   case Mob::TITAN:
      return TitanCollisionShape;
   }
   NOT_REACHED();
   return NoCollisionShape;
}

