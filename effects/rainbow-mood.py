import hyperion, time, colorsys

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 30.0))
brightness   = float(hyperion.args.get('brightness', 100))/100.0
saturation   = float(hyperion.args.get('saturation', 100))/100.0
reverse      = bool(hyperion.args.get('reverse', False))

# Calculate the sleep time and hue increment
sleepTime = 0.1
hueIncrement = sleepTime / rotationTime

# Switch direction if needed
if reverse:
	hueIncrement = -hueIncrement

# Start the write data loop
hue = 0.0
while not hyperion.abort():
	rgb = colorsys.hsv_to_rgb(hue, saturation, brightness)
	hyperion.setColor(int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2]))
	hue = (hue + hueIncrement) % 1.0
	time.sleep(sleepTime)
