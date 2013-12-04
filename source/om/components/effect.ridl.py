from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.luabind_types as luabind

class Effect(dm.Record):
   effect_id = dm.Boxed(c.int(), set=None, trace=None)
   name = dm.Boxed(std.string(), set=None, trace=None)
   start_time = dm.Boxed(c.int(), set=None, trace=None)
   params = dm.Map(std.string(), Selection(), remove=None, get=None, add=None, trace=None)
   add_param = ridl.Method(c.void(), ('name', std.string()), ('o', luabind.object()))

   _includes = [ "lib/lua/bind.h", "csg/namespace.h", "om/selection.h" ]
   _public = \
   """
   void Init(int effect_id, std::string name, int start);
   const Selection& GetParam(std::string param) const;
   """

   _private = \
   """
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);
   """
