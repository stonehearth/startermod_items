from ridl import *

class Env:
   PATH = 'radiant/om/components'

   def caps(self, obj):
      "FooBar -> FOO_BAR"
      result = ''
      last_digit = False
      for ch in obj:
         is_digit = ch.isdigit()
         is_upper = ch.isupper()
         if result:
            underscore = is_upper and not last_was_upper
            if is_digit != last_was_digit:
               underscore = True
            if underscore:
               result += '_'
         result += ch.upper()
         last_was_digit = is_digit
         last_was_upper = is_upper
      return result

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

