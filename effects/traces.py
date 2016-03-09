import hyperion
import time
import colorsys
import random

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))

sleepTime = float(hyperion.args.get('speed', 1.0)) * 0.004

runners = [
{ "i":0, "pos":0, "c":0, "step":9 , "lvl":255},
{ "i":1, "pos":0, "c":0, "step":8 , "lvl":255},
{ "i":2, "pos":0, "c":0, "step":7 , "lvl":255},
{ "i":0, "pos":0, "c":0, "step":6 , "lvl":100},
{ "i":1, "pos":0, "c":0, "step":5 , "lvl":100},
{ "i":2, "pos":0, "c":0, "step":4, "lvl":100},
]

# Start the write data loop
while not hyperion.abort():
	for r in runners:
		if r["c"] == 0:
			#ledData[r["pos"]*3+r["i"]] = 0
			r["c"] = r["step"]
			r["pos"] = (r["pos"]+1)%hyperion.ledCount
			ledData[r["pos"]*3+r["i"]] = int(r["lvl"]*(0.2+0.8*random.random()))
		else:
			r["c"] -= 1

	hyperion.setColor(ledData)
	time.sleep(sleepTime)
