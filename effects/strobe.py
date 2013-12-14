import hyperion 
import time 
import colorsys

# Get the rotation time
color     =       hyperion.args.get('color',     (255,255,255))
frequency = float(hyperion.args.get('frequency', 10.0))

# Check parameters
frequency = min(100.0, frequency)

# Compute the strobe interval
sleepTime = 1.0 / frequency

# Initialize the led data
blackLedsData = bytearray(hyperion.ledCount * (  0,  0,  0))
whiteLedsData = bytearray(hyperion.ledCount * color)

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(blackLedsData)
	time.sleep(sleepTime)
	hyperion.setColor(whiteLedsData)
	time.sleep(sleepTime)
