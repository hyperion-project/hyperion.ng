import hyperion
import time
import colorsys
import math

# Get the parameters
rotationTime = float(hyperion.args.get('rotationTime', 20.0))
color = hyperion.args.get('color', (0,0,255))
hueChange = float(hyperion.args.get('hueChange', 60.0)) / 360.0
blobs = int(hyperion.args.get('blobs', 5))
reverse = bool(hyperion.args.get('reverse', False))

# Check parameters
rotationTime = max(0.1, rotationTime)
hueChange = max(0.0, min(abs(hueChange), .5))
blobs = max(1, blobs)

# Calculate the color data
baseHsv = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color[2]/255.0)
colorData = bytearray()
for i in range(hyperion.ledCount):
	hue = (baseHsv[0] + hueChange * math.sin(2*math.pi * i / hyperion.ledCount)) % 1.0
	rgb = colorsys.hsv_to_rgb(hue, baseHsv[1], baseHsv[2])
	colorData += bytearray((int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))

# Calculate the increments
sleepTime = 0.1
amplitudePhaseIncrement = blobs * math.pi * sleepTime / rotationTime
colorDataIncrement = 3

# Switch direction if needed
if reverse:
	amplitudePhaseIncrement = -amplitudePhaseIncrement
	colorDataIncrement = -colorDataIncrement

# create a Array for the colors
colors = bytearray(hyperion.ledCount * (0,0,0))

# Start the write data loop
amplitudePhase = 0.0
rotateColors = False
while not hyperion.abort():
	# Calculate new colors
	for i in range(hyperion.ledCount):
		amplitude = max(0.0, math.sin(-amplitudePhase + 2*math.pi * blobs * i / hyperion.ledCount))
		colors[3*i+0] = int(colorData[3*i+0] * amplitude)
		colors[3*i+1] = int(colorData[3*i+1] * amplitude)
		colors[3*i+2] = int(colorData[3*i+2] * amplitude)

	# set colors
	hyperion.setColor(colors)
	
	# increment the phase
	amplitudePhase = (amplitudePhase + amplitudePhaseIncrement) % (2*math.pi)
	
	if rotateColors:
		colorData = colorData[-colorDataIncrement:] + colorData[:-colorDataIncrement]
	rotateColors = not rotateColors
	
	# sleep for a while
	time.sleep(sleepTime)
