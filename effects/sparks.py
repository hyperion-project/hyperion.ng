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
for i in range(hyperion.ledCount):
	ledData += bytearray((0, 0, 0))

sleepTime = 0.05

# Start the write data loop
while not hyperion.abort():
	ledData[:] = bytearray(3*hyperion.ledCount)
	for i in range(hyperion.ledCount):
		if random.random() < 0.005:
			ledData[i*3] = 255
			ledData[i*3+1] = 255
			ledData[i*3+2] = 255
	hyperion.setColor(ledData)
	time.sleep(sleepTime)
