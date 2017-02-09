import hyperion, time, subprocess

def setPixel(x,y,rgb):
	global imageData, width
	offset = y*width*3 + x*3
	if offset+2 < len(imageData):
		imageData[offset]   = rgb[0]
		imageData[offset+1] = rgb[1]
		imageData[offset+2] = rgb[2]

# Initialize the led data and args
sleepTime      = float(hyperion.args.get('speed', 1.0))*0.5
alarmColor     = hyperion.args.get('alarm-color', (255,0,0))
postColor      = hyperion.args.get('post-color', (255,174,11))
off            = bool(hyperion.args.get('shutdown-enabled', False))
initialBlink   = bool(hyperion.args.get('initial-blink', True))
setPostColor   = bool(hyperion.args.get('set-post-color', True))

width          = 12
height         = 10

imageData      = bytearray(height * width * (0,0,0))

# Start the write data loop
if initialBlink:
	for i in range(6):
		if hyperion.abort():
			off = False
			break
		if i % 2:
			hyperion.setColor(alarmColor[0], alarmColor[1], alarmColor[2])
		else:
			hyperion.setColor(0, 0, 0)
		time.sleep(sleepTime)

for y in range(height,0,-1):
	if hyperion.abort():
		off = False
		break
	for x in range(width):
		setPixel(x, y-1, alarmColor)
	hyperion.setImage(width, height, imageData)
	time.sleep(sleepTime)
time.sleep(1)

if setPostColor:
	for y in range(height):
		for x in range(width):
			setPixel(x, y, postColor)
	hyperion.setImage(width, height, imageData)
	time.sleep(2)

if off and not hyperion.abort():
	subprocess.call("halt")
