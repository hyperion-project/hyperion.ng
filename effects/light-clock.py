import hyperion, time, datetime

# Get the parameters
showSeconds = bool(hyperion.args.get('show_seconds', True))
centerX     = int(round(hyperion.imageWidth())/2)
centerY     = int(round(float(hyperion.imageHeight())/2))

colorsSecond = bytearray([
	0, 255,255,0,255,
	5, 255,255,0,255,
	30, 0,0,0,0,
])

colorsMinute = bytearray([
	0, 0,255,0,255,
	5, 0,255,0,250,
	90, 0,0,0,0,
	
])

colorsHour = bytearray([
	0, 0,0,255,255,
	10, 0,0,255,255,
	127, 0,0,196,127,
	255, 0,0,196,5,
])

colorsHourTop = bytearray([
	0, 0,0,255,250,
	10, 0,0,255,128,
	20, 0,0,0,0,
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

	hyperion.imageSolidFill(127,127,127);
	hyperion.imageCanonicalGradient(centerX, centerY, angleH, colorsHour)
	hyperion.imageCanonicalGradient(centerX, centerY, angleM, colorsMinute)
	hyperion.imageCanonicalGradient(centerX, centerY, angleH, colorsHourTop)
	if showSeconds:
		hyperion.imageCanonicalGradient(centerX, centerY, angleS, colorsSecond)

	hyperion.imageShow()
	time.sleep(0.5 )
