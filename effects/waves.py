import hyperion, time, math, random

randomCenter = bool(hyperion.args.get('random-center', False))
centerX      = float(hyperion.args.get('center_x', -0.15))
centerY      = float(hyperion.args.get('center_y', -0.25))
rotationTime = float(hyperion.args.get('rotation_time', 90))
colors       = hyperion.args.get('colors', ((255,0,0),(255,255,0),(0,255,0),(0,255,255),(0,0,255),(255,0,255)))
reverse      = bool(hyperion.args.get('reverse', False))
reverseTime  = int(hyperion.args.get('reverse_time', 0))
positions    = []

# calc center if random
if randomCenter:
	centerX = random.uniform(0.0, 1.0)
	centerY = random.uniform(0.0, 1.0)

rCenterX   = int(round(float(hyperion.imageWidth())*centerX))
rCenterY   = int(round(float(hyperion.imageHeight())*centerY))

#calc interval
sleepTime = max(1/(255/rotationTime), 0.016)

#calc diagonal
if centerX < 0.5:
	cX = 1.0-centerX
else:
	cX = 0.0+centerX

if centerY < 0.5:
	cY = 1.0-centerY
else:
	cY = 0.0+centerY

diag = int(round(math.hypot(cX*hyperion.imageWidth(),cY*hyperion.imageHeight())))
# some diagonal overhead
diag = int(diag*1.3)

# calc positions
pos = 0
step = int(255/len(colors))
for _ in colors:
	positions.append(pos)
	pos += step

# target time
targetTime = time.time()+float(reverseTime)
while not hyperion.abort():
	# verify reverseTime, randomize reverseTime based on reverseTime up to reversedTime*2
	if reverseTime >= 1:
		now = time.time()
		if now > targetTime:
			reverse = not reverse
			targetTime = time.time()+random.uniform(float(reverseTime), float(reverseTime*2.0))

	# prepare bytearray with colors and positions
	gradientBa = bytearray()
	it = 0
	for color in colors:
		gradientBa += bytearray((positions[it],color[0],color[1],color[2]))
		it += 1

	hyperion.imageRadialGradient(rCenterX,rCenterY, diag, gradientBa,0)
	
	# increment positions
	for i, pos in enumerate(positions):
		if reverse:
			positions[i] = pos - 1 if pos >= 1 else 255
		else:
			positions[i] = pos + 1 if pos <= 254 else 0
	hyperion.imageShow()
	time.sleep(sleepTime)
