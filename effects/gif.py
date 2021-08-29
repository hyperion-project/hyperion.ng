import hyperion, time

# Get the parameters
imageFile = hyperion.args.get('image')
framesPerSecond = float(hyperion.args.get('fps', 25))
reverse = bool(hyperion.args.get('reverse', False))

# Cropping parameters
cropLeft = int(hyperion.args.get('cropLeft', 0))
cropTop = int(hyperion.args.get('cropTop', 0))
cropRight = int(hyperion.args.get('cropRight', 0))
cropBottom = int(hyperion.args.get('cropBottom', 0))

sleepTime = 1./framesPerSecond
imageList = []

if imageFile:
	if reverse:
		imageList = reversed(hyperion.getImage(imageFile, cropLeft, cropTop, cropRight, cropBottom))
	else:
		imageList = hyperion.getImage(imageFile, cropLeft, cropTop, cropRight, cropBottom)

# Start the write data loop
while not hyperion.abort() and imageList:
	for image in imageList:
		hyperion.setImage(image["imageWidth"], image["imageHeight"], image["imageData"])
		time.sleep(sleepTime)
