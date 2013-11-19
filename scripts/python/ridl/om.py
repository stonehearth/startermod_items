class dm:
   class Record(object):
      _generate_construct_object = False
      _private_methods = None
      _public_methods = None
   Record.__name__ = "dm::Record"

   class Boxed(object):
      def __init__(self, type, **kwargs):
         self.get = kwargs.get('get', 'define')
         self.set = kwargs.get('set', 'define')
         self.trace = kwargs.get('trace', 'define')
         self.type = "dm::Boxed<%s>" % type.__name__

   class Set(object):
      def __init__(self, type, **kwargs):
         self.add = kwargs.get('add', 'define')
         self.remove = kwargs.get('remove', 'define')
         self.trace = kwargs.get('trace', 'define')
         self.name = kwargs.get('name', None)
         self.type = "dm::Set<%s>" % type.__name__

   class Map(object):
      def __init__(self, key, value, **kwargs):
         self.get = kwargs.get('get', 'define')
         self.set = kwargs.get('set', 'define')
         self.trace = kwargs.get('trace', 'define')
         self.key = key.__name__
         self.value = value.__name__
         self.type = "dm::Map<%s, %s>" % (self.key, self.value)

class csg:
   class Region3(object):
      pass
   Region3.__name__ = "csg::Region3"

class Entity(object):
   pass

class Selection(object):
   pass

class Component(dm.Record):
   pass
