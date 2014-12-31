#!/bin/env python
import os
import sys
import json

def check_mod():
   if len(sys.argv) != 2:
      print 'syntax: %s [path to mod]' % sys.argv[0]
      sys.exit(1)

   _, root_path = sys.argv

   exitcode = 0
   print 'checking', root_path
   for root, dirs, files in os.walk(root_path):
      for f in files:
         if os.path.splitext(f)[1] == '.json':
            path = os.path.join(root, f)
            try:
               json.loads(file(path).read())
            except Exception, err:
               print path, 'is not json:'
               print '   ', str(err)
               print
               exitcode = 1

   return exitcode

if __name__ == "__main__":
   sys.exit(check_mod())

