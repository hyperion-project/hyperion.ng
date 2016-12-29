#!/usr/bin/env python
import json, sys, glob
from os import path
print("-- validate json files:")

jsonFiles = sys.argv[1]
schemaFiles = sys.argv[2]

retval  = 0 
total   = 0
errors  = 0
for filename in glob.glob(jsonFiles+'/*.json'):
	with open(filename) as f:
		total += 1
		msg = "   check effect %s ... " % filename
		try:
			j      = json.load(f)
			script = path.basename(j['script'])
			if not path.exists(jsonFiles+'/'+script):
				raise ValueError('script file: '+script+' not found.')
			schema = path.splitext(script)[0]+'.schema.json'
			if not path.exists(jsonFiles+'/schema/'+schema):
				raise ValueError('schema file: '+schema+' not found.')
			print(msg + "ok")
		except ValueError as e:
			print(msg + 'error ('+str(e)+')')
			errors += 1
			retval = 1
			

#print("   checked files: %s success: %s errors: %s" % (total,(total-errors),errors))

sys.exit(retval)
