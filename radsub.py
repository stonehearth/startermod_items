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
sh_args = '--game.main_mod=stonehearth_autotest --simulation.game_speed=4 --logging.log_level=1'
if '-i' in sys.argv:
  print 'Running in interactive mode'
else:
  sh_args += ' --renderer.minimized=true'

sh_command = sh_exe_path + ' ' + sh_args

return_code = 1

print 'Running Stonehearth autotests with exe at ' + sh_exe_path + '....'

return_code = subprocess.call(sh_command, cwd=sh_cwd)

print 'Completed in ' + str((datetime.datetime.now() - t_start).total_seconds()) + ' seconds.'

if return_code != 0:
  print 'Autotests failed; check stonehearth.log for more information.'
else:
  print 'Autotests passed!  Yay!'

sys.exit(return_code)
