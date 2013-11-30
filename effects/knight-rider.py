import hyperion 
import time 
import colorsys

# Get the rotation time
speed = hyperion.args.get('speed', 1.0)
fadeFactor = hyperion.args.get('fadeFactor', 0.7)

# Initialize the led data
width = 25
imageData = bytearray(width * (0,0,0))
imageData[0] = 255

# Calculate the sleep time and rotation increment
increment = 1
sleepTime = 1.0 / (speed * width)
while sleepTime < 0.05:
	increment *= 2
	sleepTime *= 2

# Start the write data loop
position = 0
direction = 1
while not hyperion.abort():
	hyperion.setImage(width, 1, imageData)

	# Move data into next state
	for i in range(increment):
		position += direction
		if position == -1:
			position = 1
			direction = 1
		elif position == width:
			position = width-2
			direction = -1

		# Fade the old data
		for j in range(width):
			imageData[3*j] = int(fadeFactor * imageData[3*j])

		# Insert new data
		imageData[3*position] = 255
		
	# Sleep for a while
	time.sleep(sleepTime)
