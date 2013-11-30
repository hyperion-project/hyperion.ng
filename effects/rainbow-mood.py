import hyperion
import time
import colorsys

# Get the rotation time
rotationTime = hyperion.args.get('rotation-time', 3.0)

# Get the brightness
brightness = hyperion.args.get('brightness', 1.0)

# Get the saturation
saturation = hyperion.args.get('saturation', 1.0)

# Get the direction
reverse = hyperion.args.get('reverse', False)

# Calculate the sleep time and hue increment
sleepTime = 0.1
hueIncrement = sleepTime / rotationTime

# Switch direction if needed
if reverse:
	increment = -increment

# Start the write data loop
hue = 0.0
while not hyperion.abort():
	rgb = colorsys.hsv_to_rgb(hue, saturation, brightness)
	hyperion.setColor(int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2]))
	hue = (hue + hueIncrement) % 1.0
	time.sleep(sleepTime)
