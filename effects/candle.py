
# Candleflicker effect by penfold42
# Algorithm courtesy of 
# https://cpldcpu.com/2013/12/08/hacking-a-candleflicker-led/

# candles can be :
# a single led number, a list of candle numbers
# "all" to flicker all the leds randomly
# "all-together" to flicker all the leds in unison

import hyperion
import time
import colorsys
import random

# Get parameters
color = hyperion.args.get('color', (255,200,0))
sleepTime = float(hyperion.args.get('sleepTime', 0.14))
brightness = float(hyperion.args.get('brightness', 0.5))
candles = hyperion.args.get('candles', "all")

candlelist = ()
if ((candles == "all") or (candles == "all-together")):
	candlelist = range(hyperion.ledCount)
elif (type(candles) is int):
	if (candles < hyperion.ledCount):
		candlelist = (candles,)
else:
	for i in (candles):
		if (i<hyperion.ledCount):
			candlelist += (i,)


# Convert color to hsv
hsv = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color [2]/255.0)

def CandleVal():
	RAND=random.randint(0,15)
	while ((RAND & 0x0c)==0):
		RAND=random.randint(0,15)
	return ( min(RAND, 15)/15.0001 );

ledData = bytearray(hyperion.ledCount * (0,0,0) )
while not hyperion.abort():
	if (candles == "all-together"):
		rgb = colorsys.hsv_to_rgb(random.uniform(0.08,0.10), hsv[1], CandleVal() * brightness )
		for lednum in candlelist:
			ledData[3*lednum+0] = int(255*rgb[0])
			ledData[3*lednum+1] = int(255*rgb[1])
			ledData[3*lednum+2] = int(255*rgb[2])
	elif (candles == "all"):
		for lednum in candlelist:
			rgb = colorsys.hsv_to_rgb(random.uniform(0.08,0.10), hsv[1], CandleVal() * brightness )
			ledData[3*lednum+0] = int(255*rgb[0])
			ledData[3*lednum+1] = int(255*rgb[1])
			ledData[3*lednum+2] = int(255*rgb[2])
	else:
		for lednum in candlelist:
			rgb = colorsys.hsv_to_rgb(random.uniform(0.08,0.10), hsv[1], CandleVal() * brightness )
			ledData[3*lednum+0] = int(255*rgb[0])
			ledData[3*lednum+1] = int(255*rgb[1])
			ledData[3*lednum+2] = int(255*rgb[2])

	hyperion.setColor (ledData)
	time.sleep(sleepTime)

