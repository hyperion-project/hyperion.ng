import hyperion, time, colorsys, random

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 3.0))
sleepTime    = float(hyperion.args.get('sleep-time', 0.05))
brightness   = float(hyperion.args.get('brightness', 100))/100.0
saturation   = float(hyperion.args.get('saturation', 100))/100.0
color        = list(hyperion.args.get('color', (255,255,255)))
randomColor  = bool(hyperion.args.get('random-color', False))

# Check parameters
rotationTime = max(0.1, rotationTime)

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
	ledData += bytearray((0, 0, 0))

# Start the write data loop
while not hyperion.abort():
	ledData[:] = bytearray(3*hyperion.ledCount)
	for i in range(hyperion.ledCount):
		if random.random() < 0.005:

			if randomColor:
				rgb = colorsys.hsv_to_rgb(random.random(), saturation, brightness)
				for n in range(3):
					color[n] = int(rgb[n]*255)

			for n in range(3):
				ledData[i*3+n] = color[n]

	hyperion.setColor(ledData)
	time.sleep(sleepTime)
