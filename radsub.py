#!/usr/bin/env python
import datetime
import os
import subprocess
import sys

t_start = datetime.datetime.now()

if os.environ.has_key('RADIANT_ROOT'):
  rad_root = os.environ['RADIANT_ROOT']
else:
  rad_root = '..'

sh_cwd = rad_root + '/stonehearth/source/stonehearth_data/'
sh_exe_path = rad_root + '/stonehearth/build/source/stonehearth/RelWithDebInfo/Stonehearth.exe'
sh_args = '--game.main_mod=stonehearth_autotest --simulation.game_speed=3 --logging.log_level=0 --logging.mods.stonehearth.ai=spam --renderer.minimized=true'

sh_command = sh_exe_path + ' ' + sh_args

return_code = 1

print 'Stashing uncommitted files....'

# Make sure we run our tests with other changes stashed. NOTE: this doesn't stash untracked 
# changes, because unstashing untracked changes requires that you delete those files in your
# tree.
if subprocess.call('git stash --keep-index') != 0:
  print 'Error in git stash; aborting submit.'
  sys.exit(1)

try:
  print 'Running Stonehearth autotests with exe at ' + sh_exe_path + '....'

  return_code = subprocess.call(sh_command, cwd=sh_cwd)

  print 'Completed in ' + str((datetime.datetime.now() - t_start).total_seconds()) + ' seconds.'

  if return_code != 0:
    print 'Autotests failed; check stonehearth.log for more information.'
  else:
    print 'Autotests passed!  Yay!'

  print 'Unstashing files....'
finally:
  subprocess.call('git stash pop --quiet --index')

sys.exit(return_code)