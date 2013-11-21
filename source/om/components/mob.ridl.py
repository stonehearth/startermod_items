from ridl.om_types import *
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.csg_types as csg
import ridl.ridl as ridl

# forward declaration...
class Mob(Component):
   name = "Mob"

class Mob(Component):
   parent = dm.Boxed(std.weak_ptr(Mob))
   transform = dm.Boxed(csg.Transform)
   aabb = dm.Boxed(csg.Cube3f)
   moving = dm.Boxed(c.bool())
   interpolate_movement = dm.Boxed(c.bool())
   selectable = dm.Boxed(c.bool())

   move_to = ridl.Method(c.void(), ('location', csg.Point3f().const.ref))
   move_to_grid_aligned = ridl.Method(c.void(), ('location', csg.Point3().const.ref))
   turn_to = ridl.Method(c.void(), ('q', csg.Quaternion().const.ref))
   turn_to_angle = ridl.Method(c.void(), ('degrees', c.float()))
   turn_to_face_point = ridl.Method(c.void(), ('location', csg.Point3().const.ref))
   get_world_aabb = ridl.Method(csg.Cube3f()).const
   get_rotation = ridl.Method(csg.Quaternion()).const
   get_location = ridl.Method(csg.Point3f()).const
   get_location = ridl.Method(csg.Point3f()).const
   get_grid_location = ridl.Method(csg.Point3()).const
   get_world_location = ridl.Method(csg.Point3f()).const
   get_world_grid_location = ridl.Method(csg.Point3()).const
   get_world_transform = ridl.Method(csg.Transform()).const
   get_location_in_front = ridl.Method(csg.Point3f()).const

   _generate_construct_object = True
   _includes = [
      "csg/transform.h",
      "csg/point.h",
      "csg/cube.h"
   ]