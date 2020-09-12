# Effect Engine API
All available functions for usage.

## API Overview
| Function                          | Returns |   Comment  |
| ------------------------------- | ----- | -------- |
| hyperion.ledCount                 | Integer | Get the current led count from the led layout |
| hyperion.latchTime                | Integer | Get the current active latchtime in ms. |
| hyperion.imageWidth()             | Integer | Get the current image width, calculate positions for elements at the [coordinate system](http://doc.qt.io/qt-5/coordsys.html#rendering)  |
| hyperion.imageHeight()            | Integer | Get the current image height,calculate positions for elements at the [coordinate system](http://doc.qt.io/qt-5/coordsys.html#rendering) |
| hyperion.imageCRotate()           | -       | Rotates the coordinate system at the center (0,0) by the given angle. See [hyperion.imageCRotate()](#hyperion-imagecrotate) |
| hyperion.imageCOffset()           | -       | Add a offset to the coordinate system. See [hyperion.imageCOffset()](#hyperion-imagecoffset) |
| hyperion.imageCShear()            | -       | Shear the coordinate system. See [hyperion.imageCShear()](#hyperion-imagecshear) |
| hyperion.imageResetT()            | -       | Resets all coordination system modifications done with hyperion.imageCRotate(), hyperion.imageCOffset(), hyperion.imageCShear() |
| hyperion.imageMinSize()           | -       | See [hyperion.imageMinSize()](#hyperion-imageminsize)|
| hyperion.abort()                  | Boolean | If true, hyperion requests an effect abort, used in a while loop to repeat effect calculations and writing |
| hyperion.imageConicalGradient()   | -       | See [hyperion.imageConicalGradient()](#hyperion-imageconicalgradient) |
| hyperion.imageRadialGradient()    | -       | See [hyperion.imageRadialGradient()](#hyperion-imageradialgradient)|
| hyperion.imageLinearGradient()    | -       | See [hyperion.imageLinearGradient()](#hyperion-imagelineargradient)|
| hyperion.imageDrawLine()          | -       | See [hyperion.imageDrawLine()](#hyperion-imagedrawline) |
| hyperion.imageDrawPoint()         | -       | See [hyperion.imageDrawPoint()](#hyperion-imagedrawpoint) |
| hyperion.imageDrawPolygon()       | -       | See [hyperion.imageDrawPolygon()](#hyperion-imagedrawpolygon) |
| hyperion.imageDrawPie()           | -       | See [hyperion.imageDrawPie()](#hyperion-imagedrawpie) |
| hyperion.imageDrawRect()          | -       | See [hyperion.imageDrawRect()](#hyperion-imagedrawrect) |
| hyperion.imageSolidFill()         | -       | See [hyperion.imageSolidFill()](#hyperion-imagesolidfill) |
| hyperion.imageShow()              | -       | Hyperion shows the image you created with other `hyperion.image*` functions before. This is always the last step after you created the image with other hyperion.image* function |
| hyperion.imageSetPixel()          | -       | See [hyperion.imageSetPixel()](#hyperion-imagesetpixel) |
| hyperion.imageGetPixel()          | Tuple   | A [Python tuple](https://www.tutorialspoint.com/python/python_tuples.htm) RGB values for the requested position. See [hyperion.imageGetPixel()](#hyperion-imagegetpixel) |
| hyperion.imageSave()              | Integer | Create a snapshot of the current effect image and returns an ID. To display the snapshot do `hyperion.imageShow(ID)`. Snapshots are the _current_ state of the picture |
| hyperion.setColor()               | -       | Not recommended, read why! See [hyperion.setColor()](#hyperion-setcolor) |
| hyperion.setImage()               | -       | hyperion.setImage(width, height, RGB_bytearray) |


### hyperion.imageMinSize()
As the `hyperion.imageWidth()` and `hyperion.imageHeight()` scales with the led layout, you could define a minimum size to get more pixels to work with. Keep in mind that the ratio between width/height depends always on user led setup, you can't force it.
::: warning
Should be called before you start painting!
:::
`hyperion.imageMinSize(pixelX,pixelY)`
| Argument | Type       | Comment |
| ---------- | -------- | ---------------------------------------------------------------------------------- |
| pixelX   | Integer    | Minimum Pixels at the x-axis of the image to draw on with `hyperion.image*` functions |
| pixelY   | Integer    | Minimum Pixels at the y-axis of the image to draw on with `hyperion.image*` functions |

### hyperion.imageCRotate()
Rotates the coordinate system at the center which is 0 at the x-axis and 0 at the y-axis by the given angle clockwise. Note: If you want to move the center of the coordinate system you could use hyperion.imageCOffset(). **The rotation is kept until the effect ends**. \
`hyperion.imageCRotate(angle)`
| Argument | Type       | Comment |
| ---------- | -------- | ----------------------------------------------------- |
| angle   | Integer    | Angle of the rotation between `0` and `360`, clockwise |

### hyperion.imageCOffset()
Add offset to the coordinate system at the x-axis and y-axis.
::: warning
Changes at the coordinate system results in weird behavior of some shorter versions of other hyperion.image* drawing functions
:::
`hyperion.imageCOffset(offsetX, offsetY)`
| Argument | Type       | Comment |
| -------- | ---------- | ----------------------------------------------------- |
| offsetX  | Integer    | Offset which is added to the coordinate system at the x-axis. Positive value moves to the right, negative to the left |
| offsetY  | Integer    | Offset which is added to the coordinate system at the y-axis. Positive value moves to the right, negative to the left |

### hyperion.imageCShear()
Shears the coordinate system at the vertical and horizontal. More info to shearing here: [Shear Mapping](https://en.wikipedia.org/wiki/Shear_mapping)
::: warning
Changes at the coordinate system results in weird behavior of some shorter versions of other hyperion.image* drawing functions
:::
`hyperion.imageCShear(sh, sv)`
| Argument | Type       | Comment |
| -------- | ---------- | -------------------------- |
| sh       | Integer    | Horizontal pixels to shear |
| sv       | Integer    | Vertical pixels to shear. |

### hyperion.imageConicalGradient()
Draws a conical gradient on the image, all arguments are required. Add the arguments in the order of rows below. Short explanation for conical gradient at the QT docs: [Conical Gradient](http://doc.qt.io/qt-5/qconicalgradient.html#details) \
`hyperion.imageConicalGradient(startX, startY, width, height, centerX, centerY, angle, bytearray)`
| Argument | Type       | Comment |
| -------- | ---------- | ----------------------------------------------------- |
| startX    | Integer    | Defines the start point at the x-axis of the rectangle that contains the gradient |
| startY    | Integer    | Defines the start point at the y-axis of the rectangle that contains the gradient |
| width     | Integer    | Defines the width of the rectangle |
| height    | Integer    | Defines the height of the rectangle |
| centerX   | Integer    | Defines the center of the gradient at the x-axis. For the center of the picture use `hyperion.imageWidth()*0.5`, don't forget to surround it with int() or round() |
| centerY   | Integer    | Defines the center of the gradient at the y-axis. For the center of the picture use `hyperion.imageHeight()*0.5`, don't forget to surround it with int() or round() |
| angle     | Integer  | Defines the angle from `0` to `360`. Used to rotate the gradient at the center point. |
| bytearray | ByteArray | bytearray of (position,red,green,blue,alpha,position,red,green,blue,alpha,...). Could be repeated as often you need it, all values have ranges from 0 to 255. The position is a point where the red green blue values are assigned. <br/> **Example:** `bytearray([0,255,0,0,255,255,0,255,0,255])` - this is a gradient which starts at 0 with color 255,0,0 and alpha 255 and ends at position 255 with color 0,255,0 and alpha 255. The colors in between are interpolation, so this example is a color shift from red to green from 0° to 360°. |

::: tip Shorter versions of hyperion.imageConicalGradient()
`hyperion.imageConicalGradient(centerX, centerY, angle, bytearray)` -> startX and startY are 0 and the width/height is max. -> Entire image
:::

### hyperion.imageRadialGradient()
Draws a radial gradient on the image. Add the arguments in the order of rows below. All arguments are required.
Short description at QT Docs: [Radial Gradient](http://doc.qt.io/qt-5/qradialgradient.html#details) \
`hyperion.imageRadialGradient(startX, startY, width, height, centerX, centerY, radius, focalX, focalY, focalRadius, bytearray, spread)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| startX    | Integer    | start point at the x-axis of the rectangle which contains the gradient. |
| startY    | Integer    | start point at the y-axis of the rectangle which contains the gradient. |
| width     | Integer    | width of the rectangle. |
| height    | Integer    | height of the rectangle. |
| centerX   | Integer    | Defines the center at the x-axis of the gradient. For the center of the picture use `hyperion.imageWidth()*0.5`, don't forget to surround it with int() or round() |
| centerY   | Integer    | Defines the center at the y-axis of the gradient. For the center of the picture use `hyperion.imageHeight()*0.5`, don't forget to surround it with int() or round() |
| radius    | Integer    | Defines the radius of the gradient in pixels |
| focalX    | Integer    | Defines the focal point at the x-axis |
| focalY    | Integer    | Defines the focal point at the y-axis |
|focalRadius| Integer    | Defines the radius of the focal point |
| bytearray | ByteArray  | bytearray of (position,red,green,blue,position,red,green,blue,...). Could be repeated as often you need it, all values have ranges from 0 to 255. The position is a point where the red green blue values are assigned <br/> **Example:** `bytearray([0,255,0,0,255,0,255,0])` - this is a gradient which starts at 0 with color 255,0,0 and ends at position 255 with color 0,255,0. The colors in between are interpolation, so this example is a color shift from red to green. |
| spread    | Integer    | Defines the spread method outside the gradient. Available spread modes are: <br/> `0` -> The area is filled with the closest stop color <br/> `1` -> The gradient is reflected outside the gradient area <br/> `2` -> The gradient is repeated outside the gradient area <br/> Please note that outside means _inside_ the rectangle but outside of the gradient start and end points, so if these points are the same, you don't see the spread mode. A picture to the spread modes can you find here: [Spread modes](http://doc.qt.io/qt-5/qradialgradient.html#details) |

::: tip Shorter versions of hyperion.imageRadialGradient()
 - `hyperion.imageRadialGradient(startX, startY, width, height, centerX, centerY, radius, bytearray, spread)` -> focalX, focalY, focalRadius get their values from centerX, centerY and radius
 - `hyperion.imageRadialGradient(centerX, centerY, radius, focalX, focalY, focalRadius, bytearray, spread)` -> startX and startY are 0
 - `hyperion.imageRadialGradient(centerX, centerY, radius, bytearray, spread)` -> startX and startY are 0 & focalX, focalY, focalRadius get their values from centerX, centerY and radius
:::

### hyperion.imageLinearGradient()
Draws a linear gradient on the image. Add the arguments in the order of rows below. All arguments are required.
Short description at QT Docs: [Linear Gradient](http://doc.qt.io/qt-5/qlineargradient.html#details) \
`hyperion.imageLinearGradient(startRX, startRY, width, height, startX, startY, endX, endY, bytearray, spread)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| startRX   | Integer    | start point at the x-axis of the rectangle which contains the gradient. |
| startRY   | Integer    | start point at the y-axis of the rectangle which contains the gradient. |
| width     | Integer    | width of the rectangle. |
| height    | Integer    | height of the rectangle. |
| startX    | Integer    | Defines the start at the x-axis for the gradient. |
| startY    | Integer    | Defines the start at the y-axis for the gradient. |
| endX      | Integer    | Defines the end at the x-axis for the gradient. |
| endY      | Integer    | Defines the end at the y-axis for the gradient. |
| bytearray | ByteArray  | bytearray of (position,red,green,blue,alpha,position,red,green,blue,alpha,...). Could be repeated as often you need it, all values have ranges from 0 to 255. The position is a point where the red green blue values are assigned. <br/> **Example:** `bytearray([0,255,0,0,255,255,0,255,0,127])` this is a gradient which starts at 0 with color 255,0,0 and alpha 255 and ends at position 255 with color 0,255,0 and alpha 127. The colors in between are interpolation, so this example is a color shift from red to green. |
| spread    | Integer    | Defines the spread method outside the gradient. Available spread modes are: <br/> `0` -> The area is filled with the closest stop color <br/> `1` -> The gradient is reflected outside the gradient area <br/> `2` -> The gradient is repeated outside the gradient area <br/> Please note that outside means _inside_ the rectangle but outside of the gradient start and end points, so if these points are the same, you don't see the spread mode. A picture to the spread modes can you find here: [Spread modes](http://doc.qt.io/qt-5/qlineargradient.html#details) |

::: tip Shorter versions of hyperion.imageLinearGradient()
`hyperion.imageLinearGradient(startX, startY, endX, endY, bytearray, spread)` -> The rectangle which contains the gradient defaults to the full image
:::

### hyperion.imageDrawLine()
Draws a line at the image. All arguments are required, exception a for alpha. Add the arguments in the order of rows below. \
`hyperion.imageDrawLine(startX, startY, endX, endY, thick, r, g, b, a)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| startX    | Integer    | start point at the x-axis. Relates to `hyperion.imageWidth()` |
| startY    | Integer    | start point at the y-axis. Relates to `hyperion.imageHeight()` |
| endX      | Integer    | end point at the x-axis. Relates to `hyperion.imageWidth()` |
| endY      | Integer    | end point at the y-axis. Relates to `hyperion.imageHeight()` |
| thick     | Integer    | Thickness of the line, should be calculated based on image height or width. But at least one Pixel. Example: `max(int(0.1*hyperion.imageHeight(),1)` is 10% of the image height. |
| r         | Integer    | red color from `0` to `255` |
| g         | Integer    | green color from `0` to `255` |
| b         | Integer    | blue color from `0` to `255` |
| a         | Integer    | **Optional** alpha of the color from `0` to `255`, if not provided, it's `255` |

::: tip Shorter versions of hyperion.imageLinearGradient()
`hyperion.imageLinearGradient(startX, startY, endX, endY, bytearray, spread)` -> The rectangle which contains the gradient defaults to the full image
:::

### hyperion.imageDrawPoint()
Draws a point/dot at the image. All arguments are required, exception a for alpha. Add the arguments in the order of rows below. \
`hyperion.imageDrawPoint(x, y, thick, r, g, b, a)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| x         | Integer    | point position at the x-axis. Relates to `hyperion.imageWidth()` |
| y         | Integer    | point position at the y-axis. Relates to `hyperion.imageHeight()` |
| thick     | Integer    | Thickness of the point in pixel, should be calculated based on image height or width. But at least one Pixel. Example: `max(int(0.1*hyperion.imageHeight(),1)` is 10% of the image height. |
| r         | Integer    | red color from `0` to `255` |
| g         | Integer    | green color from `0` to `255` |
| b         | Integer    | blue color from `0` to `255` |
| a         | Integer    | **Optional** alpha of the color from `0` to `255`, if not provided, it's `255` |

::: tip Shorter versions of hyperion.imageDrawPoint()
`hyperion.imageDrawPoint(x, y, thick, r, g, b)` -> alpha defaults to 255
:::

### hyperion.imageDrawPolygon()
Draws a polygon at the image and fills it with the specific color. Used for free forming (triangle, hexagon,... whatever you want  ). All arguments are required, exception a for alpha. Add the arguments in the order of rows below. \
`hyperion.imageDrawPolygon(bytearray, r, g, b, a)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| bytearray | ByteArray  | bytearray([point1X,point1Y,point2X,point2Y,point3X,point3Y,...]). Add pairs of X/Y coordinates to specific the corners of the polygon, each point has a X and a Y coordinate, you could add as much points as you need. The last point automatically connects to the first point.|
| r         | Integer    | red color from `0` to `255` |
| g         | Integer    | green color from `0` to `255` |
| b         | Integer    | blue color from `0` to `255` |
| a         | Integer    | **Optional** alpha of the color from `0` to `255`, if not provided, it's `255` |

::: tip Shorter versions of hyperion.imageDrawPolygon()
`hyperion.imageDrawPolygon(bytearray, r, g, b)` -> alpha defaults to 255
:::

### hyperion.imageDrawPie()
Draws a pie (also known from pie charts) at the image and fills it with the specific color. All arguments are required, exception a for alpha. Add the arguments in the order of rows below. \
`hyperion.imageDrawPie(centerX, centerY, radius, startAngle, spanAngle, r, g, b, a)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| centerX   | Integer    | The center of the Pie at the x-axis |
| centerY   | Integer    | The center of the Pie at the y-axis |
| radius    | Integer    | radius of the Pie in Pixels |
| startAngle| Integer    | start angle from `0` to `360`. `0` is at 3 o'clock |
| spanAngle | Integer    | span (wide) of the pie from `-360` to `360` which starts at the startAngle, positive values are counter-clockwise, negative clockwise |
| r         | Integer    | red color from `0` to `255` |
| g         | Integer    | green color from `0` to `255` |
| b         | Integer    | blue color from `0` to `255` |
| a         | Integer    | **Optional** alpha of the color from `0` to `255`, if not provided, it's `255` |

::: tip Shorter versions of hyperion.imageDrawPie()
`hyperion.imageDrawPie(centerX,  centerY,  radius,  startAngle,  spanAngle,  r,  g,  b)` -> alpha defaults to 255
:::

### hyperion.imageDrawRect()
Draws a rectangle on the image. All arguments are required, exception a for alpha. Add the arguments in the order of rows below. \
`hyperion.imageDrawRect(startX, startY, width, height, thick, r, g, b, a,)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| startX    | Integer    | start point at the x-axis. Relates to `hyperion.imageWidth()` |
| startY    | Integer    | start point at the y-axis. Relates to `hyperion.imageHeight()` |
| width     | Integer    | width of the rectangle. Relates to `hyperion.imageWidth()` |
| height    | Integer    | height of the rectangle. Relates to `hyperion.imageHeight()` |
| thick     | Integer    | Thickness of the rectangle, a good start value is `1` |
| r         | Integer    | define red color from `0` to `255` |
| g         | Integer    | define green color from `0` to `255` |
| b         | Integer    | define blue color from `0` to `255` |
| a         | Integer    | **Optional** alpha of the color from `0` to `255`, if not provided, it's `255` |

### hyperion.imageSolidFill()
Fill a specific part of the image with a solid color (or entire). All arguments are required. Add the arguments in the order of rows below. \
`hyperion.imageSolidFill(startX, startY, width, height, r, g, b, a)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| startX    | Integer    | start point at the x-axis. Relates to `hyperion.imageWidth()` |
| startY    | Integer    | start point at the y-axis. Relates to `hyperion.imageHeight()` |
| width     | Integer    | width of the fill area. Relates to `hyperion.imageWidth()` |
| height    | Integer    | height of the fill area. Relates to `hyperion.imageHeight()` |
| r         | Integer    | define red color from `0` to `255` |
| g         | Integer    | define green color from `0` to `255` |
| b         | Integer    | define blue color from `0` to `255` |
| a         | Integer    | alpha of the color from `0` to `255` |

::: tip Shorter versions of hyperion.imageSolidFill()
 - `hyperion.imageSolidFill(startX, startY, width, height, r, g, b)` -> no alpha, defaults to 255
 - `hyperion.imageSolidFill(r, g, b, a)` ->  startX and startY is 0, width and height is max. -> full image
 - `hyperion.imageSolidFill(r, g, b)` ->  startX and startY is 0, width and height is max, alpha 255. -> full image
:::

### hyperion.imageSetPixel()
Assign a color to a specific pixel position. All arguments are required. Add the arguments in the order of rows below. \
`hyperion.imageSetPixel(X, Y, r, g, b)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| X         | Integer    | pixel point at the x-axis. Relates to `hyperion.imageWidth()` |
| Y         | Integer    | pixel point at the y-axis. Relates to `hyperion.imageHeight()` |
| r         | Integer    | define red color from `0` to `255` |
| g         | Integer    | define green color from `0` to `255` |
| b         | Integer    | define blue color from `0` to `255` |

### hyperion.imageGetPixel()
Get a color of a specific pixel position. All arguments are required. Add the arguments in the order of rows below. \
`hyperion.imageGetPixel(X, Y)`
| Argument  | Type       |  Comment  |
| --------- | ---------- | ----------------------------------------------------- |
| X         | Integer    | pixel point at the x-axis. Relates to `hyperion.imageWidth()` |
| Y         | Integer    | pixel point at the y-axis. Relates to `hyperion.imageHeight()` |
| Return   | Tuple    | Returns a Python Tuple of RGB values |



### hyperion.setColor()
Set a single color to all leds by adding `hyperion.setColor(255,0,0)`, all leds will be red. But it is also possible to send a bytearray of RGB values. Each RGB value in this bytearray represents one led.
 - **Example 1:** `hyperion.setColor(bytearray([255,0,0]))` The first led will be red
 - **Example 2:** `hyperion.setColor(bytearray([255,0,0,0,255,0]))` The first led will be red, the second is green
 - **Example 3:** `hyperion.setColor(bytearray([255,0,0,0,255,0,255,255,255]))` The first led will be red, the second is green, the third is white
 - You usually assign to all leds a color, therefore you need to know how much leds the user currently have. Get it with `hyperion.ledCount`

::: warning hyperion.setColor()
 - hyperion.setColor() function is not recommended to assign led colors, it doesn't work together with **`hyperion.image*`** functions
 - You don't know where is top/left/right/bottom and it doesn't work with matrix layouts!
 - Please consider to use the **`hyperion.image*`** functions instead to create amazing effects that scales with the user setup
:::