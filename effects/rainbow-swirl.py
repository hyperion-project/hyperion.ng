import hyperion, time

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 3.0))
reverse      = bool(hyperion.args.get('reverse', False))
centerX      = float(hyperion.args.get('center_x', 0.5))
centerY      = float(hyperion.args.get('center_y', 0.5))

# Check parameters
rotationTime = max(0.1, rotationTime)
angle = 0

# Calculate the sleep time and rotation increment
increment = 3
sleepTime = rotationTime / 360
while sleepTime < 0.05:
	increment *= 2
	sleepTime *= 2
increment %= 360

# table of stop colors for rainbow gradient
rainbowColors = bytearray([
	0  ,255,0  ,0,
	25 ,255,230,0,
	63 ,255,255,0,
	100,0  ,255,0,
	127,0  ,255,200,
	159,0  ,255,255,
	191,0  ,0  ,255,
	224,255,0  ,255,
	255,255,0  ,127,
])

if reverse:
	increment = -increment

while not hyperion.abort():
	angle += increment
	if angle > 360: angle=0
	if angle <   0: angle=360

	hyperion.imageCanonicalGradient(0,0,
		hyperion.imageWidth,hyperion.imageHeight,
		int(round(hyperion.imageWidth)*centerX),int(round(float(hyperion.imageHeight)*centerY)),
		angle,rainbowColors
	);

	hyperion.imageShow()
	time.sleep(sleepTime)
