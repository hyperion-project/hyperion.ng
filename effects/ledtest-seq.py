import hyperion
import time

# Get parameters
sleepTime = float(hyperion.args.get('sleepTime', 0.5))

def TestRgb( iteration ):

    switcher = {
        0: (255, 0, 0),
        1: (0, 255, 0),
        2: (0, 0, 255),
    }

    return switcher.get(iteration, (127,127,127) )

ledData = bytearray(hyperion.ledCount * (0,0,0) )

i = 0
while not hyperion.abort():

	if i < hyperion.ledCount:
		j = i % 3
		rgb = TestRgb( j )
		ledData[3*i+0] = rgb[0] 
		ledData[3*i+1] = rgb[1] 
		ledData[3*i+2] = rgb[2] 
		i += 1
	else:
		if i == hyperion.ledCount:
			ledData = bytearray(hyperion.ledCount * (0,0,0) )
			i += 1
		else:
			i = 0

	hyperion.setColor (ledData)
		
	time.sleep(sleepTime)

