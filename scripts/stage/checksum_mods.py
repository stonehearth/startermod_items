#!/bin/env python
import os
import sys
import json
import hashlib

def checksum(filename):
   sha256 = hashlib.sha256()
   with file(filename, 'rb') as f:
      sha256.update(f.read())
   return sha256.hexdigest()


if __name__ == "__main__":
   stage_path = sys.argv[1]
   checksums = {}
   for root, dirs, files in os.walk(os.path.join(stage_path, 'mods')):
      for filename in files:
         modname = os.path.splitext(filename)[0]
         checksums[modname] = checksum(os.path.join(root, filename))

   config_filename = os.path.join(stage_path, 'stonehearth.json')

   with file(config_filename) as f:
      config = json.load(f)

   config['mod_checksums'] = checksums

   with file(config_filename, 'w') as f:
      f.write(json.dumps(config, indent=3))

