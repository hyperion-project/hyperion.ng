import hyperion, time

# Get the parameters
sleepTime   = float(hyperion.args.get('sleepTime', 1000))/1000.0
length = hyperion.args.get('length', 1)
color1 = hyperion.args.get('color1', (255,255,255))
color2 = hyperion.args.get('color2', (255,0,0))

# Initialize the led data
i = 0
ledDataOdd = bytearray()
while i < hyperion.ledCount:
	for unused in range(length):
		if i<hyperion.ledCount:
			ledDataOdd += bytearray((int(color1[0]), int(color1[1]), int(color1[2])))
			i += 1

	for unused in range(length):
		if i<hyperion.ledCount:
			ledDataOdd += bytearray((int(color2[0]), int(color2[1]), int(color2[2])))
			i += 1

ledDataEven = ledDataOdd[3*length:] + ledDataOdd[0:3*length]

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledDataOdd)
	time.sleep(sleepTime)
	hyperion.setColor(ledDataEven)
	time.sleep(sleepTime)
