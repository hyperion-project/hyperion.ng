import hyperion, time, subprocess

def setPixel(x,y,rgb):
	global imageData, width
	offset = y*width*3 + x*3
	if offset+2 < len(imageData):
		imageData[offset]   = rgb[0]
		imageData[offset+1] = rgb[1]
		imageData[offset+2] = rgb[2]

# Initialize the led data and args
sleepTime      = float(hyperion.args.get('speed', 1.0))*0.3
alarmColor     = hyperion.args.get('alarm-color', (255,0,0))
off            = bool(hyperion.args.get('shutdown-enabled', False))
width          = 12
height         = 10

imageData      = bytearray(height * width * (0,0,0))

# Start the write data loop


for y in range(height,0,-1):
	if hyperion.abort():
		off = False
		break
	for x in range(width):
		setPixel(x, y-1, alarmColor)
	hyperion.setImage(width, height, imageData)
	time.sleep(sleepTime)
time.sleep(1)

for y in range(height):
	for x in range(width):
		setPixel(x, y, postColor)
hyperion.setImage(width, height, imageData)
time.sleep(2)

if off and not hyperion.abort():
	subprocess.call("halt")
