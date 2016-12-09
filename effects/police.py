import hyperion, time, colorsys

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 2.0))
colorOne     = hyperion.args.get('color_one', (255,0,0))
colorTwo     = hyperion.args.get('color_two', (0,0,255))
colorsCount  = hyperion.args.get('colors_count', hyperion.ledCount/2)
reverse      = bool(hyperion.args.get('reverse', False))

# Check parameters
rotationTime = max(0.1, rotationTime)
colorsCount = min(hyperion.ledCount/2, colorsCount)

# Initialize the led data
hsv1       = colorsys.rgb_to_hsv(colorOne[0]/255.0, colorOne[1]/255.0, colorOne[2]/255.0)
hsv2       = colorsys.rgb_to_hsv(colorTwo[0]/255.0, colorTwo[1]/255.0, colorTwo[2]/255.0)
colorBlack = (0,0,0)
ledData    = bytearray()
for i in range(hyperion.ledCount):
	if i <= colorsCount:
		rgb = colorsys.hsv_to_rgb(hsv1[0], hsv1[1], hsv1[2])
	elif (i >= hyperion.ledCount/2-1) & (i < (hyperion.ledCount/2) + colorsCount):
		rgb = colorsys.hsv_to_rgb(hsv2[0], hsv2[1], hsv2[2])
	else:
		rgb = colorBlack
	ledData += bytearray((int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))

# Calculate the sleep time and rotation increment
increment = 3
sleepTime = rotationTime / hyperion.ledCount
while sleepTime < 0.05:
	increment *= 2
	sleepTime *= 2
increment %= hyperion.ledCount

# Switch direction if needed
if reverse:
	increment = -increment
	
# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledData)
	ledData = ledData[-increment:] + ledData[:-increment]
	time.sleep(sleepTime)
