import hyperion, time

# Get the parameters
fadeTime   = float(hyperion.args.get('fade-time', 5.0))
colorStart = hyperion.args.get('color-start', (255,174,11))
colorEnd   = hyperion.args.get('color-end', (0,0,0))

steps     = (hyperion.args.get('steps', 0.0))
frequency = float(hyperion.args.get('frequency', 0.0))
sleepTime = 0 if frequency<=0 else 0.5 / frequency

color_step = (
	(colorEnd[0] - colorStart[0]) / 256.0,
	(colorEnd[1] - colorStart[1]) / 256.0,
	(colorEnd[2] - colorStart[2]) / 256.0
)

calcChannel = lambda i: min(max(int(colorStart[i] + color_step[i]*step),0),255)
colors = []
for step in range(256):
	colors.append( (calcChannel(0),calcChannel(1),calcChannel(2)) )

increment = 8

while not hyperion.abort():
	for step in range(0,256,increment):
		if hyperion.abort(): break
		hyperion.setColor( colors[step][0],colors[step][1],colors[step][2] )
		time.sleep( fadeTime / 256 )

	if sleepTime == 0: break
	time.sleep(sleepTime)

	for step in range(255,-1,-increment):
		if hyperion.abort(): break
		hyperion.setColor( colors[step][0],colors[step][1],colors[step][2] )
		time.sleep( fadeTime / 256 )

	time.sleep(sleepTime)

# maintain color until effect end
hyperion.setColor(colorEnd[0],colorEnd[1],colorEnd[2])
while not hyperion.abort():
	time.sleep(1)

