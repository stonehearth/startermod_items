import ridl.ridl as ridl
import ridl.c_types as c
import ridl.std_types as std
import ridl.dm_types as dm
from ridl.om_types import *

class RenderInfo(Component):
   scale = dm.Boxed(c.float())
   model_variant = dm.Boxed(std.string())
   animation_table = dm.Boxed(std.string())
   model_mode = dm.Boxed(std.string())
   material = dm.Boxed(std.string())
   attached_entities = dm.Set(EntityRef(), add=None, remove=None, iterate='define', singular_name='attached_entity')

   attach_entity = ridl.Method(c.void(), ('entity', EntityRef()))
   remove_entity = ridl.Method(EntityRef(), ('uri', std.string().const.ref))

   _generate_construct_object = True

   _private = \
   """
   void RemoveFromWorld(EntityPtr entity);
   """
