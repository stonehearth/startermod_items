#!/usr/bin/env python
import os
import sys

root = os.path.split(sys.argv[0])[0]
sys.path = [ root,
             os.path.join(root, 'site-packages', 'Mako-0.9.0-py2.7.egg'),
             os.path.join(root, 'site-packages', 'markupsafe-0.18-py2.7.egg')] + \
           sys.path

from mako.template import Template
from mako.runtime import Context
from mako import exceptions
from StringIO import StringIO
from ridl.env import Env

if __name__ == "__main__":
   if len(sys.argv) != 4:
      print >>sys.stderr, "usage: ridl [outfile] [template_file] [input_file.ridl]"
      sys.exit(1)

   env = Env()
   buf = StringIO()

   arg0, outfile, template_filename, infile = sys.argv
   objname = env.upper(os.path.basename(infile.split('.')[0]))

   execfile(infile)
   obj = globals()[objname]

   ctx = Context(buf, C=obj, env=env)
   t = Template(filename=template_filename)

   try:
      t.render_context(ctx)
   except:
      print exceptions.text_error_template().render()
      sys.exit(1)

   file(outfile, 'wb').write(buf.getvalue())

   
