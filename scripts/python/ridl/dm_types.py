import ridl
import std_types as std
import csg_types as csg

class ObjectId(ridl.Type):
   name = "dm::ObjectId"

class Record(ridl.Class):
   name = "dm::Record"

class Boxed(ridl.Type):
   def __init__(self, value, **kwargs):
      self.get = kwargs.get('get', 'define')
      self.set = kwargs.get('set', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.value = value
      self.name = "dm::Boxed<%s>" % value.name
      self.short_name = "boxed"
      self.trace_category = kwargs.get('trace_category', 'LUA_ASYNC_TRACES')
      self.trace_name = "dm::BoxedTrace<%s>" % self.name

class Set(ridl.Type):
   def __init__(self, value, **kwargs):
      self.add = kwargs.get('add', 'define')
      self.remove = kwargs.get('remove', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.iterate = kwargs.get('iterate', None)
      self.num = kwargs.get('num', 'define')
      self.singular_name = kwargs.get('singular_name', None)
      self.value = value
      self.accessor_value = kwargs.get('accessor_value', self.value)
      self.name = "dm::Set<%s>" % value.name
      self.short_name = "set"
      self.trace_category = kwargs.get('trace_category', 'LUA_ASYNC_TRACES')
      self.trace_name = "dm::SetTrace<%s>" % self.name

class Array(ridl.Type):
   def __init__(self, value, count, **kwargs):
      self.set = kwargs.get('set', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.iterate = kwargs.get('iterate', None)
      self.singular_name = kwargs.get('singular_name', None)
      self.value = value
      self.name = "dm::Array<%s, %d>" % (value.name, count)
      self.short_name = "array"
      self.trace_category = kwargs.get('trace_category', 'LUA_ASYNC_TRACES')
      self.trace_name = "dm::ArrayTrace<%s>" % self.name

class Map(object):
   def __init__(self, key, value, **kwargs):
      self.key = key
      self.value = value
      self.add = kwargs.get('add', 'define')
      self.remove = kwargs.get('remove', 'define')
      self.get = kwargs.get('get', 'define')
      self.num = kwargs.get('num', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.iterate = kwargs.get('iterate', None)
      self.singular_name = kwargs.get('singular_name', None)
      self.accessor_value = kwargs.get('accessor_value', self.value)
      if hasattr(self.key, 'hash') and self.key.hash:
         self.name = "dm::Map<%s, %s, %s>" % (self.key.name, self.value.name, self.key.hash)
      else:
         self.name = "dm::Map<%s, %s>" % (self.key.name, self.value.name)
      self.short_name = "map"
      self.trace_category = kwargs.get('trace_category', 'LUA_ASYNC_TRACES')
      self.trace_name = "dm::MapTrace<%s>" % self.name
