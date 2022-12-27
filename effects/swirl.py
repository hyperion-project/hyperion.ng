import hyperion, time, random

# Convert x/y (0.0 - 1.0) point to proper int values based on Hyperion image width/height
# Or get a random value
# @param bool rand Randomize point if true
# @param float x Point at the x axis between 0.0-1.0
# @param float y Point at the y axis between 0.0-1.0
# @return Tuple with (x,y) as Integer
def getPoint(rand = True ,x = 0.5, y = 0.5):
	if rand:
		x = random.uniform(0.0, 1.0)
		y = random.uniform(0.0, 1.0)
	x = int(round(x*hyperion.imageWidth()))
	y = int(round(y*hyperion.imageHeight()))
	return (x,y)

# Returns the required sleep time for a interval function based on rotationtime and steps
# Adapts also to led device latchTime if required
# @param float rt RotationTime in seconds (time for one ration, based on steps)
# @param int steps The steps it should calc the rotation time
# @return Tuple with (x,y) as Integer
def getSTime(rt, steps = 360):
	rt = float(rt)
	sleepTime = max(0.1, rt) / steps
	
	# adapt sleeptime to hardware
	minStepTime= float(hyperion.latchTime)/1000.0
	if minStepTime == 0: minStepTime = 0.001
	if minStepTime > sleepTime:
		sleepTime = minStepTime
	return sleepTime
	
# Creates a PRGBA bytearray gradient based on provided colors (RGB or RGBA (0-255, 0-1 for alpha)), the color stop positions are calculated based on color count. Requires at least 2 colors!
# @param tuple cc Colors in a tuple of RGB or RGBA
# @param bool closeCircle If True use last color as first color
# @return bytearray A bytearray of RGBA for hyperion.image*Gradient functions
def buildGradient(cc, closeCircle = True):
	if len(cc) > 1:
		withAlpha = False
		posfac = int(255/len(cc))
		ba = bytearray()
		pos = 0
		if len(cc[0]) == 4:
			withAlpha = True

		for c in cc:
			if withAlpha:
				alpha = int(c[3]*255)
			else:
				alpha = 255
			pos += posfac
			ba += bytearray([pos,c[0],c[1],c[2],alpha])

		if closeCircle:
			# last color as first color
			lC = cc[-1]
			if withAlpha:
				alpha = int(lC[3]*255)
			else:
				alpha = 255
			ba += bytearray([0,lC[0],lC[1],lC[2],alpha])

		return ba
	return bytearray()

def rotateAngle( increment = 1):
	global angle
	angle += increment
	if angle > 360: angle=0
	if angle <   0: angle=360
	return angle
	
def rotateAngle2( increment = 1):
	global angle2
	angle2 += increment
	if angle2 > 360: angle2=0
	if angle2 <   0: angle2=360
	return angle2

# set minimum image size - must be done asap
hyperion.imageMinSize(64,64)
iW = hyperion.imageWidth()
iH = hyperion.imageHeight()

# Get the parameters
rotationTime  = float(hyperion.args.get('rotation-time', 10.0))
reverse       = bool(hyperion.args.get('reverse', False))
centerX       = float(hyperion.args.get('center_x', 0.5))
centerY       = float(hyperion.args.get('center_y', 0.5))
randomCenter  = bool(hyperion.args.get('random-center', False))
custColors	  = hyperion.args.get('custom-colors', ((255,0,0),(0,255,0),(0,0,255)))

enableSecond  = bool(hyperion.args.get('enable-second', False))
#rotationTime2 = float(hyperion.args.get('rotation-time2', 5.0))
reverse2      = bool(hyperion.args.get('reverse2', True))
centerX2      = float(hyperion.args.get('center_x2', 0.5))
centerY2      = float(hyperion.args.get('center_y2', 0.5))
randomCenter2 = bool(hyperion.args.get('random-center2', False))
custColors2	  = hyperion.args.get('custom-colors2', ((255,255,255,0),(0,255,255,0),(255,255,255,1),(0,255,255,0),(0,255,255,0),(0,255,255,0),(255,255,255,1),(0,255,255,0),(0,255,255,0),(0,255,255,0),(255,255,255,1),(0,255,255,0)))

# process parameters
pointS1    = getPoint(randomCenter ,centerX, centerY)
pointS2    = getPoint(randomCenter2 ,centerX2, centerY2)
sleepTime  = getSTime(rotationTime)
#sleepTime2 = getSTime(rotationTime2)
angle      = 0
angle2     = 0
S2 		   = False

increment  = -1 if reverse else 1
increment2 = -1 if reverse2 else 1

if len(custColors) > 1:
	baS1 = buildGradient(custColors)
else:
	baS1 = bytearray([
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

# check if the second swirl should be build
if enableSecond and len(custColors2) > 1:
	S2 = True
	baS2 = buildGradient(custColors2)

# effect loop
while not hyperion.abort():
	angle += increment
	if angle > 360: angle=0
	if angle <   0: angle=360
	
	angle2 += increment2
	if angle2 > 360: angle2=0
	if angle2 <   0: angle2=360
	
	hyperion.imageConicalGradient(pointS1[0], pointS1[1], angle, baS1)
	if S2:
		hyperion.imageConicalGradient(pointS2[0], pointS2[1], angle2, baS2)
		
	hyperion.imageShow()
	time.sleep(sleepTime)


