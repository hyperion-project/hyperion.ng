#!/usr/bin/env python
import json, sys

print("-- validate json files")

retval  = 0 
total   = 0
errors  = 0
for filename in sys.argv[1:]:
	with open(filename) as f:
		total += 1
		msg = "   check json %s ... " % filename
		try:
			json.load(f)
			#print(msg + "ok")
		except ValueError as e:
			print(msg + 'invalid')
			retval = 1
			errors += 1

print("   checked files: %s success: %s errors: %s" % (total,(total-errors),errors))

sys.exit(retval)
