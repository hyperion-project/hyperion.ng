import hyperion
import time
from datetime import datetime

def myRange(index, margin):
	return [i % hyperion.ledCount for i in range(index-margin, index+margin+1)]

""" Define some variables """
sleepTime = 1
markers = [0, 13, 25, 38]

ledCount = hyperion.ledCount

offset = hyperion.args.get('offset', 0)

hourMargin = hyperion.args.get('hour-margin', 2)
minuteMargin = hyperion.args.get('minute-margin', 1)
secondMargin = hyperion.args.get('second-margin', 0)

hourColor = hyperion.args.get('hour-color', (255,0,0))
minuteColor = hyperion.args.get('minute-color', (0,255,0))
secondColor = hyperion.args.get('second-color', (0,0,255))

markerColor = hyperion.args.get('marker-color', (255,255,255))
markers = hyperion.args.get('markers', (-1,-1,-1,-1))

""" The effect loop """
while not hyperion.abort():

	""" The algorithm to calculate the change in color """
	led_data = bytearray()

	now = datetime.now()
	h = now.hour
	m = now.minute
	s = now.second

	if h > 12:
		h -= 12
		return
	
	hour = h/12. * ledCount
	led_hour = int(hour + offset) % ledCount

	minute = m/60. * ledCount
	led_minute = int(minute + offset) % ledCount

	second = s/60. * ledCount
	led_second = int(second + offset) % ledCount

	hourRange =  myRange(led_hour, hourMargin)
	minuteRange = myRange(led_minute, minuteMargin)
	secondRange = myRange(led_second, secondMargin)

	for i in range(ledCount):
		blend = [0, 0, 0]

		if i in markers:
			blend = markerColor

		if i in hourRange:
			blend = hourColor

		if i in minuteRange:
			blend = minuteColor

		if i in secondRange:
			blend = secondColor

		led_data += bytearray((int(blend[0]), int(blend[1]), int(blend[2])))

	""" send the data to hyperion """
	hyperion.setColor(led_data)

	""" sleep for a while """
	timediff = (datetime.now()-now).microseconds/1000000.
	time.sleep(sleepTime-timediff)

