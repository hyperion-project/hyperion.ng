import hyperion, time

# get options from args
sleepTime  = float(hyperion.args.get('speed', 1.5)) * 0.005
whiteLevel = int(hyperion.args.get('whiteLevel', 0))
lvl        = int(hyperion.args.get('colorLevel', 220))

# check value
whiteLevel = min( whiteLevel, 254 )
lvl        = min( lvl, 255 )

if whiteLevel >= lvl:
	lvl = 255

# Initialize the led data
ledData = bytearray()
for unused in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))

runners = [
	{ "pos":0, "step": 4, "lvl":lvl},
	{ "pos":1, "step": 5, "lvl":lvl},
	{ "pos":2, "step": 6, "lvl":lvl},
	{ "pos":0, "step": 7, "lvl":lvl},
	{ "pos":1, "step": 8, "lvl":lvl},
	{ "pos":2, "step": 9, "lvl":lvl},
	#{ "pos":0, "step":10, "lvl":lvl},
	#{ "pos":1, "step":11, "lvl":lvl},
	#{ "pos":2, "step":12, "lvl":lvl},
]

# Start the write data loop
counter = 0
while not hyperion.abort():
	counter += 1
	for r in runners:
		if counter % r["step"] == 0:
			ledData[r["pos"]] = whiteLevel
			r["pos"] = (r["pos"]+3) % (hyperion.ledCount*3)
			ledData[r["pos"]] = r["lvl"]

	hyperion.setColor(ledData)
	time.sleep(sleepTime)
