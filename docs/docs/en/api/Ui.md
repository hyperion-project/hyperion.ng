# Graphical User Interface
We use a JSON schema to create a user-friendly GUI and validate the input to prevent wrong or unwanted data that will be consumed by your python file. Each python file requires a GUI schema.

::: tip
If you have never written JSON syntax, we recommend a short introduction. [Here](https://www.digitalocean.com/community/tutorials/an-introduction-to-json) and/or [Here (technical)](http://www.json.org/)
:::

[[toc]]

## UI elements
Each UI element has a specific schema. The schema allows you to give the element a label and validate user input by providing for example a minimum and a maximum number for an element of type number. A minimum count of colors, a default value (required!) or even to show/hide an element based on the value of another element.

### Checkbox
Show a checkbox.
``` json
  "swirl_enabled" :{
    "type" : "boolean",
    "title" : "edt_eff_swirl_enabled",
    "default" : false
  }
```
The option `swirl_enabled` will be of type boolean with title `edt_eff_swirl_enabled` ([Titles will be translated](#translation)). The `default` sets the option to the defined default value, required!.

### String
An input field which accepts all kind of characters
``` json
  "swirl_name" :{
    "type" : "string",
    "title" : "edt_eff_swirl_name",
    "default" : "A cool placeholder name"
  }
```
The option `swirl_name` will be of type string with title  `edt_eff_swirl_name` ([Titles will be translated](#translation)). The `default` sets the option to the defined default value, required!. \
**Optional**
  * Add `"minLength" : 5` to force a minimum length of 5. Be aware that the default value matches
  * Add `"maxLength" : 9` to force a maximum length of 9. Be aware that the default value matches
  * Add `"enum" : ["Amazing1","Amazing","Amazing3"]` Renders the input as a select box where the user can choose one of the defined options.

### Integer
An input field for integer
``` json
  "swirl_count" :{
    "type" : "integer",
    "title" : "edt_eff_swirl_count",
    "default" : 5
  }
```
The option `swirl_count` will be of type integer with title `edt_eff_swirl_count` ([Titles will be translated](#translation)). The `default` sets the option to the defined default value, required!. \
**Optional**
  * Add `"minimum" : 5` to force a minimum value in case it shouldn't be lower
  * Add `"maximum" : 9` to force a maximum value in case it shouldn't be higher
  * Add `"step" : 2,` to define a alternate stepping of value. If not given, defaults to `1`. This doesn't prevent values which are "outside" of the step, it's more a helper if you use up/down keys and higher or smaller steps are wanted. 


### Number
A input field for numbers (float)
``` json
  "swirl_spread" :
  {
    "type" : "number",
    "title" : "edt_eff_swirl_spread",
    "default" : 7.0
  }
```
The option `swirl_spread` will be of type number (float) with title `edt_eff_swirl_spread` ([Titles will be translated](#translation)). The `default` sets the option to the defined default value, required!. \
**Optional**
  * Add `"minimum" : 5.0` to force a minimum value in case it shouldn't be lower
  * Add `"maximum" : 9.6` to force a maximum value in case it shouldn't be higher
  * Add `"step" : 0.1,` to define a alternate stepping of value. If not given, defaults to `1.0`. This doesn't prevent values which are "outside" of the step, it's more a helper if you use up/down keys and higher or smaller steps are wanted. 

### Select
Create a select element, where you can select one of the `enum` items. Default is required!
``` json
	"candles": {
		"type": "string",
		"title":"edt_eff_whichleds",
		"enum" : ["all","all-together","list"],
		"default" : "all"
	}
```

### Array
Creates an array input with the given properties at `items`. You can nest inside all kind of options
``` json
  "countries": {
    "type": "array",
    "uniqueItems": true,
    "title" : "edt_eff_countries",
    "items": {
      "type": "string",
      "title": "edt_eff_country"
    },
    "default":["de","at"]
  }
```
The option `countries` will be of type array (in python it's a python tuple) (shown as array where you can add or remove properties, in this case a string input field with the title `edt_eff_country"`) with the title `edt_eff_swirl_countries` ([Titles will be translated](#translation)). The `default` sets the option to the defined default value. Required! \
**Optional**
  * Add `"uniqueItems": true` if you want to make sure that each item is unique
  * Add `"minItems": 2` to force a minimum items count of the array
  * Add `"maxItems": 6` to force a maximum items count of the array

### Array - Multiselect
Create a selection where multiple elements from `ènum` can be selected. Default value is not required.
``` json
  "countries": {
		"type": "array",
		"title" : "edt_eff_countries",
		"uniqueItems": true,
		"items": {
      "type": "string",			
      "enum": ["de","at","fr","be","it","es","bg","ee","dk","fi","hu","ie","lv","lt","lu","mt","nl","pl","pt","ro","sl","se","ch"]
    },
    "default":["de","at"],
    "propertyOrder" : 1
  },
```

### Array - Colorpicker RGB
Creates a RGB colorpicker.
``` json
  "color" : {
    "type": "array",
    "title" : "edt_eff_color",
    "format":"colorpicker",
    "default" : [255,0,0],
    "items":{
      "type":"integer",
      "minimum": 0,
      "maximum": 255
    },
    "minItems": 3,
    "maxItems": 3
  }
```
The option `color` will be of type array (shown as RGB colorpicker) with the title `edt_eff_color` ([Titles will be translated](#translation)). This colorpicker will be set to initial red.

### Array - Colorpicker RGBA
Creates a RGBA colorpicker. Think twice brefore you provide a RGBA picker, the use case is limited. 
``` json
  "color" : {
    "type": "array",
    "title" : "edt_eff_color",
    "format":"colorpickerRGBA",
    "default" : [255,0,0,0.5],
    "minItems": 4,
    "maxItems": 4
  }
```
The option `color` will be of type array (shown as RGBA colorpicker) with the title `edt_eff_color` ([Titles will be translated](#translation)). This colorpicker will be set to red with 50% alpha initial. Required to add a default color.

## More beautification
To organize your UI better and make it prettier we provide a set of additional keywords.

### Dependencies
Hide/Show a specific option based on the value of another option
``` json{12}
  "enable-second": {
    "type": "boolean",
    "title":"edt_eff_enableSecondSwirl",
    "default": false,
    "propertyOrder" : 1
  },
  "random-center2": {
    "type": "boolean",
    "title":"edt_eff_randomCenter",
    "default": false,
    "options": {
      "dependencies": {
        "enable-second": true
      }
    }
  }
```
The option `random-center2` is NOT shown until the option `enable-second` is set to true. This also works with numbers, integers and strings.

### Order
As each option is a Object and the sort order for Objects is random you need to set an order on your own. Add a `propertyOrder` key.
``` json{5,11}
  "enable-second": {
    "type": "boolean",
    "title":"edt_eff_enableSecondSwirl",
    "default": false,
    "propertyOrder" : 1
  },
  "random-center2": {
    "type": "boolean",
    "title":"edt_eff_randomCenter",
    "default": false,
    "propertyOrder" : 2
  }
```
The option `enable-second` will be first, `random-center2` second.

### Field appends
You want a specific unit at the end of a field like "s", "ms" or "percent"? Just add a `append` with the wanted unit.
``` json{5}
  "interval": {
    "type": "integer",
    "title":"edt_eff_interval",
    "default": 5,
    "append" : "edt_append_s",
    "propertyOrder" : 1
  }
```
This will add a "s" for seconds to the input field. Please note it will be also translated, so check the translation file if your unit is already available. Add a new one if required.

### Smoothing control (only for effects)
Since v2 effects are no longer smoothed, it is possible to enable and manipulate smoothing if required. Add the following to the schema.
``` json
"smoothing-custom-settings":{  
  "type":"boolean",
  "title":"edt_eff_smooth_custom",
  "default":false,
  "propertyOrder":1
},
"smoothing-time_ms":{  
  "type":"integer",
  "title":"edt_eff_smooth_time_ms",
  "minimum":25,
  "maximum":600,
  "default":200,
  "append":"edt_append_ms",
  "options":{  
    "dependencies":{  
      "smoothing-custom-settings":true
    }
  },
  "propertyOrder":2
},
"smoothing-updateFrequency":{  
  "type":"number",
  "title":"edt_eff_smooth_updateFrequency",
  "minimum":1.0,
  "maximum":100.0,
  "default":25.0,
  "append":"edt_append_hz",
  "options":{  
    "dependencies":{  
      "smoothing-custom-settings":true
    }
  },
  "propertyOrder":3
}
```

### Translation
**only for effects - plugins will follow** \
To translate the effect options to a language we use placeholders that are translated during runtime into the target language.
It will usually look like this
`edt_eff_smooth` Available phrases begins with `edt_eff_` they are shared across all effects to prevent duplicates. Please search the [translation file](https://github.com/hyperion-project/hyperion.ng/blob/master/assets/webconfig/i18n/en.json) for a matching translation. If you don't find a matching phrase please add it.
