import hyperion
import time
import colorsys
import random

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))

sleepTime = 0.001

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledData)
	for i in range(hyperion.ledCount):
		if random.randrange(10) == 1:
			hue = random.random()
			sat = 1.0
			val = random.random()
			rgb = colorsys.hsv_to_rgb(hue, sat, val)
			ledData[i*3  ] = int(255*rgb[0])
			ledData[i*3+1] = int(255*rgb[1])
			ledData[i*3+2] = int(255*rgb[2])
	time.sleep(sleepTime)
