#!/bin/env python

import subprocess
import sys
import json
import os
import shutil
import ctypes

try:
   import Image
except:
   print
   print 'Please install PIL from http://www.pythonware.com/products/pil/#pil117'
   print
   sys.exit(1)


MAKE_TEMPLATES = [
   '../../build/' + os.environ['RADIANT_BUILD_PLATFORM'] + '/source/stonehearth/RelWithDebInfo/Stonehearth.exe',
   '--game.main_mod=stonehearth_templates',
   '--renderer.screen_width=1920',
   '--renderer.screen_height=1080',
]


def run_command(*args):
   cmd = ' '.join(args)
   print 'running', cmd
   #os.system(' '.join(args))
   p = subprocess.Popen(cmd, cwd='.')
   p.wait()

def remove_existing_templates():
   for root, dirs, files in os.walk('saved_objects/stonehearth/building_templates'):
      for f in files:
         print 'removing exisiting template', f
         os.unlink(os.path.join(root, f))

def rewrite_png(infile, outfile):
   size = 800
   xpadding = (1920 - size) / 2;
   ypadding = (1080 - size) / 2;
   im = Image.open(infile)
   region = im.crop((xpadding, ypadding, 1920 - xpadding, 1080 - ypadding)) # left, upper, right, lower
   region.save(outfile)

def rewrite_templates():
   for root, dirs, files in os.walk('saved_objects/stonehearth/building_templates'):
      for f in files:
         print 'processing template', f

         # load the template file.
         template_file = os.path.join(root, f)
         with file(template_file) as infile:
            template = json.loads(infile.read())

         # read the screenshot key out of the header and move it into the 
         # saved objects directory.  then delete the key
         preview_image = os.path.join(root, os.path.splitext(f)[0] + '.png')
         header = template['header']
         screenshot = header['screenshot']
         del header['screenshot']

         rewrite_png(screenshot, preview_image)

         # add the preview_image key to point to the moved screenshot, then rewrite
         # the json file
         header['preview_image'] = '/r/' + preview_image.replace('\\', '/')
         with file(template_file, 'w') as outfile:
            print 'rewriting', template_file, 'to include preview image'
            outfile.write(json.dumps(template, indent=3))

if __name__ == "__main__":
   remove_existing_templates()
   
   # move the mouse out of the way...
   ctypes.windll.user32.SetCursorPos(0, 0)
   run_command(*MAKE_TEMPLATES)
   rewrite_templates()

