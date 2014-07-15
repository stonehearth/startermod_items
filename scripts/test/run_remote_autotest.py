#!/usr/bin/env python

import base64
import cookielib
import json
import mimetools, mimetypes
import os
import stat
import sys
import urllib
import urllib2
from cStringIO import StringIO
from mimetools import choose_boundary
from threading import Thread, enumerate
from time import sleep

class Callable:
    def __init__(self, anycallable):
        self.__call__ = anycallable

# Controls how sequences are uncoded. If true, elements may be given multiple values by
#  assigning a sequence.
doseq = 1

class MultipartPostHandler(urllib2.BaseHandler):
    handler_order = urllib2.HTTPHandler.handler_order - 10 # needs to run first

    def http_request(self, request):
        data = request.get_data()
        if data is not None and type(data) != str:
            v_files = []
            v_vars = []
            try:
                 for(key, value) in data.items():
                     if type(value) == file:
                         v_files.append((key, value))
                     else:
                         v_vars.append((key, value))
            except TypeError:
                systype, value, traceback = sys.exc_info()
                raise TypeError, "not a valid non-string sequence or mapping object", traceback

            if len(v_files) == 0:
                data = urllib.urlencode(v_vars, doseq)
            else:
                boundary, data = self.multipart_encode(v_vars, v_files)
                contenttype = 'multipart/form-data; boundary=%s' % boundary
                if(request.has_header('Content-Type')
                   and request.get_header('Content-Type').find('multipart/form-data') != 0):
                    print "Replacing %s with %s" % (request.get_header('content-type'), 'multipart/form-data')
                request.add_unredirected_header('Content-Type', contenttype)

            request.add_data(data)
        return request


    def multipart_encode(vars, files, boundary = None, buffer = None):
        if boundary is None:
            boundary = mimetools.choose_boundary()
        if buffer is None:
            buffer = StringIO()
        for(key, value) in vars:
            buffer.write('--%s\r\n' % boundary)
            buffer.write('Content-Disposition: form-data; name="%s"' % key)
            if value is None:
                value = ""
            buffer.write('\r\n\r\n' + value + '\r\n')
        for(key, fd) in files:
            file_size = os.fstat(fd.fileno())[stat.ST_SIZE]
            filename = fd.name.split('/')[-1]
            contenttype = mimetypes.guess_type(filename)[0] or 'application/octet-stream'
            buffer.write('--%s\r\n' % boundary)
            buffer.write('Content-Disposition: form-data; name="%s"; filename="%s"\r\n' % (key, filename))
            buffer.write('Content-Type: %s\r\n' % contenttype)
            buffer.write('Content-Length: %s\r\n' % file_size)
            fd.seek(0)
            buffer.write('\r\n' + base64.b64encode(fd.read()) + '\r\n')
        buffer.write('--' + boundary + '--\r\n')
        buffer = buffer.getvalue()
        return boundary, buffer
    multipart_encode = Callable(multipart_encode)

    https_request = http_request

UPDATE_INTERVAL = 1


#NOTE: doesn't handle arrays yet.
def flatten_json_to_args(json):
   def flatten_recursive(node, current_path):
      all_args = ''
      for key in node:
         value = node[key]
         if type(value) is dict:
            path = ''
            if current_path != '':
               path = current_path + '.' + key
            else:
               path = key
            all_args = all_args + flatten_recursive(value, path)
         else:
            str_value = str(value)
            if type(value) == bool:
               str_value = str_value.lower()
            all_args = all_args + ' --' + current_path + '.' + key + '=' + str_value
      return all_args
   return flatten_recursive(json, '')


class TestThread(Thread):
   def __init__(self, url, machine_name, test_group, test_script, test_function, file_name, settings={}):
      super(TestThread, self).__init__()
      self.url = url
      self.machine_name = machine_name
      self.file_name = file_name
      self.test_group = test_group
      self.test_script = test_script
      self.test_function = test_function
      self.response = None
      self.settings = flatten_json_to_args(settings)

   def run(self):
      # This should probably be factored, but then this script is also specific to
      # Stonehearth, so for now, might as well be the One Source of Truth.
      radsubArgs = "radsub.py -o -i"
      if self.test_function:
         radsubArgs += " -f " + self.test_function
      if self.test_script:
         radsubArgs += " -s " + self.test_script
      if self.test_group:
         radsubArgs += " -g " + self.test_group

      if self.settings != '':
         radsubArgs += ' -- ' + self.settings

      metadata_json = """
      {
         "WorkingDirectory" : "stonehearth",
         "ExecutableFile" : "python",
         "ArtifactsName" : "stonehearth/test_output/results.shtest.json",
         "ExecutableArgs" : "%s"
      }
      """ % radsubArgs

      cookies = cookielib.CookieJar()
      opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cookies),
                                    MultipartPostHandler)
      params = { 
         "metadata" : metadata_json,
         "file" : open(self.file_name, "rb")
      }

      self.response = opener.open(self.url, params).read()



def read_json_file(file_path):
   f = open(file_path, 'r')
   result = json.loads(f.read())
   f.close()
   return result


def run_perf_tests(slave_json, test_group, test_script, test_func, configs_json, config_script, file_location, timeout=60 * 60):
   def alive_count(lst):
      alive = map(lambda x : 1 if x.isAlive() else 0, lst)
      return reduce(lambda a,b : a + b, alive)

   if config_script:
      configs_json = {config_script : configs_json[config_script]}
   raw_results = []
   for config_name in configs_json:
      config = configs_json[config_name]
      used_slaves = [(slave_json['machines'][machine_name]['ip_address'], machine_name) for machine_name in config['machines'] ]

      if len(used_slaves) == 0:
        continue

      threads = [ TestThread('http://' + slave[0] + ':8086/run/', slave[1], test_group, test_script, test_func, file_location, config['settings']) for slave in used_slaves ]
      
      for thread in threads:
         thread.start()

      while alive_count(threads) > 0 and timeout > 0.0:
         timeout = timeout - UPDATE_INTERVAL
         sleep(UPDATE_INTERVAL)

      raw_results.extend([ (x.response, x.machine_name, config['settings']) for x in threads ])
   return raw_results


def write_perf_results_to_json_file(results, output_file):
   combined_results = []

   # Flatten all of our results!  Our list of results can contain multiple runs from the same machine,
   # differing only by the settings used.
   for result in results:
      if result[0] != None:
         all_test_data = json.loads(result[0])

         for test_data in all_test_data:
            script_name = test_data['script_name']

            for test_run in test_data['tests_passed']:

               new_result = {
                  'machine_name' : result[1], 
                  'settings' : result[2], 
                  'perf_data' : test_run['perf'],
                  'script_name' : script_name,
                  'test_name' : test_run['name']
               }
               combined_results.append(new_result)

   f = open(output_file, 'w')
   f.write(json.dumps(combined_results, indent=2, separators=(',', ': ')))
   f.close()


def run_tests(slaves, test_group, file_location, timeout=60 * 60):
   def alive_count(lst):
      alive = map(lambda x : 1 if x.isAlive() else 0, lst)
      return reduce(lambda a,b : a + b, alive)

   threads = [ TestThread('http://' + slave[0] + ':8086/run/', slave[1], test_group, None, None, file_location) for slave in slaves ]
   
   for thread in threads:
      thread.start()

   while alive_count(threads) > 0 and timeout > 0.0:
      timeout = timeout - UPDATE_INTERVAL
      sleep(UPDATE_INTERVAL)

   return [ (x.response, x.machine_name) for x in threads ]


def write_results_to_json_file(results, output_file):
   combined_results = {}

   for result in results:
      if result[0] != None:
         combined_results[result[1]] = json.loads(result[0])

   f = open(output_file, 'w')
   f.write(json.dumps(combined_results, indent=2, separators=(',', ': ')))
   f.close()


if os.environ.has_key('RADIANT_ROOT'):
  rad_root = os.environ['RADIANT_ROOT'] + '/'
else:
  rad_root = '../'

sh_root = rad_root + 'stonehearth/'
test_script_root = sh_root + 'scripts/test/'
build_root = sh_root + 'build/'

slave_json = read_json_file(test_script_root + './tester_slaves.json')
machines_json = slave_json['machines']

# Default to everything (does not include performance tests!)
test_group = 'all'
test_script = None
test_func = None

if '-g' in sys.argv:
   test_group = sys.argv[sys.argv.index('-g') + 1]
if '-s' in sys.argv:
   test_script = sys.argv[sys.argv.index('-s') + 1]
if '-f' in sys.argv:
   test_func = sys.argv[sys.argv.index('-f') + 1]

file_location = build_root + 'test-package/stonehearth-test.zip'

if '-p' in sys.argv:
   config_script = None
   if '-c' in sys.argv:
      config_script = sys.argv[sys.argv.index('-c') + 1]
   # Performance auto-tests.
   config_json = read_json_file(test_script_root + './perf_configs.json')
   results = run_perf_tests(slave_json, test_group, test_script, test_func, config_json, config_script, file_location)
   output_file = build_root + 'combined_results.shperf.json'
   write_perf_results_to_json_file(results, output_file)
else:
   slaves = [ (machines_json[machine_name]['ip_address'], machine_name) for machine_name in slave_json['machines'] ]

   # If not explicitly stated, just run on one machine.  For now, pick machine 0.
   if not '-a' in sys.argv:
      slaves = [slaves[0]]
   # Normal autotests
   results = run_tests(slaves, test_group, file_location)
   output_file = build_root + 'combined_results.shtest.json'
   write_results_to_json_file(results, output_file)
