#!/usr/bin/env python
import json, sys
from os import path
from jsonschema import Draft3Validator, RefResolver

from urllib.parse import urljoin
from urllib.request import pathname2url

def path2url(path):
    return urljoin('file:', pathname2url(path))

print('-- validate json file')

jsonFileName   = sys.argv[1]
schemaFileName = sys.argv[2]

try:
	with open(schemaFileName) as schemaFile:
		with open(jsonFileName) as jsonFile:
			schema = json.load(schemaFile)
			uri = path2url('%s/schema/' % path.abspath(path.dirname(schemaFileName)))
			resolver = RefResolver(uri, referrer = schema)
			instance = json.load(jsonFile)
			Draft3Validator(schema, resolver=resolver).validate(instance)
except Exception as e:
	print('validation error: '+jsonFileName + ' '+schemaFileName+' ('+str(e)+')')
	sys.exit(1)

sys.exit(0)
