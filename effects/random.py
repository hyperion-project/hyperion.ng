import hyperion, time, colorsys, random

# get args
sleepTime  = float(hyperion.args.get('speed', 1.0))
saturation = float(hyperion.args.get('saturation', 1.0))
ledData    = bytearray()

# Initialize the led data
for i in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledData)
	for i in range(hyperion.ledCount):
		if random.randrange(10) == 1:
			rgb = colorsys.hsv_to_rgb(random.random(), saturation, random.random())
			ledData[i*3  ] = int(255*rgb[0])
			ledData[i*3+1] = int(255*rgb[1])
			ledData[i*3+2] = int(255*rgb[2])
	time.sleep(sleepTime)
