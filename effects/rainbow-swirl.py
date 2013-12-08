import hyperion
import time
import colorsys

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 3.0))
brightness = float(hyperion.args.get('brightness', 1.0))
saturation = float(hyperion.args.get('saturation', 1.0))
reverse = bool(hyperion.args.get('reverse', False))

# Check parameters
rotationTime = max(0.1, rotationTime)
brightness = max(0.0, min(brightness, 1.0))
saturation = max(0.0, min(saturation, 1.0))

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
	hue = float(i)/hyperion.ledCount
	rgb = colorsys.hsv_to_rgb(hue, saturation, brightness)
	ledData += bytearray((int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))

# Calculate the sleep time and rotation increment
increment = 3
sleepTime = rotationTime / hyperion.ledCount
while sleepTime < 0.05:
	increment *= 2
	sleepTime *= 2
increment %= hyperion.ledCount

# Switch direction if needed
if reverse:
	increment = -increment
	
# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledData)
	ledData = ledData[-increment:] + ledData[:-increment]
	time.sleep(sleepTime)
