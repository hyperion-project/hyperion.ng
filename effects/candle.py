
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
color = hyperion.args.get('color', (255,138,0))
colorShift = float(hyperion.args.get('colorShift', 1))/100.0
brightness = float(hyperion.args.get('brightness', 100))/100.0

sleepTime = float(hyperion.args.get('sleepTime', 0.14))

candles = hyperion.args.get('candles', "all")
ledlist = hyperion.args.get('ledlist', "1")

candlelist = ()
if (candles == "list") and (type(ledlist) is str):
	for s in ledlist.split(','):
		i = int(s)
		if (i<hyperion.ledCount):
			candlelist += (i,)
elif (candles == "list") and (type(ledlist) is list):
	for s in (ledlist):
		i = int(s)
		if (i<hyperion.ledCount):
			candlelist += (i,)
else:
	candlelist = range(hyperion.ledCount)


# Convert rgb color to hsv
hsv = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color [2]/255.0)


def CandleRgb():
	hue = random.uniform(hsv[0]-colorShift, hsv[0]+colorShift) % 1.0

	RAND=random.randint(0,15)
	while ((RAND & 0x0c)==0):
		RAND=random.randint(0,15)
	val = ( min(RAND, 15)/15.0001 ) * brightness

	frgb = colorsys.hsv_to_rgb(hue, hsv[1], val);

	return (int(255*frgb[0]), int(255*frgb[1]), int(255*frgb[2]))


ledData = bytearray(hyperion.ledCount * (0,0,0) )
while not hyperion.abort():
	if (candles == "all-together"):
		rgb = CandleRgb()
		for lednum in candlelist:
			ledData[3*lednum+0] = rgb[0] 
			ledData[3*lednum+1] = rgb[1] 
			ledData[3*lednum+2] = rgb[2] 
	elif (candles == "all"):
		for lednum in candlelist:
			rgb = CandleRgb()
			ledData[3*lednum+0] = rgb[0] 
			ledData[3*lednum+1] = rgb[1] 
			ledData[3*lednum+2] = rgb[2] 
	else:
		for lednum in candlelist:
			rgb = CandleRgb()
			ledData[3*lednum+0] = rgb[0] 
			ledData[3*lednum+1] = rgb[1] 
			ledData[3*lednum+2] = rgb[2] 

	hyperion.setColor (ledData)
	time.sleep(sleepTime)

