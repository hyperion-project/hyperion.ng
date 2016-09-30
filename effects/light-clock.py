import hyperion, time, datetime

# Get the parameters
centerX   = int(round(hyperion.imageWidth)/2)
centerY   = int(round(float(hyperion.imageHeight)/2))

# table of stop colors for rainbow gradient, first is the position, next rgb, all values 0-255

colorsSecond = bytearray([
	0, 255,0,0,255,
	5, 255,0,0,255,
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
	255, 0,0,255,5,
])



# effect loop
while not hyperion.abort():
	now = datetime.datetime.now()

	angleH = 359 - 30*(now.hour if now.hour<12 else now.hour-12)+90
	angleM = 359 - 6*now.minute+90
	angleS = 359 - 6*now.second+90

	angleH = angleH if angleH<360 else angleH-360
	angleM = angleM if angleM<360 else angleM-360
	angleS = angleS if angleS<360 else angleS-360

	hyperion.imageSolidFill(145,145,64);
	hyperion.imageCanonicalGradient(centerX, centerY, angleH, colorsHour)
	hyperion.imageCanonicalGradient(centerX, centerY, angleM, colorsMinute)
	hyperion.imageCanonicalGradient(centerX, centerY, angleS, colorsSecond)

	hyperion.imageShow()
	time.sleep(0.01 )
