import hyperion, time

# Get the parameters
speed = float(hyperion.args.get('speed', 1.0))
fadeFactor = float(hyperion.args.get('fadeFactor', 0.7))
color = hyperion.args.get('color', (255,0,0))

# Check parameters
speed = max(0.0001, speed)
fadeFactor = max(0.0, min(fadeFactor, 1.0))

# Initialize the led data
width = 25
imageData = bytearray(width * (0,0,0))
imageData[0] = color[0]
imageData[1] = color[1]
imageData[2] = color[2]

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
	for unused in range(increment):
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
			imageData[3*j+1] = int(fadeFactor * imageData[3*j+1])
			imageData[3*j+2] = int(fadeFactor * imageData[3*j+2])

		# Insert new data
		imageData[3*position] = color[0]
		imageData[3*position+1] = color[1]
		imageData[3*position+2] = color[2]
		
	# Sleep for a while
	time.sleep(sleepTime)
