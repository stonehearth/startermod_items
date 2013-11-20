from ridl import *

class Env:
   PATH = 'radiant/om/components'

   def lower(self, obj):
      "FooBar -> foo_bar"
      result = ''
      for ch in obj:
         if ch.isupper():
            ch = ch.lower()
            if result:
               result += '_'
         result += ch
      return result

   def upper(self, obj):
      "foo_bar -> FooBar"
      result = ''
      upnext = True
      for ch in obj.strip('_'):
         if ch == '_':
            upnext = True
         elif upnext:
            result += ch.upper()
            upnext = False
         else:
            result += ch
      return result

   def guard_name(self, o):
      guardname = self.PATH + '_' + o.__name__ + "_RIDL_H"
      result = ''
      for ch in guardname:
         if ch.isalnum():
            result += ch.upper()
         else:
            result += '_'
      return result

   
   def properties(self, obj, desired_type):
      desired_type_list = type(desired_type) == type([]) and desired_type or [ desired_type ]
      result = []
      for name in dir(obj):
         attr = getattr(obj, name)
         if type(attr) in desired_type_list:
            result += [(name, attr)]
      return result

