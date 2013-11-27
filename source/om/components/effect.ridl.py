from ridl.om_types import *
import ridl.ridl as ridl
import ridl.c_types as c
import ridl.dm_types as dm
import ridl.std_types as std

class Effect(dm.Record):
   name = dm.Boxed(std.string(), set=None, trace=None)
   start_time = dm.Boxed(c.int(), set=None, trace=None)
   params = dm.Map(std.string(), Selection(), remove=None, get=None, add=None, trace=None)

   _includes = [ "lib/lua/bind.h", "csg/namespace.h", "om/selection.h" ]
   _public = \
   """
   void Init(std::string name, int start);
   void AddParam(std::string param, luabind::object o);
   const Selection& GetParam(std::string param) const;
   """

   _private = \
   """
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);
   """
