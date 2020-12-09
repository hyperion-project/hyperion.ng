# Subscription
During a `serverinfo` request the caller can optionally subscribe to updates -- either
to specific [serverinfo parts](/en/json/ServerInfo.html#parts) or all available data.
These updates will be pushed whenever a server-side data change occurs, without the need
for the caller to poll.

[[toc]]

To subscribe to specific updates, you can modify the serverinfo command to:
```json
{
    "command":"serverinfo",
    "subscribe":[
        "firstCommand",
        "secondCommand",
        "thirdCommand"
    ],
    "tan":1
}
```
To subscribe for all available updates modify the severinfo command to
```json
{
    "command":"serverinfo",
    "subscribe":["all"],
    "tan":1
}
```
### Base response layout
All pushed subscription updates will have an `-update` suffix added to the relevant key
from the [serverinfo part in question](/en/json/ServerInfo.html#parts). The new data
will be in the `data` property. There is no `tan` nor `success` argument provided.
```json
{
    "command":"XYZ-update",
    "data":{
        ..Data here..
    }
}
```
### Component updates
The caller can subscribe to component updates. These updates are meant to update the
`components` section of the caller's initial serverinfo. Relevant `serverinfo`
subscription command:

```json
{
    "command":"serverinfo",
    "subscribe":[
        "components-update"
    ],
    "tan":1
}
```
After this, the caller will receive incremental updates. An example:
```json
{
    "command":"components-update",
    "data":{
        "enabled":false,
        "name":"SMOOTHING"
    }
}
```

### Session updates
The caller can subscribe to session updates (Hyperion instances found with
Bonjour/Zeroconf/Ahavi). These updates are meant to update the `sessions` section of
the caller's initial serverinfo. Relevant `serverinfo` subscription command:
```json
{
    "command":"serverinfo",
    "subscribe":[
        "sessions-update"
    ],
    "tan":1
}
```
These updates aren't incremental -- they contain all found entries on each update.
Example response with 2 HTTP server sessions (`_hyperiond-http._tcp`):
```json
{
    "command":"sessions-update",
    "data":[
        {
            "address":"192.168.58.169",
            "domain":"local.",
            "host":"ubuntu-2",
            "name":"My Hyperion Config@ubuntu:8090",
            "port":8090,
            "type":"_hyperiond-http._tcp."
        },
        {
            "address":"192.168.58.169",
            "domain":"local.",
            "host":"ubuntu-2",
            "name":"My Hyperion Config@ubuntu:8099",
            "port":8099,
            "type":"_hyperiond-http._tcp."
        }
   ]
}
```
### Priority updates
The caller can subscribe to priority updates. These updates are meant to update the
`priorities` and `priorities_autoselect` section of the caller's initial `serverinfo`.
Relevant `serverinfo` subscription command:
```json
{
    "command":"serverinfo",
    "subscribe":["priorities-update"],
    "tan":1
}
```
Caller will get the complete data. Please note that if a color or effect is running with
a timeout > -1, the caller will receive new data each second. An example update:
```json
{
  "command":"priorities-update",
  "data":{
    "priorities":[
      {
        "active":true,
        "componentId":"GRABBER",
        "origin":"System",
        "owner":"X11",
        "priority":250,
        "visible":true
      },
      {
        "active":true,
        "componentId":"EFFECT",
        "origin":"System",
        "owner":"Warm mood blobs",
        "priority":254,    
        "visible":false
      },
      {
        "active":true,
        "componentId":"COLOR",
        "origin":"System",
        "owner":"System",
        "priority":40,
        "value":{
          "HSL":[65535,0,0],
          "RGB":[0,0,0]
        },
        "visible":false
      }
    ],
    "priorities_autoselect":false
  }
}
```
### LED Mapping updates
The caller can subscribe to LED mapping type updates. These updates are meant to update
the `imageToLedMappingType` section of the caller's initial `serverinfo`.
Relevant `serverinfo` subscription command:
```json
{
    "command":"serverinfo",
    "subscribe":["imageToLedMapping-update"],
    "tan":1}
```
An example update:
```json
{
    "command":"imageToLedMapping-update",
    "data":{
        "imageToLedMappingType":"multicolor_mean"
    }
}
```
### Adjustment updates
The caller can subscribe to adjustment updates. These updates are meant to update the
`adjustment` section of the caller's initial `serverinfo`. Relevant `serverinfo`
subscription command:
```json
{
    "command":"serverinfo",
    "subscribe":["adjustment-update"],
    "tan":1
}
```
An example update:
```json
{
  "command":"adjustment-update",
    "data":[{
      "backlightColored":true,
      "backlightThreshold":0,
      "black":[0,0,0],
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
    }]
}
```
### VideoMode updates
The caller can subscribe to videomode updates. These updates are meant to update the
`videomode` section of the cakker's initial `serverinfo`. Relevant `serverinfo`
subscription command:
```json
{
  "command":"serverinfo",
  "subscribe":["videomode-update"],
  "tan":1
}
```
An example update:
```json
{
  "command":"videomode-update",
  "data":{
    "videomode": "2D"
  }
}
```
### Effects updates
The caller can subscribe to effect list updates. These updates are meant to update the
`effects` section of the caller's initial `serverinfo`. Relevant `serverinfo`
subscription command:
```json
{
  "command":"serverinfo",
  "subscribe":["effects-update"],
  "tan":1
}
```
An example update:
```json
{
  "command":"effects-update",
  "data":{
    "effects": [ ..All effects here..]
  }
}
```

### Instance updates
The caller can subscribe to instance updates. These updates are meant to update the
`instance` section of the caller's initial serverinfo. Relevant `serverinfo`
subscription command:
```json
{
  "command":"serverinfo",
  "subscribe":["instance-update"],
  "tan":1
}
```
An example update. This is not incremental -- the caller will get the full set of
instances:
```json
{
  "command":"instance-update",
  "data":[
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
}
```
### LEDs updates
The caller can subscribe to leds updates. These updates are meant to update the `leds`
section of the caller's initial `serverinfo`. Relevant `serverinfo` subscription command:
```json
{
  "command":"serverinfo",
  "subscribe":["leds-update"],
  "tan":1
}
```
An example update. This is not incremental -- the caller willg et the full set of leds:
```json
{
  "command":"leds-update",
    "data": {
      "leds" : [
      {
        "hmin":0.0,
        "hmax":1.0,
        "vmin":0.0,
        "vmax":1.0
       },
       ... more leds ...
      ]
    }
  }
```
