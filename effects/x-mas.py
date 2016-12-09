import hyperion, time, colorsys

# Get the parameters
sleepTime = float(hyperion.args.get('sleepTime', 1.0))

# Initialize the led data
ledDataOdd = bytearray()
for i in range(hyperion.ledCount):
	if i%2 == 0:
		ledDataOdd += bytearray((int(255), int(0), int(0)))
	else:
		ledDataOdd += bytearray((int(255), int(255), int(255)))
		
ledDataEven = bytearray()
for i in range(hyperion.ledCount):
	if i%2 == 0:
		ledDataEven += bytearray((int(255), int(255), int(255)))
	else:
		ledDataEven += bytearray((int(255), int(0), int(0)))

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(ledDataOdd)
	time.sleep(sleepTime)
	hyperion.setColor(ledDataEven)
	time.sleep(sleepTime)