#!/bin/env python
import os
import sys
import json
from pprint import pprint

def check_campaigns():
   if len(sys.argv) != 2:
      print 'syntax: %s [path to lua root]' % sys.argv[1]
      sys.exit(1)

   _, root_path = sys.argv

   root_path = root_path + "/stonehearth/data/gm/campaigns"

   enemy_spawning_campaigns=["city_raid", "raid_stockpiles", "pillage", "create_mission"]

   campaign_files = []
   for root, dirs, files in os.walk(root_path):
      for f in files:
         if os.path.splitext(f)[1] == '.json':
            # decoda wants \, but we get back / since we're in sh...
            path = os.path.join(root, f)
            campaign_files.append(path)
            with open(path) as data_file:    
               data = json.load(data_file)
               if data['type'] == "encounter":
                  if data['encounter_type'] in enemy_spawning_campaigns:
                     if not _check_all_keys_recursive(data, "sighted_bulletin"):
                        print 'enemy spawning campaign has no sighted_bulletin: %s' % f

   print 'check campaigns complete!'

def _check_all_keys_recursive(json, key_to_check):
   for key, value in json.items():
      if key == key_to_check:
         return True
      if isinstance(value, dict):
         if _check_all_keys_recursive(value, key_to_check):
            return True

   return False

if __name__ == "__main__":
   check_campaigns()
