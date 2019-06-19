#!/usr/bin/env python
import json, sys

retval   = 0

with open(sys.argv[1]) as f:
	data = json.load(f)
	sys.stdout.write(data['versionnr'])
	sys.exit(0)
