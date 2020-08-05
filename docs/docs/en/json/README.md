# JSON RPC Introduction
The JSON-RPC provide lot's of possibilities to interact with Hyperion. You could get information about Hyperion and it's states and trigger certain actions based on these informations or just out of the wild.

[[toc]]

## What is JSON?
JSON is a standardized message format [JSON.org](http://www.json.org/) and is supported by lot's of programming languages, which is perfect to transmit and process informations. While it's not the smartest in traffic size, it can be read by humans.

### Sending JSON
Hyperion requires a special formatted JSON message to process it. `command` is always required, while `tan` is optional. The tan is a integer you could freely define. It is part of the response so you could easy filter for it in case you need it (Might be very rarely).
``` json
{
  "command" : "YourCommand",
  "tan" : 1
}
```
Depending on command, there might be an additional subcommand required
``` json
{
  "command" : "YourCommand",
  "subcommand" : "YourSubCommand",
  "tan" : 1
}
```
  
### Response
Most messages you send, trigger a response of the following format:
``` json
{
  "command" : "YourCommand",
  "info":{ ...DATA... },
  "success" : true,
  "tan" : 1
}
```
- command: The command you requested.
- tan: The tan you provided (If not, defaults to 1).
- success: true or false. In case of false you get a proper answer what's wrong within an **error** property.
- info: The data you requested (if so) 

## Connect
Supported are currently TCP Socket ("Json Server"), WebSocket and HTTP/S.
::: tip
You can discover Hyperion servers! Checkout [Detect Hyperion](/en/api/detect.md)
:::

### TCP Socket
Is a "raw" connection, you send and receive json from the json-rpc (default port: 19444). Also known as "Json Server".

### WebSocket
Part of the webserver (default port: 8090). You send and receive json from the json-rpc.
Supported is also WSS at port 8092. We support just TEXT mode. Read more at [Websocket](https://en.wikipedia.org/wiki/WebSocket|).

### HTTP/S Json
HTTP requests can be also send to the webserver (default port: 8090, for HTTPS: 8092).
Send a HTTP/S POST request along with a properly formatted json message at the body to the (example) url: `http://IpOfDevice:WebserverPort/json-rpc`
 
<ImageWrap src="/images/en/http_jsonrpc.jpg" alt="Control Hyperion with HTTP JSON RPC">
Example picture with a [Firefox](https://addons.mozilla.org/de/firefox/addon/restclient/)/[Chrome](https://chrome.google.com/webstore/detail/advanced-rest-client/hgmloofddffdnphfgcellkdfbfbjeloo/related) Addon to send HTTP JSON messages

</ImageWrap>

::: tip
If you get a "No Authorization" message back, you need to create an [Authorization Token](/en/json/Authorization.md#token-system)
:::

::: warning HTTP/S Restrictions
Please note that the HTTP JSON-RPC lacks of the following functions due to technical limitations.
- Image streams, led color streams, logging streams, subscriptions
:::

## API

### Server Info
All kind of infos from the Server: [Server Info](/en/json/ServerInfo.md)
### Control
Control Hyperion: [Control](/en/json/Control.md)
### Authorization
All around the Authorization system: [Authorization](/en/json/Authorization.md)
### Subscribe
Data subscriptions: [Subscribe](/en/json/Subscribe.md)
