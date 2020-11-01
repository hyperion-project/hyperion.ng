# Server Information
This is the primary read mechanism of the Hyperion server. This single command provides data about the live state of Hyperion, broken down into a number
of different parts (described below).

[[toc]]

You can request a `serverinfo` response by sending the following command:
```json
{
    "command":"serverinfo",
    "tan":1
}
```

## Parts of a serverinfo response

### Components
List of Hyperion components and their current status "enabled" (on/off). You can enable
or disable them during runtime . The "ALL" component reflect Hyperion as a whole -- if
"ALL" is false (off) you can't enable any other component. [See control
components](/en/json/control#control-components)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Component updates](/en/json/subscribe#component-updates)
:::

```json
{
  "components":[
    {
      "enabled":true,
      "name":"ALL"
    },
    {
      "enabled":true,
      "name":"SMOOTHING"
    },
    {
      "enabled":true,
      "name":"BLACKBORDER"
    },
    {
      "enabled":false,
      "name":"FORWARDER"
    },
    {
      "enabled":false,
      "name":"BOBLIGHTSERVER"
    },
    {
      "enabled":false,
      "name":"GRABBER"
    },
    {
      "enabled":false,
      "name":"V4L"
    },
    {
      "enabled":true,
      "name":"LEDDEVICE"
    }
  ]
}
```

### Adjustments
Adjustments reflect the value of the last performed (non-persistent) color adjustment
(e.g. brightness). Read more about [control Adjustments](/en/json/control#adjustments)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Adjustment updates](/en/json/subscribe#adjustment-updates)
:::
```json
{
  "adjustment":[
    {
      "backlightColored":true,
      "backlightThreshold":0,
      "blue":[0,0,255],
      "brightness":1,
      "cyan":[0,127,127],
      "gammaBlue":1.4,
      "gammaGreen":1.4,
      "gammaRed":1.4,
      "green":[0,255,0],
      "id":"default",
      "magenta":[255,0,255],
      "red":[255,0,0],
      "white":[255,255,255],
      "yellow":[255,255,0]
    }
  ]
}
```

### Effect list
An array of effects where each object is one named effect. You can filter between user
created effects and system provided effects by checking the effect `file` string -- if
it begins with `:` it's a system provided effect, whereas if the path begins with `/`,
it's a user created effect.

See also [set Effect](/en/json/control#set-effect)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Effect updates](/en/json/subscribe#effects-updates)
:::
```json
{
  "effects":[
    {
      "args":{
        "blobs":5,
        "color":[
          0,
          0,
          255
        ],
        "hueChange":60,
        "reverse":false,
        "rotationTime":60
      },
      "file":":/effects//mood-blobs-blue.json",
      "name":"Blue mood blobs",
      "script":":/effects//mood-blobs.py"
    },
    {
      "args":{
        "brightness":100,
        "candles":"all",
        "color":[
          255,
          138,
          0
        ],
        "sleepTime":0.14999999999999999
      },
      "file":":/effects//candle.json",
      "name":"Candle",
      "script":":/effects//candle.py"
    }
    ....
  ]
}
```
  
### LED mapping
Active mode of the led area mapping. [See control LED mapping](/en/json/control#led-mapping)
::: tip Subscribe
You can subscribe to future data updates. Read more about [LED mapping updates](/en/json/subscribe#led-mapping-updates)
:::
```json
  "imageToLedMappingType":"multicolor_mean"
```

### Video mode
The current video mode of grabbers. Can be switched to 3DHSBS, 3DVSBS. [See control video mode](/en/json/control#video-mode)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Video mode updates](/en/json/subscribe#videomode-updates)
:::
```json
  "videomode" : "2D"
```

### Priorities
Overview of the registered/active sources. Each object is a source.
  * **active**: If "true" it is selectable for manual source selection. [See also source selection](/en/json/control#source-selection)
  * **visible**: If "true" this source is displayed and pushed to the led device. The `visible:true`-source is always the first entry!
  * **componentId**: A key belonging to a specific component that indicates the kind of input. [See available components](/en/json/control#components-ids-explained)
  * **origin**: A named external setter of this source for reference purposes. If not given it's `System` (from Hyperion).
  * **owner**: Contains additional information related to the componentId. If it's an effect,
  the effect name is shown here. If it's USB capture, the capture device is shown. If
  it's platform capture, you get the name of the platform capture implementation (e.g. dispmanx/x11/amlogic/...).
  * **priority**: The priority of this source, an integer between 0 and 255.
  * **value**: If the source is a color AND color data is available (if active is false
    there's usually no datta),hen this will be the color in RGB and HSL.
  * **duration_ms**: Actual duration in ms until this priority is automatically deleted.
    This is shown if source is color or effect AND a specific duration higher than
    `0` is set (0 means indefinite).

::: tip Subscribe
You can subscribe to future data updates. Read more about [Priority updates](/en/json/subscribe#priority-updates)
:::
```json
  "priorities":[
    {
      "active":true,
      "componentId":"COLOR",
      "origin":"Web Configuration@192.168.0.28",
      "owner":"COLOR",
      "priority":1,
      "value":{
        "HSL":[
          0,
          1,
          0.50000762939453125
        ],
        "RGB":[
          0,
          0,
          255
        ]
      },
      "visible":true
    },
    {
      "active":true,
      "componentId":"EFFECT",
      "origin":"System",
      "owner":"Warm mood blobs",
      "priority":255,
      "visible":false
    }
  ]
```

### Priorities selection: Auto/Manual
If `priorities_autoselect` is "true" the visible source is determined by priority. The
lowest number is automatically selected. If a caller requests to set a source manually,
then `priorities_autoselect` switches to `false`.

If the manually selected source is cleared/stops/completes-duration OR the user requests
the auto selection, `priorities_autoselect` switches back to `true`. This value is
atomically updated with the priority updates (shown above).
[See also source selection](/en/json/control#source-selection).
  
### Instance
Information about available instances and their state. Each instance represents a LED
device. Instances can be controlled, see: [Control Instance](/en/json/control#control-instances).
::: tip Subscribe
You can subscribe to future data updates. Read more about [Instance Updates](/en/json/subscribe#instance-updates)
:::
```json
   "instance":[
       {
         "instance": 0,
         "running" : true,
         "friendly_name" : "My First LED Hardware instance"
       },
       {
         "instance": 1,
         "running" : false,
         "friendly_name" : "PhilipsHue LED Hardware instance"
       }
   ]
```

### LEDs
Information about led layout (image mapping positions) and led count.
::: tip Subscribe
You can subscribe to future data updates. Read more about [LEDs Updates](/en/json/subscribe#leds-updates)
:::
```json
{
  "leds":[
    {
      "hmin":0.0,
      "hmax":1.0,
      "vmin":0.0,
      "vmax":1.0 
    },
    {
      "hmin":0.0,
      "hmax":1.0,
      "vmin":0.0,
      "vmax":1.0 
    },
    ...
  ]
}
```

### System & Hyperion
It's possible to retrieve basic system information about the Hyperion server and the
host it runs on. This information is static and won't change during runtime.
```json
{
    "command" : "sysinfo",
    "tan" : 1
}
```
You can use the "version" (Hyperion version) string to check application compatibility. We use [Semantic Versioning 2.0.0](https://semver.org/).
If you need a specific id to re-detect known servers you can use the `id` field which
provides a unique id and will not change for a given server.
```json
{
    "hyperion":{
        "build":"webd (brindosch-38f97dc/814977d-1489698970)",
        "gitremote": "https://github.com/hyperion-project/hyperion.git",
        "time":"Mar 16 2017 21:25:46",
        "version":"2.0.0",
        "id":"jKsh78D3hd..."
    },
    "system":{
        "architecture":"arm",
        "hostName":"raspberrypi",
        "kernelType":"linux",
        "kernelVersion":"4.4.13-v7+",
        "prettyName":"Raspbian GNU/Linux 8 (jessie)",
        "productType":"raspbian",
        "productVersion":"8",
        "wordSize":"32"
      }
}
```

### Sessions
 `sessions` shows all Hyperion servers on the current network found via Zeroconf/avahi/bonjour. See also [detect Hyperion](/en/api/detect.md)
 ::: tip Subscribe
You can subscribe to future data updates. Read more about [Session updates](/en/json/subscribe#session-updates)
:::

```json
{
  "sessions":[
    {
        "address":"192.168.0.20",
        "domain":"local.",
        "host":"raspberrypi",
        "name":"Awwh yeah!!@raspberrypi:8099",
        "port":8090,
        "type":"_hyperiond-http._tcp."
    }
  ] 
}
```
