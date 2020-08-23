# Subscription
During initial serverinfo request you can subscribe to specific data updates or all of them at once, these updates will be pushed to you whenever a server side data change occur.

[[toc]]

To subscribe for specific updates modify serverinfo command to
``` json
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
``` json
{
    "command":"serverinfo",
    "subscribe":["all"],
    "tan":1
}
```
### Base response layout
All responses will have a `-update` suffix in their command property, it's the same command you subscribed to. The new data is in the `data` property. A `tan` and `success` property does not exist.
``` json
{
    "command":"XYZ-update",
    "data":{
        ..Data here..
    }
}
```
### Component updates
You can subscribe to updates of components. These updates are meant to update the `components` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":[
        "components-update"
    ],
    "tan":1
}
```
You will get incremental updates, here an example response
``` json
{
    "command":"components-update",
    "data":{
        "enabled":false,
        "name":"SMOOTHING"
    }
}
```

### Session updates
You can subscribe to session updates (Found with Bonjour/Zeroconf/Ahavi). These updates are meant to update the `sessions` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":[
        "sessions-update"
    ],
    "tan":1
}
```
These updates aren't incremental, so they contain always all found entries.
Example response with 2 HTTP server sessions (_hyperiond-http._tcp)
``` json
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
You can subscribe to priority updates. These updates are meant to update the `priorities` and `priorities_autoselect` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":["priorities-update"],
    "tan":1
}
```
You get the complete data, please note that if a color or effect is running with a timeout > -1 you will receive at least within a 1 second interval new data. Here an example update:
``` json
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
You can subscribe to LED mapping type updates. These updates are meant to update the `imageToLedMappingType` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":["imageToLedMapping-update"],
    "tan":1}
```
Here an example update:
``` json
{
    "command":"imageToLedMapping-update",
    "data":{
        "imageToLedMappingType":"multicolor_mean"
    }
}
```
### Adjustment updates
You can subscribe to adjustment updates. These updates are meant to update the `adjustment` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":["adjustment-update"],
    "tan":1
}
```
Here an example update:
``` json
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
You can subscribe to videomode updates. These updates are meant to update the `videomode` section of your initial serverinfo. Modify serverinfo command to
``` json
{
  "command":"serverinfo",
  "subscribe":["videomode-update"],
  "tan":1
}
```
Here an example update:
``` json
{
  "command":"videomode-update",
  "data":{
    "videomode": "2D"
  }
}
```
### Effects updates
You can subscribe to effect list updates. These updates are meant to update the `effects` section of your initial serverinfo. Modify serverinfo command to
``` json
{
  "command":"serverinfo",
  "subscribe":["effects-update"],
  "tan":1
}
```
Here an example update:
``` json
{
  "command":"effects-update",
  "data":{
    "effects": [ ..All effects here..]
  }
}
```

### Instance updates
You can subscribe to instance updates. These updates are meant to update the `instance` section of your initial serverinfo. Modify serverinfo command to
``` json
{
  "command":"serverinfo",
  "subscribe":["instance-update"],
  "tan":1
}
```
Here an example update, you will also get the whole section:
``` json
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
You can subscribe to leds updates. These updates are meant to update the `leds` section of your initial serverinfo. Modify serverinfo command to
``` json
{
  "command":"serverinfo",
  "subscribe":["leds-update"],
  "tan":1
}
```
Here an example update, you will also get the whole section:
``` json
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

### Plugin updates
::: danger NOT IMPLEMENTED
THIS IS NOT IMPLEMENTED
:::
You can subscribe to plugin updates. These updates are meant to update the `plugins` section of your initial serverinfo. Modify serverinfo command to
``` json
{
    "command":"serverinfo",
    "subscribe":[
        "plugins-update"
    ],
    "tan":1
}
```
Response on new plugin has been added (or plugin has been updated to a newer version)
``` json
{
    "command":"plugins-update",
    "data":{
    "IdOfPlugin":{
        "name":"The name of plugin",
        "description":"The description of plugin",
        "version":"TheVersion",
        "running":false
        }
    }
  }
```
Response on plugin removed, the data object contains a `removed` property / the plugin id object is empty
``` json
{
    "command":"plugins-update",
    "data":{
        "ThePluginId":{
            "removed":true
        }
    }
}
```
Response on plugin running state change
``` json
{
    "command":"plugins-update",
    "data":{
        "ThePluginId":{
            "running":true
        }
    }
}
```