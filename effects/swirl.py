import hyperion, time, math, random

# set minimum image size - must be done asap
hyperion.imageMinSize(64,64)
iW = hyperion.imageWidth()
iH = hyperion.imageHeight()

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 8.0))
reverse      = bool(hyperion.args.get('reverse', False))
centerX      = float(hyperion.args.get('center_x', 0.5))
centerY      = float(hyperion.args.get('center_y', 0.5))
randomCenter = bool(hyperion.args.get('random-center', False))
custColors	 = hyperion.args.get('custom-colors', ((255,0,0),(0,255,0),(0,0,255)))

if randomCenter:
	centerX = random.uniform(0.0, 1.0)
	centerY = random.uniform(0.0, 1.0)

minStepTime  = float(hyperion.latchTime)/1000.0
sleepTime = max(0.1, rotationTime) / 360
angle     = 0
centerX   = int(round(float(iW)*centerX))
centerY   = int(round(float(iH)*centerY))
increment = -1 if reverse else 1

# adapt sleeptime to hardware
if minStepTime > sleepTime:
	increment *= int(math.ceil(minStepTime / sleepTime))
	sleepTime  = minStepTime

# table of stop colors for rainbow gradient, first is the position, next rgba, all values 0-255
rainbowColors = bytearray()

if len(custColors) > 1:
	posfac = int(255/len(custColors))
	pos = 0
	for c in custColors:
		pos += posfac
		rainbowColors += bytearray([pos,c[0],c[1],c[2],255])

	lC = custColors[-1]
	rainbowColors += bytearray([0,lC[0],lC[1],lC[2],255])
	
else:
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
	])

# effect loop
while not hyperion.abort():
	angle += increment
	if angle > 360: angle=0
	if angle <   0: angle=360
	
	hyperion.imageCanonicalGradient(centerX, centerY, angle, rainbowColors)

	hyperion.imageShow()
	time.sleep(sleepTime)