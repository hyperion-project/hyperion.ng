import hyperion, time, colorsys, random

# Get parameters
sleepTime     = max(0.02, float(hyperion.args.get('speed', 100))/1000.0)
trailLength   = max(3, int(hyperion.args.get('trailLength', 5)))
explodeRadius = int(hyperion.args.get('explodeRadius', 8))

# Ensure that the range for pixel indices stays within bounds
maxPixelIndex = hyperion.ledCount - 1
if trailLength > maxPixelIndex or explodeRadius > maxPixelIndex:
    exit(f"Error: Color length ({trailLength}) and detonation range ({explodeRadius}) must be less than number of LEDs configured ({hyperion.ledCount})")

# Create additional variables
increment = None
projectiles = []

# Initialize the led data
ledData = bytearray()
for i in range(hyperion.ledCount):
    ledData += bytearray((0,0,0))

# Start the write data loop
while not hyperion.abort():
    if len(projectiles) != 2:
        projectiles = [
            [0, 1, random.uniform(0.0, 1.0)],  # Start positions of projectiles
            [hyperion.ledCount-1, -1, random.uniform(0.0, 1.0)]
        ]
        increment = -random.randint(0, hyperion.ledCount-1) if random.choice([True, False]) else random.randint(0, hyperion.ledCount-1)

    # Backup the LED data
    ledDataBuf = ledData[:]
    for i, v in enumerate(projectiles):
        # Update projectile positions
        projectiles[i][0] = projectiles[i][0] + projectiles[i][1]
        
        for t in range(0, trailLength):
            # Calculate pixel index for the trail
            pixel = v[0] - v[1] * t
            if pixel < 0:
                pixel += hyperion.ledCount
            if pixel >= hyperion.ledCount:
                pixel -= hyperion.ledCount

            # Make sure pixel is within bounds
            if pixel < 0 or pixel >= hyperion.ledCount:
                continue

            rgb = colorsys.hsv_to_rgb(v[2], 1, (trailLength - 1.0 * t) / trailLength)
            ledDataBuf[3*pixel] = int(255 * rgb[0])
            ledDataBuf[3*pixel + 1] = int(255 * rgb[1])
            ledDataBuf[3*pixel + 2] = int(255 * rgb[2])

    hyperion.setColor(ledDataBuf[-increment:] + ledDataBuf[:-increment])

    # Check for collision and handle explosion
    for i1, p1 in enumerate(projectiles):
        for i2, p2 in enumerate(projectiles):
            if p1 is not p2:
                prev1 = p1[0] - p1[1]
                prev2 = p2[0] - p2[1]
                if (prev1 - prev2 < 0) != (p1[0] - p2[0] < 0):
                    for d in range(0, explodeRadius):
                        for pixel in range(p1[0] - d, p1[0] + d):
                            # Check if pixel is out of bounds
                            if pixel < 0 or pixel >= hyperion.ledCount:
                                continue

                            rgb = colorsys.hsv_to_rgb(random.choice([p1[2], p2[2]]), 1, (1.0 * explodeRadius - d) / explodeRadius)
                            ledDataBuf[3 * pixel] = int(255 * rgb[0])
                            ledDataBuf[3 * pixel + 1] = int(255 * rgb[1])
                            ledDataBuf[3 * pixel + 2] = int(255 * rgb[2])

                        hyperion.setColor(ledDataBuf[-increment:] + ledDataBuf[:-increment])
                        time.sleep(sleepTime)

                    projectiles.remove(p1)
                    projectiles.remove(p2)

    time.sleep(sleepTime)
