class Type(object):
   def __init__(self):
      self._attrs = []

   def __str__(self):
      return self.name

   def _add_const(self):
      self.name += ' const'
      return self
   const = property(_add_const)

   def _add_reference(self):
      self.name += '&'
      return self
   ref = property(_add_reference)

class Method(object):
   def __init__(self, result, *args, **options):
      self.result = result
      self.args = args
      self.postsig = ''
      self.no_lua_impl = options.get('no_lua_impl', False)

   def _add_const(self):
      self.postsig += ' const'
      return self
   const = property(_add_const)

   def _add_override(self):
      self.postsig += ' override'
      return self
   override = property(_add_override)

   def _get_formal_params(self):
      args = []
      for name, type in self.args:
         args += [ "%s %s" % (type, name) ]
      return ', '.join(args)
   formal_params = property(_get_formal_params)

   def _get_actual_params(self):
      return ', '.join(a[0] for a in self.args)
   actual_params = property(_get_actual_params)

class Class(Type):
   _generate_construct_object = False
   _public = None
   _private = None
   _protected = None
   _global_post = ""
   _includes = []
   _no_lua = False
   _lua_weak_ref = False
   _lua_includes = []
   _lua_impl = None

class Enum(Type):
   def __init__(self, clsname, name, **kwargs):
      Type.__init__(self)  
      self.values = [(k, kwargs[k]) for k in kwargs]
      self.values.sort(key=lambda x: x[1]) # sort by value
      self.name = '%s::%s' % (clsname, name)
      self.internal_name = name

   def __getattr__(self, name):
      for k, v in self.values:
         if k == name:
            return v
      raise AttributeError
