import hyperion, time, colorsys, random, math

# get args
sleepTime  = float(hyperion.args.get('speed', 1.0))/1000.0
saturation = float(hyperion.args.get('saturation', 1.0))
ledData    = bytearray()
ledDataBuf = bytearray()
color_step = []
minStepTime= float(hyperion.latchTime)/1000.0
if minStepTime == 0: minStepTime = 0.001
fadeSteps  = min(256.0, math.floor(sleepTime/minStepTime))
if fadeSteps == 0: fadeSteps = 1

# Initialize the led data
for i in range(hyperion.ledCount):
	ledData += bytearray((0,0,0))
	ledDataBuf += bytearray((0,0,0))
	color_step.append((0.0,0.0,0.0))

# Start the write data loop
while not hyperion.abort():
	for i in range(len(ledData)):
		ledDataBuf[i] = ledData[i]

	for i in range(hyperion.ledCount):
		if random.randrange(10) == 1:
			rgb = colorsys.hsv_to_rgb(random.random(), saturation, random.random())
			ledData[i*3  ] = int(255*rgb[0])
			ledData[i*3+1] = int(255*rgb[1])
			ledData[i*3+2] = int(255*rgb[2])

			color_step[i] = (
				(ledData[i*3  ]-ledDataBuf[i*3  ])/fadeSteps,
				(ledData[i*3+1]-ledDataBuf[i*3+1])/fadeSteps,
				(ledData[i*3+2]-ledDataBuf[i*3+2])/fadeSteps)
		else:
			color_step[i] = (0.0,0.0,0.0)
			
	for step in range(int(fadeSteps)):
		for i in range(hyperion.ledCount):
			ledDataBuf[i*3  ] = min(max(int(ledDataBuf[i*3  ] + color_step[i][0]*float(step)),0),ledData[i*3  ])
			ledDataBuf[i*3+1] = min(max(int(ledDataBuf[i*3+1] + color_step[i][1]*float(step)),0),ledData[i*3+1])
			ledDataBuf[i*3+2] = min(max(int(ledDataBuf[i*3+2] + color_step[i][2]*float(step)),0),ledData[i*3+2])
			
		hyperion.setColor(ledDataBuf)
		time.sleep(sleepTime/fadeSteps)
	hyperion.setColor(ledData)

