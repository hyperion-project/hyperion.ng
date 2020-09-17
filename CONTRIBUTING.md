## Translations
You can participate in the translation.
[![Join Translation](https://img.shields.io/badge/POEditor-translate-green.svg)](https://poeditor.com/join/project/Y4F6vHRFjA)

## Development Setup
You may already have an editor or IDE you want to use. In any case we provide a pre configured [Visual Studio Code](#visual-studio-code) setup.

## Workflow

### Issue

> TODO

### Pull requests

- Create a feature branch from the default branch (`master`) and merge back against that branch.
- It's OK to have multiple small commits as you work on the PR - GitHub automatically squashes them before merging.
- Make sure tests pass.
- If adding a new feature:
  - Provide a convincing reason to add this feature. Ideally, you should open a suggestion issue first and have it approved before working on it.
- If fixing bug:
  - If you are resolving an open issue, add `(fix #xxxx)` (`#xxxx` being the issue ID) in your PR title for a better release log, e.g. `chore(feat): implement SSR (fix #1234)`.
  - Provide a detailed description of the bug in the PR.

## Code Specification

- use QT wherever it's possible (except there is a good reason)
- use unix line endings (not windows)
- indent your code with TABs instead of spaces
- your files should end with a newline
- names are camel case
- use utf8 file encoding (ANSI encoding is strictly forbidden!)
- use speaking names for variables.
- avoid code dups -> if you write similar code blocks more the 2 times -> refactoring!
- avoid compiler macros (#ifdef #define ...) where possible
- class member variables must prefixed with underscore `int _myMemberVar`
- initializer list on constructors:

```c++
bad:
MyClass::MyClass()
	: myVarA(0), myVarB("eee"), myVarC(true)
{
}

MyClass::MyClass() : myVarA(0),
	myVarB("eee"),
	myVarC(true)
{
}

good:
MyClass::MyClass()
	: myVarA(0)
	, myVarB("eee")
	, myVarC(true)
{
}
```

- pointer declaration

```c++
bad:
int *foo;
int * fooFoo;

good:
int* foo;
```

### Logger
Hyperion has a own logger class with different log levels.
 - Use macros in include/utils/logger.h
 - Don't use the Logger class directly unless there is a good reason to do so.
``` c++
// *** including
#include <utils/logger.h>

// creating
// get a logger, this will create a logger named MAIN with min loglevel INFO, DEBUG messages won't displayed
Logger * log_main = Logger::getInstance("MAIN");

// get a logger, this will create a logger named MAIN with min loglevel DEBUG,  all messages displayed
Logger * log_main = Logger::getInstance("MAIN", Logger::DEBUG);

// using
// basic
Debug( log_main, "hello folks!");
Info( log_main, "hello again!");
Warning( log_main, "something is crazy");
Error( log_main, "oh to crazy, aborting");

// quick logging, when only one message exists and want no typing overhead - or usage in static functions
Info( Logger::getInstance("LedDevice"), "Leddevice %s started", "PublicStreetLighting");

// a bit more complex - with printf like format
Info( log_main, "hello %s, you have %d messages", "Dax", 25);

// conditional messages
WarningIf( (value>threshold), log_main, "Alert, your value is greater then %d", threshold );
```
The amount of "%" must match with following arguments

#### The Placeholders
 - %s for strings (this are cstrings, when having std::string use myStdString.c_str() to convert)
 - %d for integer numbers
 - %f for float numbers
 - more placeholders possible, see [here](http://www.cplusplus.com/reference/cstdio/printf/)

#### Log Level
  * Debug - used when message is more or less for the developer or for trouble shooting
  * Info - used for not absolutely developer stuff messages for what's going on
  * Warning - warn if something is not as it should be, but didn't harm
  * Error - used when an error occurs


## Commit specification

> TODO

## Visual Studio Code
**We assume that you successfully compiled Hyperion with the [Compile HowTo](CompileHowto.md) WITHOUT Docker** \
If you want to use VSCode for development follow the steps.

- Install [VSCode](https://code.visualstudio.com/). On Ubuntu 16.04+ you can also use the [Snapcraft VSCode](https://snapcraft.io/code) package.
- Linux: Install gdb `sudo apt-get install gdb`
- Mac: ?
- Open VSCode and click on _File_ -> _Open Workspace_ and select the file `hyperion.ng/.vscode/hyperion.code-workspace`
- Install recommended extensions
- If you installed the Task Explorer you can now use the defined vscode tasks to build Hyperion and configure cmake
- For debugging you need to build Hyperion in Debug mode and start the correct Run config
