import hyperion, time

# Get the parameters
fadeInTime     = float(hyperion.args.get('fade-in-time', 2000)) / 1000.0
fadeOutTime    = float(hyperion.args.get('fade-out-time', 2000)) / 1000.0
colorStart     = hyperion.args.get('color-start', (255,174,11))
colorEnd       = hyperion.args.get('color-end', (0,0,0))
colorStartTime = float(hyperion.args.get('color-start-time', 1000)) / 1000
colorEndTime   = float(hyperion.args.get('color-end-time', 1000)) / 1000
repeat         = hyperion.args.get('repeat-count', 0)
maintainEndCol = hyperion.args.get('maintain-end-color', True)
minStepTime    = float(hyperion.latchTime)/1000.0
if minStepTime == 0: minStepTime = 0.001
currentR = currentG = currentB = 0

# create color table for fading from start to end color
steps = float(max(abs(colorEnd[0] - colorStart[0]),max(abs(colorEnd[1] - colorStart[1]),abs(colorEnd[2] - colorStart[2]))))
color_step = (0,0,0)

if steps == 0:
	steps = 1
else:
	color_step = (
		(colorEnd[0] - colorStart[0]) / steps,
		(colorEnd[1] - colorStart[1]) / steps,
		(colorEnd[2] - colorStart[2]) / steps
	)

calcChannel = lambda i: min(max(int(round(colorStart[i] + color_step[i]*step)),0), colorEnd[i] if colorStart[i] < colorEnd[i] else colorStart[i])
colors = []
for step in range(int(steps)+1):
	colors.append( (calcChannel(0),calcChannel(1),calcChannel(2)) )

# calculate timings
if fadeInTime>0:
	incrementIn  = max(1,int(round(steps / (fadeInTime / minStepTime) )))
	sleepTimeIn  = fadeInTime / (steps / incrementIn)
else:
	incrementIn  = sleepTimeIn  = 1
	
if fadeOutTime>0:
	incrementOut = max(1,int(round(steps / (fadeOutTime / minStepTime) )))
	sleepTimeOut = fadeOutTime / (steps / incrementOut)
else:
	incrementOut  = sleepTimeOut  = 1

def setColor(r,g,b):
	global currentR,currentG,currentB
	
	currentR = r
	currentG = g
	currentB = b
	hyperion.setColor(r,g,b)

# loop
repeatCounter = 1
while not hyperion.abort():
	# fade in
	if fadeInTime > 0:
		setColor( colors[0][0],colors[0][1],colors[0][2] )
		for step in range(0,int(steps)+1,incrementIn):
			if hyperion.abort(): break
			setColor( colors[step][0],colors[step][1],colors[step][2] )
			time.sleep(sleepTimeIn)

	# end color
	t = 0.0
	while t<colorStartTime and not hyperion.abort():
		setColor( colors[int(steps)][0],colors[int(steps)][1],colors[int(steps)][2] )
		time.sleep(minStepTime)
		t += minStepTime

	# fade out
	if fadeOutTime > 0:
		setColor( colors[int(steps)][0],colors[int(steps)][1],colors[int(steps)][2] )
		for step in range(int(steps),-1,-incrementOut):
			if hyperion.abort(): break
			setColor( colors[step][0],colors[step][1],colors[step][2] )
			time.sleep(sleepTimeOut)

	# start color
	t = 0.0
	while t<colorEndTime and not hyperion.abort():
		setColor( colors[0][0],colors[0][1],colors[0][2] )
		time.sleep(minStepTime)
		t += minStepTime

	# repeat
	if repeat > 0 and repeatCounter >= repeat : break
	repeatCounter += 1

time.sleep(0.5)

# maintain end color until effect end
while not hyperion.abort() and maintainEndCol:
	hyperion.setColor( currentR, currentG, currentB )
	time.sleep(1)
	
