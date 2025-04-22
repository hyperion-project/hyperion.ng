# JSON-API Commands Overview

## Commands & Sub-Commands

List of commands and related sub-commands which can be used via JSON-API requests.

_Authorization (via password or bearer token)_

**No** - No authorization required<br>
**Yes** - Authorization required, but can be disabled for local network calls<br>
**Admin**: Authorization is always required

_Instance Cmd_

**Single** - A specific instance can be addressed<br>
**Multi** - Multiple instances can be addressed via one request<br>
**No** - The command is not instance related

_Instance must Run_

**No** - The instance is not required to run<br>
**Yes** - The instance the command is to be applied to must be running<br>

_http/s Support_

**Yes** - Command can be used by individual http/s requests<br>
**No** - Applies only to WebSocket or http/s sessions

| Command        | Sub-Command             | Authorization | Instance Cmd | Instance must Run | http/s Support |
|----------------|-------------------------|---------------|--------------|-------------------|----------------|
| adjustment     |                         | Yes           | Multi        | Yes               | Yes            |
| authorize      | adminRequired           | No            | No           | No                | Yes            |
| authorize      | answerRequest           | Admin         | No           | No                | No             |
| authorize      | createToken             | Admin         | No           | No                | No             |
| authorize      | deleteToken             | Admin         | No           | No                | Yes            |
| authorize      | getPendingTokenRequests | Admin         | No           | No                | No             |
| authorize      | getTokenList            | Admin         | No           | No                | Yes            |
| authorize      | login                   | No            | No           | No                | No             |
| authorize      | logout                  | No            | No           | No                | No             |
| authorize      | newPassword             | Admin         | No           | No                | Yes            |
| authorize      | newPasswordRequired     | No            | No           | No                | Yes            |
| authorize      | renameToken             | Admin         | No           | No                | Yes            |
| authorize      | requestToken            | No            | No           | No                | Yes            |
| authorize      | tokenRequired           | No            | No           | No                | Yes            |
| clear          |                         | Yes           | Multi        | Yes               | Yes            |
| clearall       |                         | Yes           | Multi        | Yes               | Yes            |
| color          |                         | Yes           | Multi        | Yes               | Yes            |
| componentstate |                         | Yes           | No or Multi  | Yes               | Yes            |
| config         | getconfig               | Admin         | No           | No                | Yes            |
| config         | getschema               | Admin         | No           | No                | Yes            |
| config         | reload                  | Admin         | No           | No                | Yes            |
| config         | restoreconfig           | Admin         | No           | No                | Yes            |
| config         | setconfig               | Admin         | No           | No                | Yes            |
| correction     |                         | Yes           | Single       | Yes               | Yes            |
| create-effect  |                         | Yes           | No           | No                | Yes            |
| delete-effect  |                         | Yes           | No           | No                | Yes            |
| effect         |                         | Yes           | Multi        | Yes               | Yes            |
| image          |                         | Yes           | Multi        | Yes               | Yes            |
| inputsource    | discover                | Yes           | No           | No                | Yes            |
| inputsource    | getProperties           | Yes           | No           | No                | Yes            |
| instance       | createInstance          | Admin         | No           | No                | Yes            |
| instance       | deleteInstance          | Admin         | No           | No                | Yes            |
| instance       | saveName                | Admin         | No           | No                | Yes            |
| instance       | startInstance           | Yes           | No           | No                | Yes            |
| instance       | stopInstance            | Yes           | No           | No                | Yes            |
| instance       | switchTo                | Yes           | No           | No                | Yes            |
| instance-data  | getImageSnapshot        | Yes           | Single       | Yes               | Yes            |
| instance-data  | getLedSnapshot          | Yes           | Single       | Yes               | Yes            |
| ledcolors      | imagestream-start       | Yes           | Single       | Yes               | Yes            |
| ledcolors      | imagestream-stop        | Yes           | Single       | Yes               | Yes            |
| ledcolors      | ledstream-start         | Yes           | Single       | Yes               | Yes            |
| ledcolors      | ledstream-stop          | Yes           | Single       | Yes               | Yes            |
| leddevice      | addAuthorization        | Yes           | Single       | Yes               | Yes            |
| leddevice      | discover                | Yes           | No           | No                | Yes            |
| leddevice      | getProperties           | Yes           | No           | No                | Yes            |
| leddevice      | identify                | Yes           | No           | No                | Yes            |
| logging        | start                   | Yes           | No           | No                | Yes            |
| logging        | stop                    | Yes           | No           | No                | Yes            |
| processing     |                         | Yes           | Multi        | Yes               | Yes            |
| serverinfo     |                         | Yes           | Single       | Yes               | Yes            |
| serverinfo     | getInfo                 | Yes           | No or Single | Yes               | Yes            |
| serverinfo     | subscribe               | Yes           | No or Single | Yes               | No             |
| serverinfo     | unsubscribe             | Yes           | No or Single | Yes               | No             |
| serverinfo     | getSubscriptions        | Yes           | No or Single | Yes               | No             |
| serverinfo     | getSubscriptionCommands | No            | No           | No                | No             |
| service        | discover                | Yes           | No           | No                | Yes            |
| sourceselect   |                         | Yes           | Multi        | Yes               | Yes            |
| sysinfo        |                         | Yes           | No           | No                | Yes            |
| system         | restart                 | Yes           | No           | No                | Yes            |
| system         | resume                  | Yes           | No           | No                | Yes            |
| system         | suspend                 | Yes           | No           | No                | Yes            |
| system         | toggleSuspend           | Yes           | No           | No                | Yes            |
| system         | idle                    | Yes           | No           | No                | Yes            |
| system         | toggleIdle              | Yes           | No           | No                | Yes            |
| temperature    |                         | Yes           | Single       | Yes               | Yes            |
| transform      |                         | Yes           | Single       | Yes               | Yes            |
| videomode      |                         | Yes           | No           | No                | Yes            |

## Subscription updates

List of updates which can be subscribed to via the `serverinfo/subscribe`request.

_Instance specific_

**Yes** - A specific instance can be addressed<br>
**No** - The command is not instance related

_in "all"_

**Yes** - Updates are subscribed using "all" as the command<br>
**No** - Subscription is only triggered via JSON-API request

| Subscription Command         | Instance specific | in "all" |
|:-----------------------------|:------------------|:---------|
| adjustment-update            | Yes               | Yes      |
| components-update            | Yes               | Yes      |
| effects-update               | Yes               | Yes      |
| event-update                 | No                | Yes      |
| imageToLedMapping-update     | Yes               | Yes      |
| instance-update              | Yes               | Yes      |
| ledcolors-imagestream-update | Yes               | No       |
| ledcolors-ledstream-update   | Yes               | No       |
| leds-update                  | Yes               | Yes      |
| logmsg-update                | No                | No       |
| priorities-update            | Yes               | Yes      |
| settings-update              | Yes               | Yes      |
| token-update                 | No                | Yes      |
| videomode-update             | No                | Yes      |

