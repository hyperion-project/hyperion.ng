import hyperion
import time
import colorsys

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 10.0))
color = hyperion.args.get('color', (255,0,0))
backgroundColor = hyperion.args.get('background-color', (0,0,0))
percentage = int(hyperion.args.get('percentage', 10))

# Check parameters
rotationTime = max(0.1, rotationTime)
percentage = max(1, min(percentage, 100))

# Process parameters
factor = percentage/100.0

# Initialize the led data
snakeLeds = max(1, int(hyperion.ledCount*factor))
ledData = bytearray()

color_hsv = colorsys.rgb_to_hsv(color[0]/255.0,color[1]/255.0,color[2]/255.0)
backgroundColor_hsv = colorsys.rgb_to_hsv(backgroundColor[0]/255.0,backgroundColor[1]/255.0,backgroundColor[2]/255.0)

for i in range(hyperion.ledCount-snakeLeds):
	ledData += bytearray((int(backgroundColor[0]), int(backgroundColor[1]), int(backgroundColor[2])))

def lerp(a, b, t):
    return (a[0]*(1-t)+b[0]*t, a[1]*(1-t)+b[1]*t, a[2]*(1-t)+b[2]*t)

for i in range(0, snakeLeds):
	hsv = lerp(color_hsv, backgroundColor_hsv, 1.0/(snakeLeds-1)*i)
	rgb = colorsys.hsv_to_rgb(hsv[0], hsv[1], hsv[2])
	ledData += bytearray((int(rgb[0]*255), int(rgb[1]*255), int(rgb[2]*255)))

# Calculate the sleep time and rotation increment
increment = 3
sleepTime = rotationTime / hyperion.ledCount
while sleepTime < 0.05:
	increment *= 2
	sleepTime *= 2
increment %= hyperion.ledCount

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledData)
	ledData = ledData[increment:] + ledData[:increment]
	time.sleep(sleepTime)
