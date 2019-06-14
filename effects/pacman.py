import hyperion, time
from random import randint

#get args
rotationTime = float(hyperion.args.get('rotationTime', 4))
marginPos    = float(hyperion.args.get('margin-pos', 2))

# define pacman
pacman = bytearray((255, 255, 0))

# define ghosts
redGuy  = bytearray((255, 0, 0))
pinkGuy = bytearray((255, 184, 255))
blueGuy = bytearray((0, 255, 255))
slowGuy = bytearray((255, 184, 81))

light = bytearray((255, 184, 174))
background = bytearray((0, 0, 0))

#helper
posPac    = 1
diffPac   = 6*marginPos
diffGuys  = 3*marginPos
sleepTime = max(0.02,rotationTime/hyperion.ledCount)

posPinkGuy = posPac + diffPac
posBlueGuy = posPinkGuy + diffGuys
posSlowGuy = posBlueGuy + diffGuys
posRedGuy = posSlowGuy + diffGuys

# initialize the led data
ledDataEscape = bytearray()
for i in range(hyperion.ledCount):
	if i == 1:
		ledDataEscape += pacman
	elif i == posPinkGuy:
		ledDataEscape += pinkGuy
	elif i == posBlueGuy:
		ledDataEscape += blueGuy
	elif i == posSlowGuy:
		ledDataEscape += slowGuy
	elif i == posRedGuy:
		ledDataEscape += redGuy
	else:
		ledDataEscape += background

ledDataChase = bytearray()
for i in range(hyperion.ledCount):
	if i == 1:
		ledDataChase += pacman
	elif i in [posPinkGuy, posBlueGuy, posSlowGuy, posRedGuy]:
		ledDataChase += bytearray((33, 33, 255))
	else:
		ledDataChase += background

# increment = 3, because LED-Color is defined by 3 Bytes
increment = 3

def shiftLED(ledData, increment, limit, lightPos=None):
	state = 0
	while state < limit and not hyperion.abort():
		ledData = ledData[increment:] + ledData[:increment]

		if (lightPos):
			tmp = ledData[lightPos]
			ledData[lightPos] = light

		hyperion.setColor(ledData)

		if (lightPos):
			ledData[lightPos] = tmp

		time.sleep(sleepTime)
		state += 1

# start the write data loop
while not hyperion.abort():

	# escape mode
	ledData = ledDataEscape
	shiftLED(ledData, increment, hyperion.ledCount)

	random = randint(10,hyperion.ledCount)

	# escape mode + power pellet
	s = slice(3*random, 3*random+3)
	shiftLED(ledData, increment, hyperion.ledCount - random, s)

	# chase mode
	shift   = 3*(hyperion.ledCount - random)
	ledData = ledDataChase[shift:]+ledDataChase[:shift]
	shiftLED(ledData, -increment, 2*hyperion.ledCount-random)
	time.sleep(sleepTime)

