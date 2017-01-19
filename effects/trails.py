import hyperion
import time
import colorsys
import random

min_len = int(hyperion.args.get('min_len', 3))
max_len = int(hyperion.args.get('max_len', 3))
height = int(hyperion.args.get('height', 8))
trails = int(hyperion.args.get('int', 8))
sleepTime = float(hyperion.args.get('speed', 1.0)) * 0.01
color = list(hyperion.args.get('color', (255,255,255)))
randomise = bool(hyperion.args.get('random', False))
whidth = hyperion.ledCount / height

class trail:
  def __init__(self):
    return

  def start(self, x, y, step, color, _len, _h):
    self.pos = 0.0
    self.step = step
    self.h = _h
    self.x = x
    self.data = []
    brigtness = color[2]
    step_brigtness = color[2] / _len
    for i in range(0, _len):
      rgb = colorsys.hsv_to_rgb(color[0], color[1], brigtness)
      self.data.insert(0, (int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2])))
      brigtness -= step_brigtness

    self.data.extend([(0,0,0)]*(_h-y))
    if len(self.data) < _h:
      for i in range (_h-len(self.data)):
        self.data.insert(0, (0,0,0))

  def getdata(self):
    self.pos += self.step
    if self.pos >  1.0:
      self.pos = 0.0
      self.data.pop()
      self.data.insert(0, (0,0,0))
    return self.x, self.data[-self.h:], all(x ==  self.data[0] for x in self.data)

tr = []

for i in range(trails):
  r = {
      'exec': trail()
    }

  if randomise:
    col = (random.uniform(0.1, 1.0), random.uniform(0.1, 1.0), random.uniform(0.1, 1.0))
  else:
    col = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color[2]/255.0)

  r['exec'].start(
            random.randint(0, whidth),
            random.randint(0, height),
            random.uniform(0.2, 0.8),
            col,
            random.randint(min_len, max_len),
            height
            )
  tr.append(r)

# Start the write data loop
while not hyperion.abort():
  ledData = bytearray()

  for r in tr:
    r['x'], r['data'], c = r['exec'].getdata()
    if c:
      if randomise:
        col = (random.uniform(0.1, 1.0), random.uniform(0.1, 1.0), random.uniform(0.1, 1.0))
      else:
        col = colorsys.rgb_to_hsv(color[0]/255.0, color[1]/255.0, color[2]/255.0)

      r['exec'].start(
          random.randint(0, whidth),
          random.randint(0, height),
          random.uniform(0.2, 0.8),
          col,
          random.randint(min_len, max_len),
          height
          )

  for y in range(0, height):
    for x in range(0, whidth):
      for r in tr:
        if x == r['x']:
          led = bytearray(r['data'][y])
          break
        led = bytearray((0,0,0))
      ledData += led

  hyperion.setImage(whidth,height,ledData)
  time.sleep(sleepTime)

