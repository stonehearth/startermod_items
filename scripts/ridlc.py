#!/usr/bin/env python
import os
import sys

root = os.path.split(sys.argv[0])[0]
sys.path = [ os.path.join(root, 'python/site-packages/Mako-0.9.0-py2.7.egg'),
             os.path.join(root, 'python/site-packages/markupsafe-0.18-py2.7.egg')] + sys.path

from mako.template import Template
from mako.runtime import Context
from StringIO import StringIO

class prop(object):
   def __init__(self, Type, **kwargs):
      self.get = kwargs.get('get', True)
      self.set = kwargs.get('set', True)
      self.trace = kwargs.get('trace', True)
      self.type = Type.__name__

class Entity(object):
   pass

class EntityRef(object):
   pass

class Component(object):
   pass

class CarryBlock(Component):
   carry_block = prop(EntityRef)


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


   def ref(self, name):
      return name + "Ref";
   
   def guard_name(self, o):
      guardname = self.PATH + '_' + o.__name__ + "_H"
      result = ''
      for ch in guardname:
         if ch.isalnum():
            result += ch.upper()
         else:
            result += '_'
      return result


   def properties(self, obj):
      result = []
      for name in dir(obj):
         attr = getattr(obj, name)
         if type(attr) == prop:
            result += [(name, attr)]
      return result

buf = StringIO()
ctx = Context(buf, C=CarryBlock, env=Env())
#t = Template(filename='foo.py')

t = Template(TEMPLATE)
t.render_context(ctx)
print buf.getvalue()


