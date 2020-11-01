# JSON RPC Introduction
The JSON-RPC interfaces provides many ways to interact with Hyperion. You can retrieve
information about your server, your instances and take actions (such as setting a
priority input).

[[toc]]

## What is JSON?
JSON is a standardized message format (see [JSON.org](http://www.json.org/)) and is supported
by most programming languages. It is human readable which makes for easier debugging.

### Sending JSON
Hyperion requires a specially formatted JSON message. A `command` argument is always
required. A `tan` argument is optional. This is an integer you can freely choose -- it is
part of the response you will receive to allow you to filter the response from other server
messages (this functionality is likely necessary for advanced usecases only).

```json
{
  "command" : "YourCommand",
  "tan" : 1
}
```
Depending on the command, there might be an additional subcommand required:
```json
{
  "command" : "YourCommand",
  "subcommand" : "YourSubCommand",
  "tan" : 1
}
```
  
### Response
Most messages you send will trigger a response of the following format:
```json
{
  "command" : "YourCommand",
  "info":{ ...DATA... },
  "success" : true,
  "tan" : 1
}
```
- **command**: The command you requested.
- **tan**: The tan you provided (If not, it will default to 0 in the response).
- **success**: true or false. If false, an **error** argument will contain details of the issue.
- **info**: The data you requested (if any).

## Connect
Hyperion currently supports multiple connection mechanisms: TCP Socket ("Json Server"), WebSocket and HTTP/S.
::: tip
You can automatically discover Hyperion servers! See [Detect Hyperion](/en/api/detect.md)
:::

### TCP Socket
This is a "raw" connection, you can send and receive line-separated json from the server
(default port: 19444). This is also known as the "Json Server".

### WebSocket
This is part of the Hyperion webserver (default port: 8090). You send and receive json
commands. WSS is also supported on port 8092. Only TEXT mode is supported. Read more
about websockets at [Websocket](https://en.wikipedia.org/wiki/WebSocket|).

### HTTP/S Json
HTTP requests can also be sent to the webserver (default port: 8090, for HTTPS: 8092). Send a HTTP/S POST request along with a properly formatted json message in the body to the (example) url: `http://IpOfDevice:WebserverPort/json-rpc`
 
<ImageWrap src="/images/en/http_jsonrpc.jpg" alt="Control Hyperion with HTTP JSON RPC">
Example picture with a [Firefox](https://addons.mozilla.org/de/firefox/addon/restclient/)/[Chrome](https://chrome.google.com/webstore/detail/advanced-rest-client/hgmloofddffdnphfgcellkdfbfbjeloo/related) Addon to send HTTP JSON messages

</ImageWrap>

::: tip
If you get a "No Authorization" response, you need to create an [Authorization Token](/en/json/Authorization.md#token-system)
:::

::: warning HTTP/S Restrictions
Please note that the HTTP JSON-RPC lacks of the following functions due to technical limitations.
- Image streams, led color streams, logging streams, subscriptions
:::

## API

### Server Info
A large variety of data is available from the server: [Server Info](/en/json/ServerInfo.md)
### Control
Control your Hyperion server: [Control](/en/json/Control.md)
### Authorization
Authorization mechanisms: [Authorization](/en/json/Authorization.md)
### Subscribe
Data subscriptions: [Subscribe](/en/json/Subscribe.md)
