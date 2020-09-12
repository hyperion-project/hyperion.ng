# Sever Information
It provides you informations about Hyperion that are required to perform certain actions.

[[toc]]

Grab it by sending:
``` json
{
    "command":"serverinfo",
    "tan":1
}
```

## Parts

### Components
List of Hyperion components and their current status "enabled" (on/off). You can enable or disable them during runtime. The "ALL" component reflect Hyperion as a whole and as long "ALL" is false (off) you can't enable another component. [See control components](/en/json/control#control-components)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Component updates](/en/json/subscribe#component-updates)
:::

``` json
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
Adjustments reflects the value of the last performed (non persistent) color adjustment. Read more about [control Adjustments](/en/json/control#adjustments)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Adjustment updates](/en/json/subscribe#adjustment-updates)
:::
``` json
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
An array of effects where each object is one effect, usually you just use the `name` to create a list for the user. You could filter between user created effects and provided effects by checking the effect `file` string. In case it begins with `:` it's a provided effect. If the path begins with `/`, it's a user created effect. Could be used to list the user effects earlier than the provided effects and to keep the overview.
See also [set Effect](/en/json/control#set-effect)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Effect updates](/en/json/subscribe#effects-updates)
:::
``` json
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
Active mode of led area mapping. [See control LED mapping](/en/json/control#led-mapping)
::: tip Subscribe
You can subscribe to future data updates. Read more about [LED mapping updates](/en/json/subscribe#led-mapping-updates)
:::
``` json
  "imageToLedMappingType":"multicolor_mean"
```

### Video mode
The current video mode of grabbers Can be switched to 3DHSBS, 3DVSBS. [See control video mode](/en/json/control#video-mode)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Video mode updates](/en/json/subscribe#videomode-updates)
:::
``` json
  "videomode" : "2D"
```

### Priorities
Overview of the registered/active sources. Each object is a source.
  * active: If "true" it is selectable for manual source selection. [See also source selection](/en/json/control#source-selection)
  * visible: If "true" this source is displayed and will be pushed to the led device. The `visible:true`-source is always the first entry!
  * componentId: A string belongs to a specific component. [See available components](/en/json/control#components-ids-explained)
  * origin: The external setter of this source "NameOfRemote@IP". If not given it's `System` (from Hyperion).
  * owner: Contains additional information related to the componentId. If it's an effect, the effect name is shown here. If it's USB capture it shows the capture device. If it's platform capture you get the name of it (While we use different capture implementations on different hardware (dispmanx/x11/amlogic/...)).
  * priority: The priority of this source.
  * value: Just available if source is a color AND color data is available (active = false has usually no data). Outputs the color in RGB and HSL.
  * duration_ms: Actual duration in ms until this priority is deleted. Just available if source is color or effect AND a specific duration higher than `0` is set (because 0 is endless).

::: tip Subscribe
You can subscribe to future data updates. Read more about [Priority updates](/en/json/subscribe#priority-updates)
:::
``` json
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

### Priorities selection Auto/Manual
If "priorities_autoselect" is "true" the visible source is determined by priority. The lowest number is selected. If someone request to set a source manual, the value switches to "false".
In case the manual selected source is cleared/stops/duration runs out OR the user requests the auto selection, it switches back to "true". [See also source selection](/en/json/control#source-selection).
This value will be updated together with the priority update.
  
### Instance
Information about available instances and their state. Each instance represents a LED device. How to control them: [Control Instance](/en/json/control#control-instances).
::: tip Subscribe
You can subscribe to future data updates. Read more about [Instance Updates](/en/json/subscribe#instance-updates)
:::
``` json
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
    [
```

### LEDs
Information about led layout (image mapping positions) and led count.
::: tip Subscribe
You can subscribe to future data updates. Read more about [LEDs Updates](/en/json/subscribe#leds-updates)
:::
``` json
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
It's possible to gather some basic software informations Hyperion runs on. This information is static and won't change during runtime.
``` json
{
    "command" : "sysinfo",
    "tan" : 1
}
```
You can use the "version" (Hyperion version) string to check application compatibility. We use [Semantic Versioning 2.0.0](https://semver.org/).
If you need a specific id to re-detect known servers you can use the "id" field which provides a unique id.
``` json
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

Hyperion will answer your request. Below just the "info"-part of the response, splitted for better overview.
::: tip Subscribe
You can subscribe to future data updates. Read more about [Subscriptions](/en/json/subscribe)
:::

### Sessions
 "sessions" shows all Hyperion servers at the current network found via Zeroconf/avahi/bonjour. See also [detect Hyperion](/en/api/detect.md)
 ::: tip Subscribe
You can subscribe to future data updates. Read more about [Session updates](/en/json/subscribe#session-updates)
:::

``` json
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

### Plugins
::: danger NOT IMPLEMENTED
THIS IS NOT IMPLEMENTED
:::
Information about installed plugins. How to control them: [Control Plugins](/en/json/control#plugins)
::: tip Subscribe
You can subscribe to future data updates. Read more about [Plugin updates](/en/json/subscribe#plugin-updates)
:::
``` json
  "plugins": {
    "service.kodi": {
      "description": "Connect to a Kodi instance to get player state",
      "name": "Kodi Connector",
      "version": "0.0.2",
      "running": true
    },
    "service.wol": {
      "description": "Send WOL packages to somewhere",
      "name": "WOL Packages",
      "version": "0.0.1",
      "running": false
    }
  }
```