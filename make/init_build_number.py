#!/usr/bin/env python

import os
import sys

DEFINES = {
   'bamboo_product_identifier' : ('PRODUCT_IDENTIFIER', 'string'),
   'bamboo_product_name' : ('PRODUCT_NAME', 'string'),
   'bamboo_product_version_major': ('PRODUCT_MAJOR_VERSION', 'number'),
   'bamboo_product_version_minor': ('PRODUCT_MINOR_VERSION', 'number'),
   'bamboo_product_version_patch': ('PRODUCT_PATCH_VERSION', 'number'),
   'bamboo_buildNumber' : ('PRODUCT_BUILD_NUMBER', 'number'),
   'bamboo_buildTimeStamp' : ('PRODUCT_BUILD_TIME', 'number'),
   'bamboo_planRepository_branchName': ('PRODUCT_BRANCH', 'string'),
   'bamboo_repository_revision_number' : ('PRODUCT_REVISION', 'string'),
}

OVERRIDES = {
   'bamboo_product_analytics_game_key' : ('GAME_ANALYTICS_GAME_KEY', 'string'),
   'bamboo_product_analytics_secret_key' : ('GAME_ANALYTICS_SECRET_KEY', 'string'),
   'bamboo_product_analytics_data_api_key' : ('GAME_ANALYTICS_DATA_API_KEY', 'string'),
}

if __name__ == "__main__":
   contents = ""
   contents += "#ifndef _BUILD_OVERRIDES_H\n"
   contents += "#define _BUILD_OVERRIDES_H\n"
   contents += "\n"
   contents += "#include \"build_defaults.h\"\n"
   contents += "\n"

   
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

   pvs = "%s.%s.%s" % (os.environ['bamboo_product_version_major'],
                       os.environ['bamboo_product_version_minor'],
                       os.environ['bamboo_product_version_patch'])

   pfvs = "%s.%s.%s.%s" % (os.environ['bamboo_product_version_major'],
                           os.environ['bamboo_product_version_minor'],
                           os.environ['bamboo_product_version_patch'],
                           os.environ['bamboo_buildNumber'])

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


