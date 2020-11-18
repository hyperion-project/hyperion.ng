# Advanced
Specific topics with details

[[TOC]]

## LED Layout
Hyperion assigns each single led a specific position at the picture. These positions are squares and to create a square you need 4 values (top edge, bottom edge, left edge, right edge). These edges are reflected in `hmin`, `hmax` for horizontal and `vmin`, `vmax` for vertical. They have a value range from `0.0` to `1.0`.

<ImageWrap src="/images/en/user_ledlayout.jpg" alt="Hyperion Led Layout">
Assignment of LED edges

</ImageWrap>

So let's have a closer look. Following a single led definition:
``` json
{
  "hmax": 0.2,
  "hmin": 0,
  "vmax": 0.2,
  "vmin": 0
}
```
Let's visualize the example above!

<ImageWrap src="/images/en/user_ledlayout1.jpg" alt="Hyperion Led Layout">
A single led definition

</ImageWrap>

So let us add 2 more leds to make it more clear \
**The order is important! The first entry is the first led, the second the second led, ...**
``` json
{
  "hmax": 0.2,
  "hmin": 0,
  "vmax": 0.2,
  "vmin": 0
},
{
  "hmax": 0.5,
  "hmin": 0.3,
  "vmax": 0.5,
  "vmin": 0.3
},
{
  "hmax": 1.0,
  "hmin": 0.7,
  "vmax": 1,
  "vmin": 0.7
}
```
<ImageWrap src="/images/en/user_ledlayout2.jpg" alt="Hyperion Led Layout">
Now with three LEDs

</ImageWrap>

### Additional properties
You may connected different led stripe charges with different RGB byte orders. You can overwrite the global RGB byte order by adding a `colorOrder` property to all leds that require a different one.
``` json
{
  "hmax": 0.2,
  "hmin": 0,
  "vmax": 0.2,
  "vmin": 0,
  "colorOrder":"gbr"
},
{
  "hmax": 0.5,
  "hmin": 0.3,
  "vmax": 0.5,
  "vmin": 0.3
},
{
  "hmax": 1.0,
  "hmin": 0.7,
  "vmax": 1,
  "vmin": 0.7
}
```
In this example the first led will be `gbr`, the other leds will be assigned to the global RGB order that has been defined at the led hardware section.

### Edit with Web Configuration
While editing these values in a local texteditor is a little bit weird, you could edit them at the web configuration!
  - Make sure you raised the [Hyperion Settings level](../Configuration.md#settings-level) to **Advanced**.
  - Navigate to Configuration -> LED Hardware and switch to the LED Layout tab. You will notice a new section **Generated/Current LED Configuration**.
<ImageWrap src="/images/en/user_ledlayout3.jpg" alt="Hyperion Led Layout" />

You could freely edit the values, show a preview on the right side by clicking **Update Preview**. When you are happy with the changes don't forget to save.

## Blackbar detection
Explain the differences between the available modes for blackbar detection.

  * **Default:** 3 scanlines in each direction (X Y) - fastest detection
  * **Classic:** The original implementation - lower cpu time (legacy for RPi 1) just scan the top one third of the picture which leads to a  slow detection and trouble with TV channel logo.
  * **OSD:** Based on the default mode - not that effective but prevents border switching which may caused of OSD overlays (program infos and volume bar).
  * **Letterbox:** Based on the default mode - only considers blackbars at the top and bottom of the picture, ignoring the sides.
<ImageWrap src="/images/en/user_bbmodes.jpg" alt="Hyperion Blackbar detection modes" />

## Gamma Curve
 Gamma values in a graphic. AS you see 1.0 is neutral. Lower than 1.0 increase the color, higher reduce color. 
 <ImageWrap src="/images/en/user_gammacurve.png" alt="Hyperion Gamma Curve" />


## CLI
All executables shipped with Hyperion have some command line arguments/options

### hyperiond
The heart of Hyperion
``` sh
# Show the version/build date/commit of Hyperion 
hyperiond --version

# Reset current adminstration password back to 'hyperion'
hyperiond --resetPassword

# Overwrite the path for user data (which defaults to your home directory)
hyperiond --userdata /temp/anotherDir

# Overwrite log level temporarily: hyperiond -s for silent -v for verbose and -d for debug
hyperiond -d

# Export effects to directory
hyperiond --export-effects /tmp

# Run Hyperion in desktop mode
hyperiond --desktop
```

::: tip
If a path name contains spaces, surround it with `“`.
`hyperiond --userdata "/temp/another Dir"`
:::

### hyperion-remote
hyperion-remote is a command line tool which translates given arguments to JSON commands and sends them to the Hyperion JSON-RPC. Easy to use for scripts. It supports nearly all commands that Hyperion provides.

``` sh
# Get a list of all available commands
hyperion-remote -h

# Set a color by using HTML color names
hyperion-remote -c aqua

# Set color with hex value
hyperion-remote -c FF7F50

# Set color with a duration of 5 seconds instead endless 
hyperion-remote -c FF7F50 -d 5000

# Start an effect
hyperion-remote -e "Rainbow swirl"

# with a duration of 8 seconds instead endless
hyperion-remote -e "Rainbow swirl" -d 8000

# Target a specific instance
# ATTENTION: Hyperion instances will synchronize with the Instance Syncing feature by default
# You can configure the behaviour for each instance
hyperion-remote -I "My cool instance name"
# Or
hyperion-remote --instance "My cool instance name"
# Example set effect for instance
hyperion-remote --instance "My cool instance name" -e "Rainbow swirl"
```

::: tip
Hyperion remote will search for a Hyperion server automatically. So you can even use that on another device in your local network without providing a ip/port
:::

### hyperion-capture
 We deliver also stand alone capture apps right in your Hyperion directory. They are called hyperion-dispmanx, hyperion-osx, hyperion-x11, hyperion-amlogic, hyperion-framebuffer, hyperion-qt. Depending on your platform you have more or less.

All these application can be started independent from Hyperion and all of these have slightly different options. They communicate with the flatbuffer interface of Hyperion. So let's start one of them! In this example i use dispmanx for Raspberry Pi, so let us check the available options.

``` sh
hyprion-dispmanx -h
  -f, --framerate <framerate>  Capture frame rate [default: 10]
  --width <width>              Width of the captured image [default: 64]
  --height <height>            Height of the captured image [default: 64]
  --screenshot                 Take a single screenshot, save it to file and quit
  -a, --address <address>      Set the address of the hyperion server [default: 127.0.0.1:19445]
  -p, --priority <priority>    Use the provided priority channel (suggested 100-199) [default: 150]
  --skip-reply                 Do not receive and check reply messages from Hyperion
  -h, --help                   Show this help message and exit
  --crop-left <crop-left>      pixels to remove on left after grabbing
  --crop-right <crop-right>    pixels to remove on right after grabbing
  --crop-top <crop-top>        pixels to remove on top after grabbing
  --crop-bottom <crop-bottom>  pixels to remove on bottom after grabbing
  --3DSBS                      Interpret the incoming video stream as 3D side-by-side
  --3DTAB                      Interpret the incoming video stream as 3D top-and-bottom

# Let's start with capture interval of 15, width of 100 and a height of 100
hyperion-dispmanx -a 192.168.0.20:19445 -f 15 --width 100 --height 100
```
