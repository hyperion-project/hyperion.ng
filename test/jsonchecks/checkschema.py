#!/usr/bin/env python
import json, sys, glob
from os import path
from jsonschema import Draft3Validator, RefResolver

print('-- validate json file')

jsonFileName   = sys.argv[1]
schemaFileName = sys.argv[2]

try:
	with open(schemaFileName) as schemaFile:
		with open(jsonFileName) as jsonFile:
			resolver = RefResolver('file://%s/schema/' % path.abspath(path.dirname(schemaFileName)), None)
			Draft3Validator(json.loads(schemaFile.read()), resolver=resolver).validate(json.loads(jsonFile.read()))
except Exception as e:
	print('validation error: '+jsonFileName + ' '+schemaFileName+' ('+str(e)+')')
	sys.exit(1)

sys.exit(0)
