# JSON-API Commands Overview

## Commands & Sub-Commands

List of commands and related sub-commands which can be used via JSON-API requests.

_Authorization (via password or bearer token)_

**No** - No authorization required<br>
**Yes** - Authorization required, but can be disabled for local network calls<br>
**Admin**: Authorization is always required

_Instance specific_

**Yes** - A specific instance can be addressed<br>
**Multi** - Multiple instances can be addressed via one request<br>
**No** - The command is not instance related

_http/s Support_

**Yes** - Command can be used by individual http/s requests<br>
**No** - Applies only to WebSocket or http/s sessions

| Command        | Sub-Command             | Authorization | Instance specific | http/s Support |
|:---------------|:------------------------|:--------------|:------------------|:---------------|
| adjustment     | -                       | Yes           | Multi             | Yes            |
| authorize      | adminRequired           | No            | No                | Yes            |
| authorize      | answerRequest           | Admin         | No                | No             |
| authorize      | createToken             | Admin         | No                | No             |
| authorize      | deleteToken             | Admin         | No                | Yes            |
| authorize      | getPendingTokenRequests | Admin         | No                | No             |
| authorize      | getTokenList            | Admin         | No                | Yes            |
| authorize      | login                   | No            | No                | No             |
| authorize      | logout                  | No            | No                | No             |
| authorize      | newPassword             | Admin         | No                | Yes            |
| authorize      | newPasswordRequired     | No            | No                | Yes            |
| authorize      | renameToken             | Admin         | No                | Yes            |
| authorize      | requestToken            | No            | No                | Yes            |
| authorize      | tokenRequired           | No            | No                | Yes            |
| clear          | -                       | Yes           | Multi             | Yes            |
| clearall       | -                       | Yes           | Multi             | Yes            |
| color          | -                       | Yes           | Multi             | Yes            |
| componentstate | -                       | Yes           | Multi             | Yes            |
| config         | getconfig               | Admin         | No                | Yes            |
| config         | getschema               | Admin         | No                | Yes            |
| config         | reload                  | Admin         | No                | Yes            |
| config         | restoreconfig           | Admin         | No                | Yes            |
| config         | setconfig               | Admin         | No                | Yes            |
| correction     | -                       | Yes           | Yes               | Yes            |
| create-effect  | -                       | Yes           | Yes               | Yes            |
| delete-effect  | -                       | Yes           | Yes               | Yes            |
| effect         | -                       | Yes           | Multi             | Yes            |
| image          | -                       | Yes           | Multi             | Yes            |
| inputsource    | discover                | Yes           | No                | Yes            |
| inputsource    | getProperties           | Yes           | No                | Yes            |
| instance       | createInstance          | Admin         | No                | Yes            |
| instance       | deleteInstance          | Admin         | No                | Yes            |
| instance       | saveName                | Admin         | No                | Yes            |
| instance       | startInstance           | Yes           | No                | Yes            |
| instance       | stopInstance            | Yes           | No                | Yes            |
| instance       | switchTo                | Yes           | No                | Yes            |
| ledcolors      | imagestream-start       | Yes           | Yes               | Yes            |
| ledcolors      | imagestream-stop        | Yes           | Yes               | Yes            |
| ledcolors      | ledstream-start         | Yes           | Yes               | Yes            |
| ledcolors      | ledstream-stop          | Yes           | Yes               | Yes            |
| leddevice      | addAuthorization        | Yes           | Yes               | Yes            |
| leddevice      | discover                | Yes           | Yes               | Yes            |
| leddevice      | getProperties           | Yes           | Yes               | Yes            |
| leddevice      | identify                | Yes           | Yes               | Yes            |
| logging        | start                   | Yes           | No                | Yes            |
| logging        | stop                    | Yes           | No                | Yes            |
| processing     | -                       | Yes           | Multi             | Yes            |
| serverinfo     | -                       | Yes           | Yes               | Yes            |
| serverinfo     | getInfo                 | Yes           | Yes               | Yes            |
| serverinfo     | subscribe               | Yes           | Yes               | No             |
| serverinfo     | unsubscribe             | Yes           | Yes               | No             |
| serverinfo     | getSubscriptions        | Yes           | Yes               | No             |
| serverinfo     | getSubscriptionCommands | No            | No                | No             |
| service        | discover                | Yes           | No                | Yes            |
| sourceselect   | -                       | Yes           | Multi             | Yes            |
| sysinfo        | -                       | Yes           | No                | Yes            |
| system         | restart                 | Yes           | No                | Yes            |
| system         | resume                  | Yes           | No                | Yes            |
| system         | suspend                 | Yes           | No                | Yes            |
| system         | toggleSuspend           | Yes           | No                | Yes            |
| system         | idle                    | Yes           | No                | Yes            |
| system         | toggleIdle              | Yes           | No                | Yes            |
| temperature    | -                       | Yes           | Yes               | Yes            |
| transform      | -                       | Yes           | Yes               | Yes            |
| videomode      | -                       | Yes           | No                | Yes            |

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

