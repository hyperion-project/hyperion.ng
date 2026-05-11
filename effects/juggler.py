import hyperion, time, math, colorsys

# Parameter
brightness = float(hyperion.args.get('brightness', 100)) / 100.0

# Image setup
hyperion.imageMinSize(64, 64)
iW = hyperion.imageWidth()
iH = hyperion.imageHeight()

sleepTime = max(hyperion.lowestUpdateInterval(), 1.0 / 60.0)

# Buffer
buf = [[[0, 0, 0] for _ in range(iH)] for _ in range(iW)]

# Constants
numDots = 8
dotR = max(2, min(iW, iH) // 20)
dotRSq = dotR * dotR

# Precompute the circular dot mask
dotMask = []
for ox in range(-dotR, dotR + 1):
	for oy in range(-dotR, dotR + 1):
		distSq = ox * ox + oy * oy
		if distSq <= dotRSq:
			dist = math.sqrt(distSq)
			intensity = max(0, 255 - int(dist / dotR * 200))
			dotMask.append((ox, oy, intensity))

# Main loop
startTime = time.time()
while not hyperion.abort():
	tMs = (time.time() - startTime) * 1000.0

	# Fade buffer
	scale = max(0.0, 1.0 - 25 / 256.0)
	for x in range(iW):
		for y in range(iH):
			buf[x][y][0] = int(buf[x][y][0] * scale)
			buf[x][y][1] = int(buf[x][y][1] * scale)
			buf[x][y][2] = int(buf[x][y][2] * scale)

	# Draw moving dots
	for i in range(numDots):
		angleX = (tMs / 60000.0) * (i + 5) * 2.0 * math.pi + i * 1.3
		angleY = (tMs / 60000.0) * (i + 3) * 2.0 * math.pi + i * 0.9 + math.pi
		dx = int((math.sin(angleX) + 1.0) * 0.5 * (iW - 1))
		dy = int((math.sin(angleY) + 1.0) * 0.5 * (iH - 1))

		r, g, b = colorsys.hsv_to_rgb(((i * 32) & 255) / 255.0, 200 / 255.0, 255 / 255.0)
		r = int(r * 255)
		g = int(g * 255)
		b = int(b * 255)

		for ox, oy, intensity in dotMask:
			x = dx + ox
			y = dy + oy
			if 0 <= x < iW and 0 <= y < iH:
				pr = r * intensity // 255
				pg = g * intensity // 255
				pb = b * intensity // 255
				buf[x][y][0] = max(buf[x][y][0], pr)
				buf[x][y][1] = max(buf[x][y][1], pg)
				buf[x][y][2] = max(buf[x][y][2], pb)

	# Push buffer to Hyperion image
	for x in range(iW):
		for y in range(iH):
			r = int(buf[x][y][0] * brightness)
			g = int(buf[x][y][1] * brightness)
			b = int(buf[x][y][2] * brightness)
			hyperion.imageSetPixel(x, y, r, g, b)

	hyperion.imageShow()
	hyperion.imageStackClear()

	time.sleep(sleepTime)