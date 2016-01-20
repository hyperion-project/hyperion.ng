import hyperion
import time
import colorsys
import random

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))

sleepTime = 0.005

runners = [
{ "pos":0, "step":4 , "lvl":255},
{ "pos":1, "step":5 , "lvl":255},
{ "pos":2, "step":6 , "lvl":255},
{ "pos":0, "step":7 , "lvl":255},
{ "pos":1, "step":8 , "lvl":255},
{ "pos":2, "step":9,  "lvl":255},
]

# Start the write data loop
count = 0
while not hyperion.abort():
	count += 1
	for r in runners:
		if count%r["step"] == 0:
			ledData[r["pos"]] = 0
			r["pos"] = (r["pos"]+3)%(hyperion.ledCount*3)
			ledData[r["pos"]] = r["lvl"]

	hyperion.setColor(ledData)
	time.sleep(sleepTime)
