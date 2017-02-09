#!/usr/bin/env python
import json, sys, glob
from os import path
from jsonschema import Draft3Validator

print('-- validate json file')

jsonFileName   = sys.argv[1]
schemaFileName = sys.argv[2]

try:
	with open(schemaFileName) as schemaFile:
		with open(jsonFileName) as jsonFile:
			j = json.load(jsonFile)
			validator = Draft3Validator(json.load(schemaFile))
			validator.validate(j)
except Exception as  e:
	print('validation error: '+jsonFileName + ' '+schemaFileName+' ('+str(e)+')')
	sys.exit(1)

sys.exit(0)
