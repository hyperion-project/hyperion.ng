# Our first addon
You want to write an addon? You have no clue about everything? Perfect, the next steps will explain you everything you need to know. Also Python beginners are invited. Let's start with the addon types.

[[toc]]

## Addon types
Currently there are 2 types of Addons.
  * Service
  * Module

A service addon runs usually the whole time (if enabled), it starts and stops with the execution of Hyperion. Additional it has settings that can be configured from the user to fit their needs. This can be an IP address to another service, a time to trigger, or anything else that you will discover from the Addon API

A module provides utility methods which can be imported from a service addon to be used. Examples are Python packages from the PyPi repository like httplib2, which makes the developer life easier when working with HTTP requests. Instead implementing everything on your own you can rely on the work of others with well tested modules from the Python community. You can also create modules on your own if you think the code can be reused in other service addons.

## Development setup
All you need is:
  * An installed and running Hyperion. You need access to the filesystem where Hyperion has been installed
  * An editor to write code (recommended [Visual Studio Code](https://code.visualstudio.com/)) for Windows/Linux/Mac)

### Folder layout
The follow files are part of an addon.
| File                | language                               | Comment                                                                     |
| ------------------- | -------------------------------------- | --------------------------------------------------------------------------- |
| service.py          | [Python](https://www.python.org/)      | Required for `service` addons. Entry point, this is where your code goes to |
| addon.json          | [JSON](http://www.json.org/)           | The meta data file required for all addons                                  |
| settingsSchema.json | [JSON Schema](http://json-schema.org/) | Required for `service` addons. Options UI. [Read more](/en/api/ui.md)       |
| lib                 | Folder                                 | Required for `module` addons. Entry point for modules                       |

### Create a new service Addon
To create a service addon you need to follow a simple structure.
  - Navigate to ~/.hyperion/addons (folder in your home directory)
  - Think about a short "id", this id needs to be unique and should just contain letters (English), lowercase no whitespace Example: `kodicon` `plexcon` `timer` `cec`
  - Create a folder with your id and prepend `service.`. Example: `service.kodicon` `service.timer` `service.cec`
  - Inside this folder create a file called `service.py`, this is where your addon code goes to
  - Create a file called `settingsSchema.json`, this will represent our options
  - Create a new file called `addon.json` and add [metadata](#metadata|)
  - That's it!

::: tip
A prepared service playground addon can be downloaded from the repository. This shows the basic usage of the API but also how option UI works
:::

### Metadata
The metadata file called addon.json is a description of your addon to declare name, version, dependencies, support URL, sourcecode URL and more. This file is parsed by Hyperion to provide users required informations and download updates accordingly.

| Property     | Type   | Comment                                                                |
| ------------ | ------ | ---------------------------------------------------------------------- |
| name         | String | User friendly name of your addon                                        |
| description  | String | What does this addon as a brief overview                               |
| id           | String | The unique id                                                          |
| version      | String | The current version of your addon. [Semver 2.0.0](https://semver.org/) |
| category     | Array  | Add the addon to a category. Available `utility`                       |
| dependencies | Object | You can depend on modules / a specific minimum Hyperion version.       |
| changelog    | Array  | Array of Objects with notes                                            |
| provider     | String | Author of this addon                                                   |
| support      | String | A URL for support.                                                     |
| source       | String | A URL where the source code is hosted                                  |
| licence      | String | Licence type, can be MIT, GPL, LGPL                                    |

Here a version for copy and paste
``` json
{
	"name":"My addon Name",
	"description" : "What does my addon as a brief overview",
	"id":"service.myid",
	"version" : "0.1.0",
    "category" : "utility", 
	"dependencies" :{"hyperion" : "2.0.0"},
	"changelog" : [{"0.1.0 Crunchy Cudator":"-New featrues -bugfixs -other stuff"}],
	"provider":"hyperion-project",
	"support":"URL to hyperion-project.org Forum thread",
	"source" :"URL to source code",
	"licence" : "MIT"
}
```

### Addon categories
Addons can be assigned to  a category for better sorting. Select the best matching
  * **utility** A utility is usually a smaller addon, which does very basic tasks. It doesn't connect to another software. For example the Wake On LAN addon is a utility.
  * **integration** A integration addon interfaces with another software to listen for specific API events which triggers now actions on the Hyperion side or vice versa. Example is the Kodi addon
  * You can suggest new categories!

### Development
Enough preparation stuff, let's start! \
It's highly recommended to work/read once through a Python tutorial which explains you the principals of Python if you are new to Python.
  * New to Python? No problem, here is a interactive tutorial where you can read, write and execute your first python scripts inside your browser [learnpython](https://www.learnpython.org/), available in 7 languages!
  * Good beginner guide: [Python course EN](https://www.python-course.eu/python3_interactive.php). [Python Kurs DE](https://www.python-kurs.eu/python3_interaktiv.php). It's not necessary (nor possible) to understand everything, but the first pages are very helpful as a start!
