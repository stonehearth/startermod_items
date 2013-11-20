from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class Effect(dm.Record):
   name = 'Effect'

class EffectList(Component):
   effects = dm.Set(std.shared_ptr(Effect()), singular_name="effect", add=None, remove=None)
   add_effect = ridl.Method(std.shared_ptr(Effect()), ('effect_name', std.string().const.ref), ('start_time', c.int()))
   remove_effect = ridl.Method(c.void(), ('effect', std.shared_ptr(Effect())))
   
   _includes = [ "dm/set.h" ]