import hyperion, time

# Get the parameters
imageFile = hyperion.args.get('image')
framesPerSecond = float(hyperion.args.get('fps', 25))
reverse       = bool(hyperion.args.get('reverse', False))

sleepTime = 1./framesPerSecond
imageList = []

if imageFile:
	imageList = [reversed(hyperion.getImage(imageFile))] if reverse else hyperion.getImage(imageFile)

# Start the write data loop
while not hyperion.abort() and imageList:
	for image in imageList:
		hyperion.setImage(image["imageWidth"], image["imageHeight"], image["imageData"])
		time.sleep(sleepTime)
