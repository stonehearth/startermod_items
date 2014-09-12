from ridl.om_types import *
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.om_types as om
import ridl.std_types as std
import ridl.csg_types as csg
import ridl.ridl as ridl

# forward declaration...
class Mob(Component):
   name = "Mob"

class Mob(Component):
   axis_alignment_flags = ridl.Enum('Mob', 'AlignToGrid',
      X  = (1 << 0),
      Y  = (1 << 1),
      Z  = (1 << 2),
   )
   mob_collision_types = ridl.Enum('Mob', 'MobCollisionTypes',
      NONE        = 0,
      TINY        = 1,
      HUMANOID    = 2,
      CLUTTER     = 3,
      TITAN       = 4,
   )

   bone = dm.Boxed(std.string())
   parent = dm.Boxed(std.weak_ptr(Entity()))
   transform = dm.Boxed(csg.Transform())
   velocity = dm.Boxed(csg.Transform())
   model_origin = dm.Boxed(csg.Point3f())
   region_origin = dm.Boxed(csg.Point3f())
   align_to_grid_flags = dm.Boxed(c.int())
   interpolate_movement = dm.Boxed(c.bool())
   in_free_motion = dm.Boxed(c.bool())
   mob_collision_type = dm.Boxed(mob_collision_types)

   move_to = ridl.Method(c.void(), ('location', csg.Point3f().const.ref))
   move_to_grid_aligned = ridl.Method(c.void(), ('location', csg.Point3().const.ref))
   turn_to = ridl.Method(c.void(), ('degrees', c.float()))
   turn_to_face_point = ridl.Method(c.void(), ('location', csg.Point3f().const.ref))
   get_facing = ridl.Method(c.float()).const
   set_rotation = ridl.Method(c.void(), ('q', csg.Quaternion().const.ref))
   get_rotation = ridl.Method(csg.Quaternion()).const
   get_location = ridl.Method(csg.Point3f()).const
   get_grid_location = ridl.Method(csg.Point3()).const
   get_world_location = ridl.Method(csg.Point3f(), ('rootEntity', om.EntityRef().ref), no_lua_impl = True).const
   get_world_grid_location = ridl.Method(csg.Point3(), ('rootEntity', om.EntityRef().ref), no_lua_impl = True).const
   get_world_transform = ridl.Method(csg.Transform(), ('rootEntity', om.EntityRef().ref), no_lua_impl = True).const
   get_location_in_front = ridl.Method(csg.Point3f()).const
   set_location_grid_aligned = ridl.Method(c.void(), ('location', csg.Point3().const.ref))

   _generate_construct_object = True
   _includes = [
      "csg/transform.h",
      "csg/point.h",
      "csg/cube.h",
      "dm/dm_save_impl.h",
   ]

   _public = """
      csg::Region3 const& GetMobCollisionRegion() const;
   """

   _lua_impl = """
static bool IsWorldRoot(om::EntityRef entityRef)
{
   om::EntityPtr entity = entityRef.lock();
   if (entity) {
      return entity->GetObjectId() == 1;
   } else {
      return false;
   }
}

// returns nil if the entity is not in the world
luabind::object Mob_GetWorldTransform(lua_State *L, std::weak_ptr<Mob> o)
{
   auto instance = o.lock();
   if (instance) {
      om::EntityRef entityRoot;
      csg::Transform transform = instance->GetWorldTransform(entityRoot);
      if (om::IsRootEntity(entityRoot)) {
         return luabind::object(L, transform);
      } else {
         return luabind::object();
      }
   }
   throw std::invalid_argument("invalid reference in mob::get_world_transform");
}

// returns nil if the entity is not in the world
luabind::object Mob_GetWorldLocation(lua_State *L, std::weak_ptr<Mob> o)
{
   auto instance = o.lock();
   if (instance) {
      om::EntityRef entityRoot;
      csg::Point3f location = instance->GetWorldLocation(entityRoot);
      if (om::IsRootEntity(entityRoot)) {
         return luabind::object(L, location);
      } else {
         return luabind::object();
      }
   }
   throw std::invalid_argument("invalid reference in mob::get_world_location");
}

// returns nil if the entity is not in the world
luabind::object Mob_GetWorldGridLocation(lua_State *L, std::weak_ptr<Mob> o)
{
   auto instance = o.lock();
   if (instance) {
      om::EntityRef entityRoot;
      csg::Point3 location = instance->GetWorldGridLocation(entityRoot);
      if (om::IsRootEntity(entityRoot)) {
         return luabind::object(L, location);
      } else {
         return luabind::object();
      }
   }
   throw std::invalid_argument("invalid reference in mob::get_world_grid_location");
}
   """
   
   _global_post = \
   """
IMPLEMENT_DM_ENUM(radiant::om::Mob::MobCollisionTypes);
   """
