#!/usr/bin/env python
import argparse
import datetime
import json
import os
import re
import subprocess
import sys
import ctypes
from time import sleep

SEM_NOGPFAULTERRORBOX = 0x0002 # From MSDN


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
   s = log_time_string.strip()
   return datetime.datetime.strptime(s, '%Y-%b-%d %H:%M:%S.%f')


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

   print 'Running Stonehearth autotests with command ' + sh_command

   # Disable that annoying crash diagnosis window on Windows.
   ctypes.windll.kernel32.SetErrorMode(SEM_NOGPFAULTERRORBOX);

   sp = subprocess.Popen(sh_command, cwd=sh_cwd)

   wait_time = 0
   return_code = sp.poll()
   while wait_time < timeout and return_code == None:
      return_code = sp.poll()
      sleep(1)
      wait_time = wait_time + 1

   if return_code == None:
      print 'Timeout.  Terminating test.'
      return_code = 1
      sp.kill()
   else:
      print 'Completed in ' + str((datetime.datetime.now() - t_start).total_seconds()) + ' seconds.'

   return return_code

parser = argparse.ArgumentParser(description='Stonehearth Test Runner.')
parser.add_argument('-d', '--debug', action='store_true', help='runs the tests on the debug version of Stonehearth (local builds only)')
parser.add_argument('-o', '--official', action='store_true', help='runs the tests assuming the directory structure of an official build')
parser.add_argument('-f', '--function', help='runs the specified test function (script must be set using --sc!)')
parser.add_argument('-s', '--script', help='runs the specified test script')
parser.add_argument('-g', '--group', help='runs the specified test group')
parser.add_argument('-i', '--interactive', action='store_true', help='run in interactive mode (maximizes the window)')
parser.add_argument('-t', '--timeout', help='Sets a timeout for the test-running process')
parser.add_argument('settings', nargs=argparse.REMAINDER, help='settings to pass to Stonehearth (e.g. --mods.foo.bar=7 --mods.foo.baz=bing)')

args = parser.parse_args(sys.argv[1:])

t_start = datetime.datetime.now()

if os.environ.has_key('RADIANT_ROOT'):
  rad_root = os.environ['RADIANT_ROOT'] + '/'
else:
  rad_root = '../'

sh_root = rad_root + 'stonehearth/'

if args.debug:
  build_type = 'Debug'
else:
  build_type = 'RelWithDebInfo'
  
sh_build_root = sh_root + 'build/'
sh_cwd = sh_root + 'source/stonehearth_data/'
sh_exe_path = sh_build_root + 'source/stonehearth/' + build_type + '/Stonehearth.exe'

if args.official:
   # In an official build, look elsewhere for our cwd/exe.
   sh_cwd = './'
   sh_build_root = './'
   sh_exe_path = './Stonehearth.exe'

sh_args = '--game.main_mod=stonehearth_autotest --simulation.game_speed=4 --logging.log_level=0'

if args.function and args.script:
   sh_args += ' --mods.stonehearth_autotest.options.script=' + args.script
   sh_args += ' --mods.stonehearth_autotest.options.function=' + args.function
elif args.script:
   sh_args += ' --mods.stonehearth_autotest.options.script=' + args.script
elif args.group:
   sh_args += ' --mods.stonehearth_autotest.options.group=' + args.group
else:
   sh_args += ' --mods.stonehearth_autotest.options.group=all'

timeout = int(args.timeout)
if not timeout:
   timeout = 10 * 60

if not args.interactive:
  sh_args += ' --renderer.minimized=true'

if len(args.settings) > 1:
   sh_args += ' ' + reduce(lambda x,y: x + ' ' + y, args.settings[1:])

return_code = run_tests()

if return_code != 0:
   print 'Autotests failed; check stonehearth.log for more information.'
else:
   print 'Autotests passed!  Yay!'

write_log_results(produce_log_results(sh_cwd + 'stonehearth.log'), sh_build_root + 'test_output/', 'results.shtest.json')

sys.exit(return_code)
