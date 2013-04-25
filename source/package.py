import os
import shutil
import zipfile

PACKAGE_FILE = 'stonehearth-package.zip'
STAGE_DIR = '..\\build\\stage-install'


def run(cmd):
   print cmd
   os.system(cmd)

def make_dir(dir):
   if not os.path.isdir(dir):
      print 'creating stage directory', dir
      os.makedirs(dir)

def stage_file(srcfile, dstdir):
   srcfile = srcfile.replace('/', '\\')
   dstdir = dstdir.replace('/', '\\')

   path, f = os.path.split(srcfile)
   dstdir = os.path.join(STAGE_DIR, dstdir)
   dstfile = os.path.join(dstdir, f)
   make_dir(os.path.join(dstdir))
   print 'staging file', dstfile
   if '*' in f:
      run('copy "%s" "%s"' % (srcfile, dstdir))
   else:
      run('copy "%s" "%s"' % (srcfile, dstfile))

def stage_dir(dir, dst):
   dir = dir.replace('/', '\\')
   dst = dst.replace('/', '\\')
   for root, dirs, files in os.walk(dir):
      relroot = root[len(dir)+1:]
      for f in files:
         stage_file(os.path.join(root, f), os.path.join(dst, relroot))

def make_zip(name):
   if os.path.isfile(name):
      print 'removing old', name
      os.unlink(name)
   print 'creating', name
   return zipfile.Zipfile(name)

make_dir(STAGE_DIR)
stage_dir('stonehearth_data/scripts', 'scripts')
stage_dir('stonehearth_data/cursors', 'cursors')
stage_dir('stonehearth_data/resources/radiant', 'resources/radiant')
stage_dir('stonehearth_data/horde',   'horde')
stage_dir('stonehearth_data/models',  'models')
stage_file('stonehearth_data/*.json', '')
stage_file('../build/client_app/RelWithDebInfo/client_app.exe', '')
stage_file('../build/client_app/RelWithDebInfo/client_app.pdb', '')
stage_file('../build/client_app/RelWithDebInfo/icudt.dll', '')
stage_file('../build/client_app/RelWithDebInfo/libcef.dll', '')
stage_file('../build/client_app/RelWithDebInfo/lua51.dll', '')
stage_file('../build/client_app/RelWithDebInfo/*.pak', '.')
stage_dir('../build/client_app/RelWithDebInfo/locales', 'locales')
stage_dir('c:\\users\\ponder\\documents\\github\\stonehearth-ui\\docroot', 'ui')

package = make_zip(PACKAGE_FILE)
