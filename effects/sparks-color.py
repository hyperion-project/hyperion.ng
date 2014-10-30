import hyperion
import time
import colorsys
import random

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 3.0))
brightness = float(hyperion.args.get('brightness', 1.0))
saturation = float(hyperion.args.get('saturation', 1.0))
reverse = bool(hyperion.args.get('reverse', False))

# Check parameters
rotationTime = max(0.1, rotationTime)
brightness = max(0.0, min(brightness, 1.0))
saturation = max(0.0, min(saturation, 1.0))

# Initialize the led data
ledData = bytearray()

sleepTime = 0.05

# Start the write data loop
while not hyperion.abort():
	ledData[:] = bytearray(3*hyperion.ledCount)
	for i in range(hyperion.ledCount):
		if random.random() < 0.005:
			hue = random.random()
			sat = 1
			val = 1
			rgb = colorsys.hsv_to_rgb(hue, sat, val)
			ledData[i*3]   = int(255*rgb[0])
			ledData[i*3+1] = int(255*rgb[1])
			ledData[i*3+2] = int(255*rgb[2])
	hyperion.setColor(ledData)
	time.sleep(sleepTime)
