from __future__ import division
import hyperion, time, colorsys, math

hyperion.imageMinSize(64, 64)
width = hyperion.imageWidth()
height = hyperion.imageHeight()
sleepTime = float(hyperion.args.get('sleepTime', 0.2))

def mapto(x, in_min, in_max, out_min, out_max):
	return float((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

pal = []
for h in range(256):
	r, g, b = colorsys.hsv_to_rgb(mapto(h, 0.0, 255.0, 0.0, 1.0), 1.0, 1.0)
	pal.append( bytearray( (int(r*255), int(g*255), int(b*255)) ) )

plasma = [[]]
for x in range(width):
	plasma.append([])
	for y in range(height):
		color = int(128.0 + (128.0 * math.sin(x / 16.0)) + \
			128.0 + (128.0 * math.sin(y / 8.0)) + \
			128.0 + (128.0 * math.sin((x+y)) / 16.0) + \
			128.0 + (128.0 * math.sin(math.sqrt(x**2.0 + y**2.0) / 8.0))) / 4
		plasma[x].append(color)

while not hyperion.abort():
	ledData = bytearray()
	mod = time.process_time() * 100
	for x in range(height):
		for y in range(width):
			ledData += pal[int((plasma[y][x] + mod) % 256)]

	hyperion.setImage(width,height,ledData)
	time.sleep(sleepTime)

