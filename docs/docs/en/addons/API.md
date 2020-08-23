
# Addon API
Overview of the API. Get access to these functions by importing the `hapi` module `import hapi`.
| Function                  | Returns | Comment                                                                                             |
| ------------------------- | ------- | --------------------------------------------------------------------------------------------------- |
| hapi.abort()              | bool    | Check if we should abort the script, True on abort request else false                               |
| hapi.log()                | -       | Print a log message to the Hyperion log. See [hapi.log()](#hapi-log)                                 |
| hapi.registerCallback()   | -       | Register a function with a callback function. See [hapi.registerCallback()](#hapi-registercallback) |
| hapi.unregisterCallback() | -       | Unregister a function from callback. See [hapi.unregisterCallback()](#hapi-unregistercallback)      |
| hapi.getComponentState()  | int     | Get current state of a component. See [hapi.getComponentState()](#hapi-getcomponentstate )          |
| hapi.setComponentState()  | bool    | Enable or disable a component. See [hapi.setComponentState()](#hapi-setcomponentstate)              |
| hapi.getSettings()        | data    | Get the current user settings for your addon. See [hapi.getSettings()](#hapi-getsettings)           |
| hapi.setColor()           | -       | Set a color. See [hapi.setColor()](#hapi-setcolor)                                                  |
| hapi.setEffect()          | int     | Start effect by name. See [hapi.setEffect()](#hapi-seteffect )                                      |
| hapi.getPriorityInfo()    | dict    | Get info about a priority. See [hapi.getPriorityInfo()](#hapi-getpriorityinfo)                      |
| hapi.getAllPriorities()   | list    | Get all registered priorities. See [hapi.getAllPriorities()](#hapi-getallpriorities)                |
| hapi.getVisiblePriority() | int     | Get current visible priority. See [hapi.getVisiblePriority()](#hapi-getvisiblepriority)             |
| hapi.setVisiblePriority() | bool    | Select a specific priority. See [hapi.setVisiblePriority()](#hapi-setvisiblepriority)               |
| hapi.getAdjustmentList()  | list    | Get a list of all adjustment ids. See [hapi.getAdjustmentList()](#hapi-getadjustmentlist)           |
| hapi.getBrightness()      | int     | Get current brightness. See [hapi.getBrightness()](#hapi-getbrightness)                             |
| hapi.setBrightness()      | bool    | Set new brightness. See [hapi.setBrightness()](#hapi-setbrightness)                                 |

## Data transformation cheatsheet
As we need to transform "UI elements"-data from `json` to `python`, here a cheatsheet how the transformation is done.

|  Json   | Python |
| :-----: | :----: |
| boolean |  int   |
| integer |  int   |
| number  | float  |
| string  |  str   |
|  array  |  list  |
| object  |  dict  |

## Methods

### hapi.log()
Write a message to the Hyperion log. Your addon name will be automatically prepended to all messages! 
`hapi.log(msg, lvl)`
| Argument | Type       | Comment |
| msg      | str     | The message you want to print as string |
| lvl      | enum     | Optional: Print message with a specific log lvl. `LOG_INFO` = Info, `LOG_WARNING` = Warning, `LOG_ERROR` = Error, `LOG_DEBUG` = Debug. **Defaults to Debug** |

### hapi.registerCallback()
To listen for specific Hyperion events you can register callbacks. 
`hapi.registerCallback(callbackType, connectFunction)`
| Argument                                  | Type | Comment                                                                                             |
| ----------------------------------------- | ---- | --------------------------------------------------------------------------------------------------- |
| callbackType                              | Enum | Callbacks: `ON_COMP_STATE_CHANGED`, `ON_SETTINGS_CHANGED`, `ON_VISIBLE_PRIORITY_CHANGED`            |
| connectFunction                           | -    | A previously defined function which will be used to deliver the callback                            |
| callbackType: ON_COMP_STATE_CHANGED       | -    | When comp state changes, arg1 `comp` component as str, arg2 `newState` as int                       |
| callbackType: ON_SETTINGS_CHANGED         | -    | When user saved new settings, reload settings [hapi.getSettings()](#hapi.getsettings)               |
| callbackType: ON_VISIBLE_PRIORITY_CHANGED | -    | Is called whenever the visible priority changes, argument `priority` of type int (the new priority) |
``` python
# Example implementation with log output
import hapi

def onCompStateChanged(comp, newState):
  hapi.log("The component "+comp+" changed state to:"+str(newState))

def onSettingsChanged():
  hapi.log("New settings! Do something!")
  
def onVsibilePriorityChanged(newPriority):
  hapi.log("The new visible priority is: "+str(newPriority))

hapi.registerCallback(ON_COMP_STATE_CHANGED, onCompStateChanged)
hapi.registerCallback(ON_SETTINGS_CHANGED, onSettingsChanged)
hapi.registerCallback(ON_VISIBLE_PRIORITY_CHANGED, onVsibilePriorityChanged)
```
 
### hapi.unregisterCallback()
You can also unregister a callbackType again. You can register and unregister as often you need it. Usually you won't need it, but for completion here it is! After unregister the connected function does no longer react upon callbacks 
`hapi.unregisterCallback(callbackType)`
| Argument     | Type | Comment                                                                              |
| ------------ | ---- | ------------------------------------------------------------------------------------ |
| callbackType | Enum | One of `ON_COMP_STATE_CHANGED`, `ON_SETTINGS_CHANGED`, `ON_VISIBLE_PRIORITY_CHANGED` |

### hapi.getComponentState()
Get the current state of a component. 
`hapi.getComponentState(comp)`
| Argument | Type | Comment                                                                                                                                                                                                                   |
| -------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| comp     | int  | The component ID one of `COMP_ALL COMP_SMOOTHING COMP_BLACKBORDER COMP_LEDDEVICE COMP_GRABBER COMP_V4L`, prefixed with `COMP_`. See explanation here [Component IDs explained](/en/json/control#components-ids-explained) |
| @return  | int  | `True` if enabled, `False` if disabled, `-1` if not found                                                                                                                                                                 |


### hapi.setComponentState()
Set a Hyperion component to a new state. This method writes a debug message on each call! 
`hapi.setComponentState(comp, enable)`
| Argument | Type | Comment                                                                                                                                                                                                                   |
| -------- | ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| comp     | int  | The component ID one of `COMP_ALL COMP_SMOOTHING COMP_BLACKBORDER COMP_LEDDEVICE COMP_GRABBER COMP_V4L`, prefixed with `COMP_`. See explanation here [Component IDs explained](/en/json/control#components-ids-explained) |
| enable   | int  | True to enable it, False to disable it                                                                                                                                                                                    |
| @return  | bool | True if the requested component was found else false                                                                                                                                                                      |

### hapi.getSettings()
Get the current user settings data for your addon. The returned data contains all settings that has been transformed from JSON to python data types. See the table below how they are transformed.
`hapi.getSettings()`
|  Json   | Python |
| :-----: | :----: |
| boolean |  int   |
| integer |  int   |
| number  | float  |
| string  |  str   |
|  array  | tuple  |
| object  |  dict  |

**Basics** \
The root data type is usually of type object which will be transformed to a python dict, so we use the dict syntax to get the value. The fallback parameter should be defined wisely, if the value is not found it will be used instead.
``` python
import hapi
# Get a bool value of a dict 
myBoolValue = MySettings.get('MyBoolOption', True)

# Get a int value of a dict 
myIntValue = MySettings.get('MyIntOption', 20)

# Get a float value of a dict 
myFloatValue = MySettings.get('MyFloatOption', 15.7)
 
# Get a string value of a dict 
myStringValue = MySettings.get('MyStringOption', 'MyFallbackString')
  
# Get a tuple of a dict
# Two RGB colors would look like the following. A tuple that contains 2 other tuples where each contains 3 int values
myListValue = MySettings.get('MyListOption', ())

# Here a colorpicker example
myColorValue = MySettings.get('MyColorPicker', (255,0,0))
# Here an array(tuple) of multiple colorpickers
myColorValues = MySettings.get('MyColorPickers', ((255,0,0),(0,255,0)) )
```

### hapi.setColor()
Set a RGB color value with timeout at the given priority. 
`hapi.setColor(red, green, blue, timeout, priority)`
| Argument | Type | Comment                                                                |
| -------- | ---- | ---------------------------------------------------------------------- |
| red      | int  | Red 0-255                                                              |
| green    | int  | Green 0-255                                                            |
| blue     | int  | Blue 0-255                                                             |
| timeout  | int  | Optional: Set a specific timeout in ms. Defaults to -1 (endless)       |
| priority | int  | Optional: Set the color at a specific priority channel. Defaults to 50 |

### hapi.setEffect()
Start an effect by name with timeout at the given priority. 
`hapi.setEffect(effectName, timeout, priority)`
| Argument   | Type | Comment                                                                |
| ---------- | ---- | ---------------------------------------------------------------------- |
| effectName | str  | The name of the effect                                                 |
| timeout    | int  | Optional: Set a specific timeout in ms. Defaults to -1 (endless)       |
| priority   | int  | Optional: Set the color at a specific priority channel. Defaults to 50 |
| @return    | int  | `0` on success, `-1` on failure (eg not found)                         |

### hapi.getPriorityInfo()
Get priority info for the given priority. 
`hapi.getPriorityInfo(priority)`
| Argument | Type | Comment                                                                                                      |
| -------- | ---- | ------------------------------------------------------------------------------------------------------------ |
| priority | int  | The priority you want information for                                                                        |
| @return  | dict | A dict with fields `priority` as int, `timeout` as int, `component` as str, `origin` as str, `owner` as str. |

### hapi.getAllPriorities()
Get all registered priorities. 
`hapi.getAllPriorities()`
| Argument | Type  |                                   Comment                                   |
| :------: | :---: | :-------------------------------------------------------------------------: |
| @return  | tuple | A tuple of all priorities that has been registered (type int) `(1,240,255)` |


### hapi.getVisiblePriority()
Get the current visible priority. 
`hapi.getVisiblePriority()`
| Argument | Type | Comment      |
| -------- | ---- | ------------ |
| @return  | int  | The priority |

### hapi.setVisiblePriority()
Set a specific priority to visible. Returns `True` on success or `False` if not found. 
`hapi.setVisiblePriority(priority)`
| Argument | Type | Comment                                              |
| -------- | ---- | ---------------------------------------------------- |
| priority | int  | The priority to set                                  |
| @return  | bool | `True` on success, `False` on failure (eg not found) |


### hapi.getAdjustmentList()
Returns a list of all available adjustment ids. These ids are used to apply changes to different calibration profiles. 
`hapi.getAdjustmentList()`
| Argument | Type | Comment                |
| -------- | ---- | ---------------------- |
| @return  | list | List of adjustment ids |

### hapi.getBrightness()
Get the brightness of a given adjustment id. If no id is provided it will return the brightness of primary adjustment (e.g. first). See also [hapi.getAdjustmentList()](#hapi-getadjustmentlist). 
`hapi.getBrightness(id)`
| Argument | Type | Comment                                                                                                                 |
| -------- | ---- | ----------------------------------------------------------------------------------------------------------------------- |
| id       | str  | Optional: Get the brightness of a specific adjustment id, if not given it will return the primary adjustment brightness |
| @return  | int  | The brightness between `0` and `100` (considered as %). Returns `-1` if the adjustment id is not found                  |

### hapi.setBrightness()
Set the brightness for the given adjustment id. If no id is provided it will set all available adjustment ids to the given brightness. See also getAdjustmentList() 
`hapi.setBrightness(brightness, id)`
|  Argument  | Type  |                                                 Comment                                                 |
| :--------: | :---: | :-----------------------------------------------------------------------------------------------------: |
| brightness |  int  |                               The brightness value between `0` and `100`                                |
|     id     |  str  | Optional: Target a specific adjustment id, if not set it will apply the brightness to all available ids |
|  @return   | bool  |                `True` on success, `False` on failure (eg given adjustment id not found)                 |
