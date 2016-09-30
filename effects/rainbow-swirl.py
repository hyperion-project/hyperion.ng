import hyperion, time

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 3.0))
reverse      = bool(hyperion.args.get('reverse', False))
centerX      = float(hyperion.args.get('center_x', 0.5))
centerY      = float(hyperion.args.get('center_y', 0.5))

sleepTime = max(0.1, rotationTime) / 360
angle     = 0
centerX   = int(round(hyperion.imageWidth)*centerX)
centerY   = int(round(float(hyperion.imageHeight)*centerY))
increment = -1 if reverse else 1

# table of stop colors for rainbow gradient, first is the position, next rgb, all values 0-255
rainbowColors = bytearray([
	0  ,255,0  ,0, 255,
	25 ,255,230,0, 255,
	63 ,255,255,0, 255,
	100,0  ,255,0, 255,
	127,0  ,255,200, 255,
	159,0  ,255,255, 255,
	191,0  ,0  ,255, 255,
	224,255,0  ,255, 255,
	255,255,0  ,127, 255,
	#0, 255, 0, 0, 255,
	#42, 255, 255, 0, 255,
	#85, 0, 255, 0, 255,
	#128, 0, 255, 255, 255,
	#170, 0, 0, 255, 255,
	#212, 255, 0, 255, 255,
	#255, 255, 0, 0, 255,
])

# effect loop
while not hyperion.abort():
	angle += increment
	if angle > 360: angle=0
	if angle <   0: angle=360

	hyperion.imageCanonicalGradient(centerX, centerY, angle, rainbowColors)

	hyperion.imageShow()
	time.sleep(sleepTime)
