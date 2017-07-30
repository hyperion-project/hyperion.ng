import hyperion, time, datetime

hyperion.imageMinSize(32,32)

# Get the parameters
showSec    = bool(hyperion.args.get('show_seconds', True))
hC         = hyperion.args.get('hour-color', (0,0,255))
mC  	   = hyperion.args.get('minute-color', (0,255,0))
sC 		   = hyperion.args.get('second-color', (255,0,0))
bgC		   = hyperion.args.get('background-color', (0,0,0))
markEnable = hyperion.args.get('marker-enabled', False)
markD      = int(hyperion.args.get('marker-depth', 5))/100.0
markW      = int(hyperion.args.get('marker-width', 5))/100.0
markC	   = hyperion.args.get('marker-color', (255,255,255))


#calculate some stuff
centerX    = int(round(hyperion.imageWidth())/2)
centerY    = int(round(float(hyperion.imageHeight())/2))
markDepthX = int(round(hyperion.imageWidth()*markD))
markDepthY = int(round(hyperion.imageHeight()*markD))
markThick  = int(round(hyperion.imageHeight()*markW))

colorsSecond = bytearray([
	0, sC[0],sC[1],sC[2],255,
	8, sC[0],sC[1],sC[2],255,
	10, 0,0,0,0,
])

colorsMinute = bytearray([
	0, mC[0],mC[1],mC[2],255,
	35, mC[0],mC[1],mC[2],255,
	50, mC[0],mC[1],mC[2],127,
	90, 0,0,0,0,
])

colorsHour = bytearray([
	0, hC[0],hC[1],hC[2],255,
	90, hC[0],hC[1],hC[2],255,
	150, hC[0],hC[1],hC[2],127,
	191, 0,0,0,0,
])


# effect loop
while not hyperion.abort():
	now = datetime.datetime.now()

	angleH = 449 - 30*(now.hour if now.hour<12 else now.hour-12)
	angleM = 449 - 6*now.minute
	angleS = 449 - 6*now.second
	
	angleH -= 0 if angleH<360 else 360
	angleM -= 0 if angleM<360 else 360
	angleS -= 0 if angleS<360 else 360

	#reset image
	hyperion.imageSolidFill(bgC[0],bgC[1],bgC[2])
	
	#paint clock
	if angleH-angleM < 90 and angleH-angleM > 0:
		hyperion.imageConicalGradient(centerX, centerY, angleM, colorsMinute)
		hyperion.imageConicalGradient(centerX, centerY, angleH, colorsHour)
	else:
		hyperion.imageConicalGradient(centerX, centerY, angleH, colorsHour)
		hyperion.imageConicalGradient(centerX, centerY, angleM, colorsMinute)

	if showSec:
		hyperion.imageConicalGradient(centerX, centerY, angleS, colorsSecond)
	if markEnable:
		#marker left, right, top, bottom
		hyperion.imageDrawLine(0, centerY, 0+markDepthX, centerY, markThick, markC[0], markC[1], markC[2])
		hyperion.imageDrawLine(int(hyperion.imageWidth()), centerY, int(hyperion.imageWidth())-markDepthX, centerY, markThick, markC[0], markC[1], markC[2])
		hyperion.imageDrawLine(centerX, 0, centerX, 0+markDepthY, markThick, markC[0], markC[1], markC[2])
		hyperion.imageDrawLine(centerX, int(hyperion.imageHeight()), centerX, int(hyperion.imageHeight())-markDepthY, markThick, markC[0], markC[1], markC[2])

	hyperion.imageShow()
	time.sleep(0.5)