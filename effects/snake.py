import hyperion
import time
import colorsys

# Get the parameters
rotationTime = float(hyperion.args.get('rotation-time', 10.0))
color = hyperion.args.get('color', (255,0,0))
percentage = int(hyperion.args.get('percentage', 10))

# Check parameters
rotationTime = max(0.1, rotationTime)
percentage = max(1, min(percentage, 100))

# Process parameters
factor = percentage/100.0
hsv = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color[2]/255.0)

# Initialize the led data
snakeLeds = max(1, int(hyperion.ledCount*factor))
ledData = bytearray()

for i in range(hyperion.ledCount-snakeLeds):
	ledData += bytearray((0, 0, 0))

for i in range(1,snakeLeds+1):
	rgb = colorsys.hsv_to_rgb(hsv[0], hsv[1], hsv[2]/i)
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
