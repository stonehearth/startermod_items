#!/usr/bin/env python
import os
import sys
import subprocess

GIT = 'C:\\Program Files (x86)\\Git\\bin\\git.exe'

def shell(*args):
   print 'running', ' '.join(*args)
   return subprocess.Popen(*args, shell=True)

def run(*args):
   print 'running', ' '.join(*args)
   return subprocess.Popen(*args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def git(*args):
   return run([GIT] + list(args))

def last_build_file(submodule):
   return os.path.join(submodule, '.last_build_' + os.getenv('RADIANT_BUILD_PLATFORM'))

def build_submodule(sha1, submodule):
   """
   build the submodule. if successful, write the sha1 hash to .last_build.
   """
   if os.path.isfile(os.path.join(submodule, 'Makefile')):
      print ' * building submodule %s.' % submodule
      name = os.path.split(submodule)[-1]
      p = shell(['make', name+'-module'])
      #for line in iter(p.stdout.readline, ''):
      #   print line

      p.wait()
      if p.returncode !=0:
         sys.exit(p.returncode)
   else:
      print ' * ignoring submodule %s (no Makefile).' % submodule
      
   print ' * writing sha1 hash to %s.' % last_build_file(submodule)
   file(last_build_file(submodule), 'w').write(sha1)

   """
   hash = subprocess.Popen('git rev-parse --verify HEAD', 
                            shell=True,
                            stdout=subprocess.PIPE).communicate()[0]
   """

def get_last_build_sha1(submodule):
   """
   get the sha1 hash of the submodule the last time it as built.
   """
   last_built_hash = ''
   try: 
      last_built_hash = file(last_build_file(submodule), 'r').read()
   except:
      pass
   return last_built_hash

def check_submodule_flag(flag, sha1, submodule):
   if flag:
      if flag == '-':
         print ' ! submodule %s is not initialized' % submodule
         sys.exit(1)
      elif flag == '+':
         print ' ! submodule %s does not match index' % submodule 
      elif flag == 'U':
         print ' ! submodule %s has merge conflicts!' % submodule 
      else:
         print ' ! submodule %s has unknown flag \'%s\'' % (submodule, flag)

def check_submodule(flag, sha1, submodule):
   """
   make sure the submodule is in a good state.  this includes being properly
   synced up and built since the last time it was syncd
   """
   check_submodule_flag(flag, sha1, submodule)

   last_build_sha1 = get_last_build_sha1(submodule)
   if sha1 == last_build_sha1:
      print ' * submodule %s is up-to-date.' % submodule
   else:
      build_submodule(sha1, submodule)

def check_submodules():
   status = git('submodule', 'status').communicate()[0]
   for line in status.split('\n'):
      tokens = line.strip().split(' ')
      if len(tokens) > 1:
         sha1, submodule = tokens[:2]
         flag = sha1[0]
         if not flag.isalnum():
            sha1 = sha1[1:]
         else:
            flag = None
         check_submodule(flag, sha1, submodule)

check_submodules()

