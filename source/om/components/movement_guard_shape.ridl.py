from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.luabind_types as luabind

class MovementGuardShape(Component):
   region = dm.Boxed(Region3fBoxedPtr(), trace='deep_region')   
   set_guard_cb = ridl.Method(c.void(), ('unsafe_cb', luabind.object().const.ref))

   _public = \
   """
   bool CanPassThrough(om::EntityPtr const& entity, csg::Point3 const& point);
   """

   _private = \
   """
   luabind::object      _guardCb;
   """

   _includes = [
      "om/region.h",
      "dm/dm_save_impl.h",
   ]
