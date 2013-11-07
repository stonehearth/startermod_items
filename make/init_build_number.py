#!/usr/bin/env python

import os
import sys

DEFINES = {
   'PRODUCT_IDENTIFIER' : 'string',
   'PRODUCT_NAME' : 'string',
   'PRODUCT_MAJOR_VERSION' : 'number',
   'PRODUCT_MINOR_VERSION' : 'number',
   'PRODUCT_PATCH_VERSION' : 'number',
   'PRODUCT_BUILD_NUMBER' : 'number',
   'PRODUCT_BRANCH' : 'string',
   'PRODUCT_REVISION' : 'string',
}

OVERRIDES = {
   'GAME_ANALYTICS_GAME_KEY' : 'string',
   'GAME_ANALYTICS_SECRET_KEY' : 'string',
   'GAME_ANALYTICS_DATA_API_KEY' : 'string',
}

if __name__ == "__main__":
   contents = ""
   contents += "#ifndef _BUILD_OVERRIDES_H\n"
   contents += "#define _BUILD_OVERRIDES_H\n"
   contents += "\n"
   contents += "#include \"build_defaults.h\"\n"
   contents += "\n"

   
   for k, t in DEFINES.iteritems():
      value = os.environ.get(k, None)
      if not value:
         print >>sys.stderr, '%s missing from environment.  aborting' % k
         sys.exit(1)
      contents += "#undef %-30s\n" % k
      if t == 'string':
         contents += "#define %-30s \"%s\"\n" % (k, str(value))
      else:
         contents += "#define %-30s %s\n" % (k, str(value))

   for k, t  in OVERRIDES.iteritems():
      value = os.environ.get(k, None)
      if value:
         contents += "#undef %-30s\n" % k
         contents += "#define %-30s \"%s\"\n" % (k, str(value))

   contents += "\n"
   contents += "#endif _BUILD_OVERRIDES_H\n"

   print contents


