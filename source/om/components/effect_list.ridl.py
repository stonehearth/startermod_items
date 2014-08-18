from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm

class Effect(dm.Record):
   name = 'Effect'

class EffectList(Component):
   next_id = dm.Boxed(c.int(), get=None, set=None, trace=None)
   default_effect_id = dm.Boxed(c.int(), get=None, set=None, trace=None)
   effects = dm.Map(c.int(), std.shared_ptr(Effect()), singular_name="effect", add=None, remove=None, iterate='define')
   add_effect = ridl.Method(std.shared_ptr(Effect()), ('effect_name', std.string().const.ref), ('start_time', c.int()))
   remove_effect = ridl.Method(c.void(), ('effect', std.shared_ptr(Effect())))
   default = dm.Boxed(std.string(), get=None, set=None, trace=None) 
     
   _includes = [ "dm/set.h" ]
   _lua_includes = [ "om/components/effect.ridl.h" ]
   _public = \
   """
   void OnLoadObject(dm::SerializationType r) override;
   """

   _private = \
   """
   void AddRemoveDefault();
   void ConstructObject() override;
   EffectPtr CreateEffect(std::string const& name, int startTime);
   """
