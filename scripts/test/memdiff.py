# Diff two lua memory log snapshots.

import string
import sys

#keep it in KB
def to_scale(scale_str):
   if scale_str == 'B':
      return 0.001
   elif scale_str == 'MB':
      return 1000.0
   return 1.0

def parse_line(line):
   split_str = line.split(':')
   split_str = [s.strip() for s in split_str]
   if split_str[0].startswith('unknown'):
      sizes = split_str[1].strip()
      sizes_split = sizes.split(' ')

      total_size = float(sizes_split[0]) * to_scale(sizes_split[1])
      count = int(sizes_split[2][1:-1])
      return ('unknown', total_size, count)      

   sizes = split_str[3].strip()
   sizes_split = sizes.split(' ')

   total_size = float(sizes_split[0]) * to_scale(sizes_split[1])
   count = 0
   if len(sizes_split) > 2:
      count = int(sizes_split[2][1:-1])

   return (split_str[0] + ':' + split_str[1] + ':' + split_str[2], total_size, count)

def parse_file(filename):
   counts = {}
   f = open(filename, 'r')

   # ignore first two lines
   ls = f.readlines()[2:]

   for l in ls:
      result = parse_line(l)
      counts[result[0]] = (result[1], result[2])
   return counts

def diff_counts(new_counts, old_counts):
   added = {}
   for kind in new_counts:
      if kind not in old_counts:
         added[kind] = new_counts[kind]
   grown = {}
   for kind in new_counts:
      new_count = new_counts[kind]
      if kind in old_counts and new_count[0] > old_counts[kind][0]:
         diff_bytes = new_count[0] - old_counts[kind][0]
         diff_count = new_count[1] - old_counts[kind][1]
         grown[kind] = (diff_bytes, diff_count)
   return (added, grown)

def flatten_and_sort(results):
   flat = [(key, results[key][0],  results[key][1]) for key in results]
   return sorted(flat, key=lambda v: v[1], reverse=True)

def print_formatted(flat):
   max_len = 0
   for v in flat:
      if len(v[0]) > max_len:
         max_len = len(v[0])
   new_bytes = 0

   for v in flat:
      new_bytes = new_bytes + v[1]
      l = string.ljust(v[0], max_len) + '  ' + string.ljust(str(v[1]), 10) + string.ljust(str(v[2]), 5)
      print l
   print new_bytes

new_counts = parse_file(sys.argv[1])
old_counts = parse_file(sys.argv[2])

results = diff_counts(new_counts, old_counts)

flat_added = flatten_and_sort(results[0])
flat_grown = flatten_and_sort(results[1])

print "**********ADDED**********"
print_formatted(flat_added)
print
print "**********GROWN**********"
print_formatted(flat_grown)
