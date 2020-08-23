# Control
You can control Hyperion by sending specific JSON messages. Or get a image and led colors stream of the current active source priority.

::: tip
The `tan` property is supported, but omitted.
:::

[[toc]]

## Sections

### Set Color
Set a color for all leds or provide a pattern of led colors.

| Property |  Type   | Required |                                                     Comment                                                      |
| :------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
|  color   |  Array  |   true   |                                 An array of R G B Integer values e.g. `[R,G,B]`                                  |
| duration | Integer |  false   |                  Duration of color in ms. If you don't provide a duration, it's `0` -> endless                   |
| priority | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin  | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

``` json
// Example: Set color blue with endless duration at priority 50 
{
  "command":"color",
  "color":[0,0,255],
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set color cyan for 12 seconds at priority 20
{
  "command":"color",
  "color":[0,255,255],
  "duration":12000,
  "priority":20,
  "origin":"My Fancy App"
}

// Example: Provide a color pattern, which will be reapted until all LEDs have a color
// In this case LED 1: Red, LED 2: Red, LED 3: Blue. This repeats now
{
  "command":"color",
  "color":[255,0,0,255,0,0,0,0,255], // one led has 3 values (Red,Green,Blue) with range of 0-255
  "duration":12000,
  "priority":20,
  "origin":"My Fancy App"
}
```

### Set Effect
Set an effect by name. Get a name from [Serverinfo](/en/json/ServerInfo.md#effect-list)

| Property |  Type   | Required |                                                     Comment                                                      |
| :------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
|  effect  | Object  |   true   |                          Object with additional properties. e.g. `"name":"EffectName"`.                          |
| duration | Integer |  false   |                  Duration of effect in ms. If you don't provide a duration, it's `0` -> endless                  |
| priority | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin  | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

``` json
// Example: Set effect Warm mood blobs with endless duration
{
  "command":"effect",
  "effect":{
    "name":"Warm mood blobs"
  },
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set effect Rainbow swirl for 5 seconds
{
  "command":"effect",
  "effect":{
    "name":"Rainbow swirl"
  },
  "duration":5000,
  "priority":50,
  "origin":"My Fancy App"
}
// Example: Set effect Rainbow swirl for 1 seconds with overwritten agrument
// Each effect has different agruments inside the args property that can be overwritten
// WARNING: We highly recommend to use the effects configurator instead. As you can send wrong values and the effect can crash/ behave strange
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
Set a single image. Supported are all [Qt5](https://doc.qt.io/qt-5/qimagereader.html#supportedImageFormats) image formats, including png/jpg/gif.

| Property  |  Type   | Required |                                                     Comment                                                      |
| :-------: | :-----: | :------: | :--------------------------------------------------------------------------------------------------------------: |
| imagedata | String  |   true   |                         Data of image as [Base64](https://en.wikipedia.org/wiki/Base64)                          |
|  format   | String  |   true   |                         Set to `auto` to let Hyperion parse the image according to type                          |
|   name    | String  |   true   |                                              The name of the image                                               |
| duration  | Integer |  false   |                  Duration of image in ms. If you don't provide a duration, it's `0` -> endless                   |
| priority  | Integer |   true   | We recommend `50`, following the [Priority Guidelines](/en/api/guidelines#priority_guidelines). Min `2` Max `99` |
|  origin   | String  |   true   |              A short name of your application like `Hyperion of App` . Max length is `20`, min `4`.              |

``` json
// Set a image for 5 seconds
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
Clear a priority, usually used to revert these: [set color](#set-color), [set effect](#set-effect), [set image](#set-image)
``` json
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
When you clear all, you clear all effects and colors independent who set it! We recommend to provide a list of possible clear targets instead based on the priority list
:::

### Adjustments
Adjustments reflect the color calibration. You can modify all properties of [serverinfo adjustments](/en/json/serverinfo#adjustments)

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

``` json
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
Switch the image to led mapping mode. Available are `unicolor_mean` (led color based on whole picture color) and `multicolor_mean` (led colors based on led layout)
``` json
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
Switch the video mode between 2D, 3DSBS, 3DTAB.
 ``` json
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
It is possible to enable and disable certain components during runtime.
To get the current state and available components See [serverinfo Components](/en/json/serverinfo#components).
See also: [Components/IDs explained](#components-ids-explained)

 ``` json
// Example: disable component LEDDEVICE
{
  "command":"componentstate",
  "componentstate":{
    "component":"LEDDEVICE",
    "state":false
  }
}
// Example: enable component SMOOTHING
{
  "command":"componentstate",
  "componentstate":{
    "component":"SMOOTHING",
    "state":true
  }
}
```
::: warning
Hyperion needs to be enabled! Check the status of "ALL" at the components list before you change another component!
:::

### Components/IDs explained
Each component has a unique id. Not all of them can be enabled/disabled some of them like effect and color is used to determine the source 
 type you are confronted with at the priority overview.
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
Sources are always selected automatically by priority value (lowest number first), now you could select on your own a specific priority which should be visible (shown). You need the priority value of the source you want to select. Get them from the [serverinfo Priorities](/en/json/serverinfo#priorities).
``` json
// Example: Set priority 50 to visible
{
  "command":"sourceselect",
  "priority":50
}
```
If you get a success response, the `priorities_autoselect`-status will switch to false: [serverinfo Autoselection Mode](/en/json/serverinfo##priorities-selection-auto-manual). You are now in manual mode, to switch back to auto mode send:
``` json
{
  "command":"sourceselect",
  "auto":true
}
```
Now, the `priorities_autoselect`-status will be again true

::: warning
You can just select priorities which are `active:true`!
:::

### Control Instances
An instance represents a LED hardware instance, it runs within a own scope along with it's own plugin settings, led layout, calibration. First, you need to get informations about instances. The first shot from [serverinfo Instance](/en/json/serverinfo#instance).
``` json
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
In both cases you get a success response, it doesn't return any error responses.

### API Instance handling
On connection to the API you will be connected to instance `0`, that means you can control just one instance at the same time within a connection. It's possible to switch to another instance with the following command.

``` json
// We switch to instance 1
{
  "command" : "instance",
  "subcommand" : "switchTo",
  "instance" : 1
}
```
Will return a success response, or a error response when the instance isn't available

::: warning
It's possible that an instance will stop while you are connected. In this case you will be automatically reseted to instance `0`. So keep watching the instance data as subscription: [Instance updates](/en/json/subscribe#instance-updates)
:::

### Live Image Stream
You can request a live image stream (when the current source priority can deliver). So it might be possible that there is no response or it stops and starts in between.
``` json
{
  "command":"ledcolors",
  "subcommand":"imagestream-start"
}
```
You will receive "ledcolors-imagestream-update" messages with a base64 encoded image.
Stop the stream by sending
``` json
{
  "command":"ledcolors",
  "subcommand":"imagestream-stop"
}
```
::: danger HTTP/S
This feature is not available for HTTP/S JSON-RPC
:::


### Live Led Color Stream
You can request a live led color stream with current color values in RGB for each single led. Update rate is 125ms.
``` json
{
  "command":"ledcolors",
  "subcommand":"ledstream-start"
}
```
You will receive "ledcolors-ledstream-update" messages with an array of all led colors.
Stop the stream by sending
``` json
{
  "command":"ledcolors",
  "subcommand":"ledstream-stop"
}
```
::: danger HTTP/S
This feature is not available for HTTP/S JSON-RPC
:::

### Plugins
::: danger NOT IMPLEMENTED
THIS IS NOT IMPLEMENTED
:::
You can start and stop plugins. First, you need to get informations about plugins. The first short from [serverinfo plugins](/en/json/serverinfo#plugins).
You need now the plugin id. Example: `service.kodi`.
``` json
// Command to start a plugin
{
  "command":"plugin",
  "subcommand":"start",
  "id":"service.kodi"
}
// Command to stop a plugin
{
  "command":"plugin",
  "subcommand":"stop",
  "id":"service.kodi"
}
```  
You will get a response of your action. `plugin-start` or `plugin-stop` with success true/false.
