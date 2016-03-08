import hyperion, time

# Get the rotation time
color     =       hyperion.args.get('color',     (255,255,255))
frequency = float(hyperion.args.get('frequency', 10.0))

# Check parameters
frequency = min(100.0, frequency)

# Compute the strobe interval
sleepTime = 1.0 / frequency

# Start the write data loop
while not hyperion.abort():
	hyperion.setColor(0, 0, 0)
	time.sleep(sleepTime)
	hyperion.setColor(color[0], color[1], color[2])
	time.sleep(sleepTime)
