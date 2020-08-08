# Effect development
Hyperion provides a powerful API to write your own effects, along with possible options and user interface to tune them.

[[toc]]

## Effect Files 
An effect has 3 different files.
|         File          |               language                |                                        Comment                                        |
| :-------------------: | :-----------------------------------: | :-----------------------------------------------------------------------------------: |
|     neweffect.py      |   [Python](https://www.python.org)    |                                The heart of the effect                                |
|    neweffect.json     |      [JSON](http://www.json.org)      |           Contains options for the python file, which makes it configurable           |
| neweffect.schema.json | [JSON Schema](http://json-schema.org) | Creates the options UI and is used to validate user input. [Read more](/en/api/ui.md) |