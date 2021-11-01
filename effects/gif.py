import hyperion, time

# Get the parameters
imageData = hyperion.args.get('url') if hyperion.args.get('imageSource', "") == "url" else hyperion.args.get('file')
framesPerSecond = float(hyperion.args.get('fps', 25))
reverse = bool(hyperion.args.get('reverse', False))
cropLeft = int(hyperion.args.get('cropLeft', 0))
cropTop = int(hyperion.args.get('cropTop', 0))
cropRight = int(hyperion.args.get('cropRight', 0))
cropBottom = int(hyperion.args.get('cropBottom', 0))
grayscale = bool(hyperion.args.get('grayscale', False))

sleepTime = 1./framesPerSecond
imageFrameList = []

if imageData:
	if reverse:
		imageFrameList = reversed(hyperion.getImage(imageData, cropLeft, cropTop, cropRight, cropBottom, grayscale))
	else:
		imageFrameList = hyperion.getImage(imageData, cropLeft, cropTop, cropRight, cropBottom, grayscale)

# Start the write data loop
while not hyperion.abort() and imageFrameList:
	for image in imageFrameList:
		if not hyperion.abort():
			hyperion.setImage(image["imageWidth"], image["imageHeight"], image["imageData"])
			time.sleep(sleepTime)
