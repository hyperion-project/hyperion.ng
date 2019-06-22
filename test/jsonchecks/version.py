#!/usr/bin/env python
import json, sys

retval   = 0

with open(sys.argv[1]) as f:
	if len(sys.argv) < 3:
		data = json.load(f)
		sys.stdout.write(data['versionnr'])
		sys.exit(0)
	else:
		data = json.load(f)
		sys.stdout.write(data['channel'])
		sys.exit(0)
