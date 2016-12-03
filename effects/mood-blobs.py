import hyperion, time, colorsys, math
from random import random

# Get the parameters
rotationTime = float(hyperion.args.get('rotationTime', 20.0))
color = hyperion.args.get('color', (0,0,255))
colorRandom = bool(hyperion.args.get('colorRandom', False))
hueChange = float(hyperion.args.get('hueChange', 60.0))
blobs = int(hyperion.args.get('blobs', 5))
reverse = bool(hyperion.args.get('reverse', False))
baseColorChange = bool(hyperion.args.get('baseChange', False))
baseColorRangeLeft = float(hyperion.args.get('baseColorRangeLeft',0.0))   # Degree
baseColorRangeRight = float(hyperion.args.get('baseColorRangeRight',360.0))   # Degree
baseColorChangeRate = float(hyperion.args.get('baseColorChangeRate',10.0))   # Seconds for one Degree

# switch baseColor change off if left and right are too close together to see a difference in color
if (baseColorRangeRight > baseColorRangeLeft and (baseColorRangeRight - baseColorRangeLeft) < 10) or \
    (baseColorRangeLeft > baseColorRangeRight and ((baseColorRangeRight + 360) - baseColorRangeLeft) < 10):
    baseColorChange = False

# 360 -> 1
fullColorWheelAvailable = (baseColorRangeRight % 360) == (baseColorRangeLeft % 360)
baseColorChangeIncreaseValue = 1.0 / 360.0 # 1 degree
hueChange /= 360.0
baseColorRangeLeft = (baseColorRangeLeft / 360.0)
baseColorRangeRight = (baseColorRangeRight / 360.0)

# Check parameters
rotationTime = max(0.1, rotationTime)
hueChange = max(0.0, min(abs(hueChange), .5))
blobs = max(1, blobs)
baseColorChangeRate = max(0, baseColorChangeRate) # > 0

# Calculate the color data
baseHsv = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color[2]/255.0)
if colorRandom:
    baseHsv = (random(), baseHsv[1], baseHsv[2])

colorData = bytearray()
for i in range(hyperion.ledCount):
	hue = (baseHsv[0] + hueChange * math.sin(2*math.pi * i / hyperion.ledCount)) % 1.0
	rgb = colorsys.hsv_to_rgb(hue, baseHsv[1], baseHsv[2])
	colorData += bytearray((int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))

# Calculate the increments
sleepTime = 0.1
amplitudePhaseIncrement = blobs * math.pi * sleepTime / rotationTime
colorDataIncrement = 3
baseColorChangeRate /= sleepTime

# Switch direction if needed
if reverse:
	amplitudePhaseIncrement = -amplitudePhaseIncrement
	colorDataIncrement = -colorDataIncrement

# create a Array for the colors
colors = bytearray(hyperion.ledCount * (0,0,0))

# Start the write data loop
amplitudePhase = 0.0
rotateColors = False
baseColorChangeStepCount = 0
baseHSVValue = baseHsv[0]
numberOfRotates = 0

while not hyperion.abort():

    # move the basecolor
    if baseColorChange:
        # every baseColorChangeRate seconds
        if baseColorChangeStepCount >= baseColorChangeRate:
            baseColorChangeStepCount = 0
            # cyclic increment when the full colorwheel is available, move up and down otherwise
            if fullColorWheelAvailable:
                baseHSVValue = (baseHSVValue + baseColorChangeIncreaseValue) % baseColorRangeRight
            else:
                # switch increment direction if baseHSV <= left or baseHSV >= right
                if baseColorChangeIncreaseValue < 0 and baseHSVValue > baseColorRangeLeft and (baseHSVValue + baseColorChangeIncreaseValue) <= baseColorRangeLeft:
                    baseColorChangeIncreaseValue = abs(baseColorChangeIncreaseValue)
                elif baseColorChangeIncreaseValue > 0 and baseHSVValue < baseColorRangeRight and (baseHSVValue + baseColorChangeIncreaseValue)  >= baseColorRangeRight :
                    baseColorChangeIncreaseValue = -abs(baseColorChangeIncreaseValue)

                baseHSVValue = (baseHSVValue + baseColorChangeIncreaseValue) % 1.0

            # update color values
            colorData = bytearray()
            for i in range(hyperion.ledCount):
                hue = (baseHSVValue + hueChange * math.sin(2*math.pi * i / hyperion.ledCount)) % 1.0
                rgb = colorsys.hsv_to_rgb(hue, baseHsv[1], baseHsv[2])
                colorData += bytearray((int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))

            # set correct rotation after reinitialisation of the array
            colorData = colorData[-colorDataIncrement*numberOfRotates:] + colorData[:-colorDataIncrement*numberOfRotates]

        baseColorChangeStepCount += 1

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
        numberOfRotates = (numberOfRotates +  1) % hyperion.ledCount
    rotateColors = not rotateColors

    # sleep for a while
    time.sleep(sleepTime)
