#!/usr/bin/env python
import datetime
import os
import subprocess
import sys
import re
import json


def close_test(passed, script, test, time_delta):
   if script['script_name'] != '' and test['name'] != '':
      test['time'] = time_delta.total_seconds()

      if passed:
         script['tests_passed'].append(test)
      else:
         script['tests_failed'].append(test)


def new_script(script_name):
   return { 'script_name':script_name, 'tests_passed':[], 'tests_failed':[] }


def new_test(test_name):
   return { 'name' : test_name, 'time' : 0.0, 'perf' : [] }


def get_log_time(log_time_string):
   return datetime.datetime.strptime(log_time_string.strip(), '%Y-%b-%d %H:%M:%S.%f')


def produce_log_results(log_path):
   results = []

   test_re = re.compile(" running test (\w+) \((\d+) of (\d+)\)")
   script_re = re.compile(" running script ((?:/(?:\w+))+)")
   failure_re = re.compile(" test \"(\w+)\" failed\.  aborting\.")
   perf_re = re.compile(" Perf\:(\d+\.\d+)\:(\d+)")
   
   f = open(log_path, 'r')

   current_script = new_script('')
   current_test = new_test('')
   test_start = datetime.datetime.fromtimestamp(0)

   for line in f:
      split_line = line.split('|')

      if len(split_line) < 4:
         continue

      current_time = get_log_time(split_line[0])
      actual_log = split_line[3]
      
      r = script_re.match(actual_log)
      if r:
         # Matches a new script starting.
         close_test(True, current_script, current_test, current_time - test_start)
         current_test = new_test('')
         current_script = new_script(r.group(1))
         results.append(current_script)
         continue

      r = test_re.match(actual_log)
      if r:
         # Matches a new test starting
         close_test(True, current_script, current_test, current_time - test_start)
         current_test = new_test(r.group(1))
         test_start = current_time
         continue

      r = perf_re.match(actual_log)
      if r:
         dt = int((current_time - test_start).total_seconds() * 1000)
         current_test['perf'].append([dt, float(r.group(1)), int(r.group(2))])
         continue

      r = failure_re.match(actual_log)
      if r:
         # Matches a failed test.
         close_test(False, current_script, current_test, current_time - test_start)
         current_script = new_script('')
         current_test = new_test('')

   # TODO: fix the last time!  This involves fixing some ugly logging....
   if current_test['name'] != '':
      close_test(True, current_script, current_test, datetime.timedelta(0))
   f.close()
   return results


def write_log_results(data, result_file_path, file_name):   
   try:
      os.mkdir(result_file_path)
   except OSError:
      # Dir probably already exists....
      pass

   f = open(result_file_path + file_name, 'w')
   d = json.dumps(data, indent=2, separators=(',', ': '))
   f.write(d)
   f.close()


def run_tests():
   sh_command = sh_exe_path + ' ' + sh_args

   return_code = 1

   print 'Running Stonehearth autotests with exe at ' + sh_exe_path + '....'

   return_code = subprocess.call(sh_command, cwd=sh_cwd)

   print 'Completed in ' + str((datetime.datetime.now() - t_start).total_seconds()) + ' seconds.'

   return return_code

t_start = datetime.datetime.now()

if os.environ.has_key('RADIANT_ROOT'):
  rad_root = os.environ['RADIANT_ROOT'] + '/'
else:
  rad_root = '../'

sh_root = rad_root + 'stonehearth/'

if '-d' in sys.argv:
  build_type = 'Debug'
else:
  build_type = 'RelWithDebInfo'
  
sh_build_root = sh_root + 'build/'
sh_cwd = sh_root + 'source/stonehearth_data/'
sh_exe_path = sh_build_root + 'source/stonehearth/' + build_type + '/Stonehearth.exe'

if '-o' in sys.argv:
   # In an official build, look elsewhere for our cwd/exe.
   sh_cwd = './'
   sh_build_root = './'
   sh_exe_path = './Stonehearth.exe'

sh_args = '--game.main_mod=stonehearth_autotest --simulation.game_speed=4 --logging.log_level=1'

if '-f' in sys.argv and '-sc' in sys.argv:
   sh_args += ' --mods.stonehearth_autotest.options.script=' + sys.argv[sys.argv.index('-sc') + 1]
   sh_args += ' --mods.stonehearth_autotest.options.function=' + sys.argv[sys.argv.index('-f') + 1]
elif '-sc' in sys.argv:
   sh_args += ' --mods.stonehearth_autotest.options.script=' + sys.argv[sys.argv.index('-sc') + 1]
elif '-g' in sys.argv:
   sh_args += ' --mods.stonehearth_autotest.options.group=' + sys.argv[sys.argv.index('-g') + 1]
else:
   sh_args += ' --mods.stonehearth_autotest.options.group=all'

if '-i' in sys.argv:
  print 'Running in interactive mode'
else:
  sh_args += ' --renderer.minimized=true'

if '-s' in sys.argv:
   sh_args += sys.argv[sys.argv.index('-s') + 1]

return_code = run_tests()

if return_code != 0:
   print 'Autotests failed; check stonehearth.log for more information.'
else:
   print 'Autotests passed!  Yay!'

write_log_results(produce_log_results(sh_cwd + 'stonehearth.log'), sh_build_root + 'test_output/', 'results.shtest.json')

sys.exit(return_code)
