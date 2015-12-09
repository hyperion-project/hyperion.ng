import hyperion
import time
import colorsys

# Get the parameters
frequency = float(hyperion.args.get('frequency', 10.0))

# Check parameters
frequency = min(100.0, frequency)

# Compute the strobe interval
sleepTime = 1.0 / frequency

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
	time.sleep(0.75)
	hyperion.setColor(ledDataEven)
	time.sleep(0.75)