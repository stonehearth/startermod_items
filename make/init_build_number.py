#!/usr/bin/env python

import os
import sys

DEFINES = {
   'BAMBOO_PRODUCT_IDENTIFIER' : ('PRODUCT_IDENTIFIER', 'string'),
   'BAMBOO_PRODUCT_NAME' : ('PRODUCT_NAME', 'string'),
   'BAMBOO_PRODUCT_VERSION_MAJOR': ('PRODUCT_MAJOR_VERSION', 'number'),
   'BAMBOO_PRODUCT_VERSION_MINOR': ('PRODUCT_MINOR_VERSION', 'number'),
   'BAMBOO_PRODUCT_VERSION_PATCH': ('PRODUCT_PATCH_VERSION', 'number'),
   'BAMBOO_BUILD_NUMBER' : ('PRODUCT_BUILD_NUMBER', 'number'),
   'BAMBOO_BUILD_TIME' : ('PRODUCT_BUILD_TIME', 'number'),
   'BAMBOO_BRANCH_NAME': ('PRODUCT_BRANCH', 'string'),
   'BAMBOO_BRANCH_REVISION' : ('PRODUCT_REVISION', 'string'),
}

OVERRIDES = {
   'BAMBOO_PRODUCT_ANALYTICS_GAME_KEY'     : ('GAME_ANALYTICS_GAME_KEY', 'string'),
   'BAMBOO_PRODUCT_ANALYTICS_SECRET_KEY'   : ('GAME_ANALYTICS_SECRET_KEY', 'string'),
   'BAMBOO_PRODUCT_ANALYTICS_DATA_API_KEY' : ('GAME_ANALYTICS_DATA_API_KEY', 'string'),
   'BAMBOO_REPORT_SYSINFO_URI'             : ('REPORT_SYSINFO_URI', 'string'),
   'BAMBOO_REPORT_CRASHDUMP_URI'           : ('REPORT_CRASHDUMP_URI', 'string'),
}

if __name__ == "__main__":
   contents = ""
   contents += "#ifndef _BUILD_OVERRIDES_H\n"
   contents += "#define _BUILD_OVERRIDES_H\n"
   contents += "\n"
   contents += "#include \"build_defaults.h\"\n"
   contents += "\n"

   
   print >>sys.stderr, 'current environment:'
   for k in sorted(os.environ):
      print >>sys.stderr, '  ', k, '=', os.environ[k]

   for k, (name, t) in DEFINES.iteritems():
      value = os.environ.get(k, None)
      if not value:
         print >>sys.stderr, '%s missing from environment.  aborting' % k
         sys.exit(1)
      contents += "#undef %-30s\n" % name
      if t == 'string':
         contents += "#define %-30s \"%s\"\n" % (name, str(value))
      else:
         contents += "#define %-30s %s\n" % (name, str(value))

   pvs = "%s.%s.%s" % (os.environ['BAMBOO_PRODUCT_VERSION_MAJOR'],
                       os.environ['BAMBOO_PRODUCT_VERSION_MINOR'],
                       os.environ['BAMBOO_PRODUCT_VERSION_PATCH'])

   pfvs = "%s.%s.%s.%s" % (os.environ['BAMBOO_PRODUCT_VERSION_MAJOR'],
                           os.environ['BAMBOO_PRODUCT_VERSION_MINOR'],
                           os.environ['BAMBOO_PRODUCT_VERSION_PATCH'],
                           os.environ['BAMBOO_BUILD_NUMBER'])

   contents += "#undef PRODUCT_VERSION_STR\n"
   contents += "#undef PRODUCT_FILE_VERSION_STR\n"
   contents += "#define %-30s \"%s\"\n" % ("PRODUCT_VERSION_STR", pvs)
   contents += "#define %-30s \"%s\"\n" % ("PRODUCT_FILE_VERSION_STR", pfvs)

   for k, (name, t)  in OVERRIDES.iteritems():
      value = os.environ.get(k, None)
      if value:
         contents += "#undef %-30s\n" % k
         contents += "#define %-30s \"%s\"\n" % (k, str(value))

   contents += "\n"
   contents += "#endif // _BUILD_OVERRIDES_H\n"

   print contents


