# Control
You can control Hyperion by sending specific JSON messages.

::: tip
The `tan` property is supported in these calls, but omitted for brevity.
:::

[[toc]]

## Sections

### Set Color
Set a color for all leds or provide a pattern of led colors.

| Property |  Type   | Required |                                                     Comment                                                      |
| :------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
|  color   |  Array  |   true   |                                 An array of R G B Integer values e.g. `[R,G,B]`                                  |
| duration | Integer |  false   |                  Duration of color in ms. If you don't provide a duration, it's `0` -> indefinite                |
| priority | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin  | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

```json
// Example: Set a blue color with indefinite duration at priority 50 
{
  "command":"color",
  "color":[0,0,255],
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set a cyan color for 12 seconds at priority 20
{
  "command":"color",
  "color":[0,255,255],
  "duration":12000,
  "priority":20,
  "origin":"My Fancy App"
}

// Example: Provide a color pattern, which will be repeated until all LEDs have a color
// In this case LED 1: Red, LED 2: Red, LED 3: Blue.
{
  "command":"color",
  "color":[255,0,0,255,0,0,0,0,255], // one led has 3 values (Red,Green,Blue) with range of 0-255
  "duration":12000,
  "priority":20,
  "origin":"My Fancy App"
}
```

### Set Effect
Set an effect by name. Available names can be found in the [serverinfo effect list](/en/json/ServerInfo.md#effect-list).

| Property |  Type   | Required |                                                     Comment                                                      |
| :------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
|  effect  | Object  |   true   |                          Object with additional properties. e.g. `"name":"EffectName"`.                          |
| duration | Integer |  false   |                  Duration of effect in ms. If you don't provide a duration, it's `0` -> indefinite               |
| priority | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin  | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

```json
// Example: Set the 'Warm mood blobs' effect with indefinite duration
{
  "command":"effect",
  "effect":{
    "name":"Warm mood blobs"
  },
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set 'Rainbow swirl' effect for 5 seconds
{
  "command":"effect",
  "effect":{
    "name":"Rainbow swirl"
  },
  "duration":5000,
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set 'Rainbow swirl' effect for 1 second with overridden agrument
// Each effect has different agruments inside the args property that can be overridden.
// WARNING: We highly recommend using the effects configurator in the UI instead. Sending invalid values may cause the effect to misbehave or crash.
{
  "command":"effect",
  "effect":{
    "name":"Rainbow swirl",
    "args":{
      "rotation-time":1
    }
  },
  "duration":5000,
  "priority":50,
  "origin":"My Fancy App"}
```

### Set Image
Set a single image. Supports all [Qt5](https://doc.qt.io/qt-5/qimagereader.html#supportedImageFormats) image formats, including png/jpg/gif.

| Property  |  Type   | Required |                                                     Comment                                                      |
| :-------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
| imagedata | String  |   true   |                         Data of image as [Base64](https://en.wikipedia.org/wiki/Base64)                          |
|  format   | String  |   true   |                         Set to `auto` to let Hyperion parse the image according to type                          |
|   name    | String  |   true   |                                              The name of the image                                               |
| duration  | Integer |  false   |                  Duration of image in ms. If you don't provide a duration, it's `0` -> endless                   |
| priority  | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin   | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

```json
// Set an image for 5 seconds
{
  "command":"image",
  "imagedata":"VGhpcyBpcyBubyBpbWFnZSEgOik=", // as base64!
  "name":"Name of Image",
  "format":"auto",
  "priority":50,
  "duration":5000,
  "origin": "My Fancy App"
}
```

### Clear
Clear a priority, usually used to revert [set color](#set-color), [set
effect](#set-effect) or [set image](#set-image).
```json
// Clear effect/color/image with priority 50
{
  "command":"clear",
  "priority":50,
}
// Clear all effects/colors/images
{
  "command":"clear",
  "priority":-1,
}
```
::: warning
When you clear all, you clear all effects and colors regardless of who set them!
Instead, we recommend users provide a list of possible clear targets based on a
priority list
:::

### Adjustments
Adjustments reflect the color calibration. You can modify all properties of [serverinfo adjustments](/en/json/serverinfo#adjustments).

|        Property        |      Type      | Required |                                            Comment                                             |
| :--------------------: | :------------: | :------: | :--------------------------------------------------------------------------------------------: |
|          red           |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|         green          |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|          blue          |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|          cyan          |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|        magenta         |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|         yellow         |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|         white          |     Array      |  false   |                        An array of R G B Integer values e.g. `[R,G,B]`                         |
|        gammaRed        | Number (float) |  false   |                           minimum:`0.1` maximum `5.0` step of `0.1`                            |
|       gammaGreen       | Number (float) |  false   |                           minimum:`0.1` maximum `5.0` step of `0.1`                            |
|       gammaBlue        | Number (float) |  false   |                           minimum:`0.1` maximum `5.0` step of `0.1`                            |
|       brightness       |    Integer     |  false   |                             minimum: `0` maximum `100` step of `1`                             |
| brightnessCompensation |    Integer     |  false   |                             minimum: `0` maximum `100` step of `1`                             |
|   backlightThreshold   |    Integer     |  false   | minimum: `0` maximum `100`. Step of `1`. (Minimum brightness!) Disabled for effect/color/image |
|    backlightColored    |    Boolean     |  false   |    If `true` the backlight is colored, `false` it's white. Disabled for effect/color/image     |
|           id           |     String     |  false   |                                        Short identifier                                        |

```json
// Example: Set gammaRed to 1.5
{
  "command":"adjustment",
  "adjustment":{
    "gammaRed":1.5
  }
}
// Example: Set green to [0,236,0]
{
  "command":"adjustment",
  "adjustment":{
    "green":[0,236,0]
  }
}
// Example: Set backlightColored to true
{
  "command":"adjustment",
  "adjustment":{
    "backlightColored":true
  }
}
// Example: Send the 3 examples above at once
{
  "command":"adjustment",
  "adjustment":{
    "backlightColored":true,
    "gammaRed":1.5,
    "green":[0,236,0]
  }
}
```

### LED mapping
Switch the image to led mapping mode. Possible values are `unicolor_mean` (led color based on whole picture color) and `multicolor_mean` (led colors based on led layout)
```json
// Example: Set mapping mode to multicolor_mean
{
  "command":"processing",
  "mappingType":"multicolor_mean"
}
// Example: Set mapping mode to unicolor_mean
{
  "command":"processing",
  "mappingType":"unicolor_mean"
}
```

### Video Mode
Switch the video mode. Possible values are: `2D`, `3DSBS` and `3DTAB`.
 ```json
// Example: Set video mode to 3DTAB
{
  "command":"videomode",
  "videoMode":"3DTAB"
}
// Example: Set video mode to 3DSBS
{
  "command":"videomode",
  "videoMode":"3DSBS"
}
```

### Control Components
Some components can be enabled and disabled at runtime. To get the current state and
available components see [Serverinfo Components](/en/json/serverinfo#components). See
also: [Components/IDs explained](#components-ids-explained)

 ```json
// Example: disable LEDDEVICE component
{
  "command":"componentstate",
  "componentstate":{
    "component":"LEDDEVICE",
    "state":false
  }
}
// Example: enable SMOOTHING component
{
  "command":"componentstate",
  "componentstate":{
    "component":"SMOOTHING",
    "state":true
  }
}
```
::: warning
Hyperion itself needs to be enabled! Check the status of "ALL" in the components list before you change another component!
:::

### Components/IDs explained
Each component has a unique id. Not all of them can be enabled/disabled -- some of them,
such as effect and color, are used to determine the source type when examining the
[priority list](/en/json/ServerInfo.html#priorities).
|  ComponentID   |      Component       | Enable/Disable |                                    Comment                                    |
| :------------: | :------------------: | :------------: | :---------------------------------------------------------------------------: |
|   SMOOTHING    |      Smoothing       |      Yes       |                              Smoothing component                              |
|  BLACKBORDER   | Blackborder detector |      Yes       |                         Black bar detection component                         |
|   FORWARDER    | Json/Proto forwarder |      Yes       |                        Json/Proto forwarder component                         |
| BOBLIGHTSERVER |   Boblight server    |      Yes       |                           Boblight server component                           |
|    GRABBER     |   Platform capture   |      Yes       |                          Platform Capture component                           |
|      V4L       |  V4L capture device  |      Yes       |                         USB capture device component                          |
|   LEDDEVICE    |      Led device      |      Yes       |     Led device component start/stops output of the configured led device      |
|      ALL       |  SPECIAL: Hyperion   |      Yes       | Enable or disable Hyperion. Recovers/saves last state of all other components |
|     COLOR      |     Solid color      |       No       |            All colors that has been set belongs to this component             |
|     EFFECT     |        Effect        |       No       |                     All effects belongs to this component                     |
|     IMAGE      |     Solid Image      |       No       |          All single/solid images belongs to this. NOT for streaming           |
| FLATBUFSERVER  |  Flatbuffers Server  |       No       |                All image stream sources from flatbuffer server                |
|  PROTOSERVER   |  Protobuffer Server  |       No       |               All image stream sources from Protobuffer server                |


### Source Selection
Sources are always selected automatically by priority value (lowest value is the highest
priority). You need to know the priority value of the source you want to select -- these
priority values are available in the [serverinfo
Priorities](/en/json/serverinfo#priorities).
```json
// Example: Set priority 50 to visible
{
  "command":"sourceselect",
  "priority":50
}
```
If you get a success response, the `priorities_autoselect`-status will switch to false (see [serverinfo Autoselection Mode](/en/json/serverinfo##priorities-selection-auto-manual)). You are now in manual mode, to switch back to auto mode send:
```json
{
  "command":"sourceselect",
  "auto":true
}
```
After which, the `priorities_autoselect`-status will return to `true`.

::: warning
You can only select priorities which are `active:true`!
:::

### Control Instances
An instance represents an LED hardware instance. It runs within its own scope with it's
own plugin settings, led layout and calibration. Before selecting you can instance, you
should first get information about the available instances from [serverinfo](/en/json/serverinfo#instance).

```json
// Command to start instance 1
{
  "command" : "instance",
  "subcommand" : "startInstance",
  "instance" : 1
}

// Command to stop instance 1
{
  "command" : "instance",
  "subcommand" : "stopInstance",
  "instance" : 1
}
```

### API Instance handling
On connection to the API you will be connected to instance `0` by default. You can
control only one instance at the same time within a single connection, and
[subscriptions](/en/json/subscribe#instance-updates) are in the context of the selected instance.

It's possible to switch to another instance with the following command:

```json
// Switch to instance 1
{
  "command" : "instance",
  "subcommand" : "switchTo",
  "instance" : 1
}
```
This will return a success response or an error if the instance isn't available.

::: warning
It's possible that an instance will stop while you are connected to it. In this case
connections on that instance will automatically be reset to instance `0`. Keep watching
the instance data via subscription if you need to handle this case.
See: [Instance updates](/en/json/subscribe#instance-updates).
:::

### Live Image Stream
You can request a live image stream (if the current source priority supports it,
otherwise here may be no response).
```json
{
  "command":"ledcolors",
  "subcommand":"imagestream-start"
}
```
You will receive "ledcolors-imagestream-update" messages with a base64 encoded image.
Stop the stream by sending:
```json
{
  "command":"ledcolors",
  "subcommand":"imagestream-stop"
}
```
::: danger HTTP/S
This feature is not available for HTTP/S JSON-RPC
:::


### Live Led Color Stream
You can request a live led color stream with current color values in RGB for each single
led. The update rate is 125ms.
```json
{
  "command":"ledcolors",
  "subcommand":"ledstream-start"
}
```
You will receive "ledcolors-ledstream-update" messages with an array of all led colors.
Stop the stream by sending:
```json
{
  "command":"ledcolors",
  "subcommand":"ledstream-stop"
}
```
::: danger HTTP/S
This feature is not available for HTTP/S JSON-RPC
:::
