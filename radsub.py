#!/usr/bin/env python
import datetime
import os
import subprocess
import sys

t_start = datetime.datetime.now()

sh_cwd = os.environ['RADIANT_ROOT'] + '/stonehearth/source/stonehearth_data/'
sh_exe_path = os.environ['RADIANT_ROOT'] + '/stonehearth/build/source/stonehearth/RelWithDebInfo/Stonehearth.exe'
sh_args = '--game.main_mod=stonehearth_autotest --simulation.game_speed=3 --logging.log_level=0 --logging.mods.stonehearth.ai=spam --renderer.minimized=true'

sh_command = sh_exe_path + " " + sh_args

print 'Stashing uncommitted files....'

subprocess.call('git stash --keep-index')

print 'Running Stonehearth autotests with exe at ' + sh_exe_path + '....'

return_code = subprocess.call(sh_command, cwd=sh_cwd)

print 'Completed in ' + str((datetime.datetime.now() - t_start).total_seconds()) + ' seconds.'

if return_code != 0:
  print "Autotests failed; check stonehearth.log for more information."
else:
  print 'Autotests passed!  Yay!'

print 'Unstashing files....'

subprocess.call('git stash pop --quiet --index')

sys.exit(return_code)