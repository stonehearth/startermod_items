import ridl
import dm_types as dm
import std_types as std

class Entity(ridl.Type):
   name = "Entity"

class Selection(ridl.Type):
   name = "Selection"

class Component(dm.Record):
   name = "Component"
   _lua_weak_ref = True

class Region2Boxed(ridl.Type):
   name = "Region2Boxed"

class Region3Boxed(ridl.Type):
   name = "Region3Boxed"

class Region2BoxedPtr(ridl.Type):
   name = "Region2Boxed"

class Region3BoxedPtr(ridl.Type):
   name = "Region3BoxedPtr"

def EntityRef():
   return std.weak_ptr(Entity())
   _lua_weak_ref = True

def EntityPtr():
   return std.shared_ptr(Entity())
