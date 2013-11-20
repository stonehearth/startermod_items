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

class Set(ridl.Type):
   def __init__(self, value, **kwargs):
      self.add = kwargs.get('add', 'define')
      self.remove = kwargs.get('remove', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.singular_name = kwargs.get('singular_name', None)
      self.value = value
      self.name = "dm::Set<%s>" % value.name

class Array(ridl.Type):
   def __init__(self, value, count, **kwargs):
      self.set = kwargs.get('set', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.singular_name = kwargs.get('singular_name', None)
      self.value = value
      self.name = "dm::Array<%s, %d>" % (value.name, count)

class Map(object):
   def __init__(self, key, value, **kwargs):
      self.insert = kwargs.get('insert', 'define')
      self.remove = kwargs.get('remove', 'define')
      self.get = kwargs.get('get', 'define')
      self.trace = kwargs.get('trace', 'define')
      self.singular_name = kwargs.get('singular_name', None)
      self.key = key
      self.value = value
      if hasattr(self.key, 'hash') and self.key.hash:
         self.name = "dm::Map<%s, %s, %s>" % (self.key.name, self.value.name, self.key.hash)
      else:
         self.name = "dm::Map<%s, %s>" % (self.key.name, self.value.name)
