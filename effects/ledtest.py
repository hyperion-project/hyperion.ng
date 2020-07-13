# testleds can be :
# "all" to test all the leds
# a single led number, a list of led numbers

import hyperion
import time
#import colorsys

# Get parameters
sleepTime = float(hyperion.args.get('sleepTime', 0.5))
testleds = hyperion.args.get('testleds', "all")
ledlist = hyperion.args.get('ledlist', "1")

testlist = ()
if (testleds == "list") and (type(ledlist) is str):
	for s in ledlist.split(','):
		i = int(s)
		if (i<hyperion.ledCount):
			testlist += (i,)
elif (testleds == "list") and (type(ledlist) is list):
	for s in (ledlist):
		i = int(s)
		if (i<hyperion.ledCount):
			testlist += (i,)
else:
	testlist = range(hyperion.ledCount)

def TestRgb( iteration ):

    switcher = {
        0: (255, 0, 0),
        1: (0, 255, 0),
        2: (0, 0, 255),
        3: (255, 255, 255),
        4: (0, 0, 0),
    }

    return switcher.get(iteration, (127,127,127) )

ledData = bytearray(hyperion.ledCount * (0,0,0) )
i = 0
while not hyperion.abort():
	j = i % 5
	if (testleds == "all"):
		for lednum in testlist:
			rgb = TestRgb( j )
			ledData[3*lednum+0] = rgb[0] 
			ledData[3*lednum+1] = rgb[1] 
			ledData[3*lednum+2] = rgb[2] 
	else:
		for lednum in testlist:
			rgb = TestRgb( j )
			ledData[3*lednum+0] = rgb[0] 
			ledData[3*lednum+1] = rgb[1] 
			ledData[3*lednum+2] = rgb[2] 

	hyperion.setColor (ledData)
	i += 1
	time.sleep(sleepTime)

