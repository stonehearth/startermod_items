#!/bin/env python
import os
import sys

def make_decoda_project():
   if len(sys.argv) != 3:
      print 'syntax: %s [name of project] [path to lua root]' % sys.argv[1]
      sys.exit(1)

   _, project_filename, root_path = sys.argv

   project_files = []
   for root, dirs, files in os.walk(root_path):
      for f in files:
         if os.path.splitext(f)[1] == '.lua':
            # decoda wants \, but we get back / since we're in sh...
            path = os.path.join(root, f).replace('/', '\\')
            project_files.append(path)

   if os.path.splitext(project_filename)[1] != '.deproj':
      project_filename += '.deproj'

   outfile = file(project_filename, 'w')
   xml_body = '\n'.join(['<file><filename>%s</filename></file>' % \
                            f for f in project_files])
   xml_file = '<?xml version="1.0" encoding="utf-8"?>\n<project>\n' + \
               xml_body + '</project>\n'

   file(project_filename, 'w').write(xml_file)
   print 'decoda project file %s created!' % project_filename

if __name__ == "__main__":
   make_decoda_project()

