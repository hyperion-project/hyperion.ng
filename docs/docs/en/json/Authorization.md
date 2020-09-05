## Authorization
Hyperion has an authorization system where people can login via password and applications with Tokens. The user can decide how strong or weak the Hyperion API should be protected.

[[toc]]

### Token System
Tokens are a simple way to authenticate an App for API access. They can be created "by hand" TODO LINK TO WEBUI TOKEN CREATION at the webconfiguration or your application can [request one](#request-a-token). 

### Authorization Check
It's for sure useful to check if you actually need an authorization to work with the API. Ask Hyperion by sending
``` json
{
    "command" : "authorize",
    "subcommand" : "tokenRequired"
}
```
If the property "required" is true, you need authentication. Example response.
``` json
{
    "command" : "authorize-tokenRequired",
    "info" : {
        "required" : true
        },
    "success" : true,
    "tan" :0
}
```

### Login with Token
Login with your token as follows. And get a [Login response](#login-response)
``` json
{
    "command" : "authorize",
    "subcommand" : "login",
    "token" : "YourPrivateTokenHere"
}
```

### Login with Token over HTTP/S
Add the HTTP Authorization header to every request. On error, you will get a failed [Login response](#login-response)
``` http
  Authorization : token YourPrivateTokenHere
```

#### Login response
Login success response
``` json
{
    "command" : "authorize-login",
    "success" : true,
    "tan" : 0
}
```

Login failed response
``` json
{
    "command" : "authorize-login",
    "error" : "No Authorization",
    "success" : false,
    "tan" : 0
}
```

### Logout
You can also logout. Hyperion doesn't verify the login state, it will be always a success.
``` json
{
    "command" : "authorize",
    "subcommand" : "logout"
}
```
Response
``` json
{
    "command" : "authorize-logout",
    "success" : true,
    "tan" : 0
}
```
::: warning
A Logout will stop all streaming data services and subscriptions
:::

### Request a Token
If you want to get the most comfortable way for your application to authenticate
  * You ask Hyperion for a token along with a comment (A text which identifies you as the sender, meaningful informations are desired - appname + device) and a short random created id (numbers/letters)
  * Wait for the response, the user needs to accept the request from the webconfiguration
  * -> On success you get a UUID token which is now your personal app token
  * -> On error you won't get a token, in this case the user denied the request or it timed out (180s).
  * Now you are able to access the API, your access can be revoked by the user at any time, but will last for current connected sessions.

Requesting a token is easy, just send the following command, make sure to add a sufficient comment. The "id" field has 5 random chars created by yourself. And add a meaningful comment.
``` json
{
    "command" : "authorize",
    "subcommand" : "requestToken",
    "comment" : "OpenHab 2 Binding",
    "id" : "T3c91"
}
```
Now you wait for the response, show a popup that the user should login to the webconfiguration and accept the token request. Show the comment and the id so that the user can confirm the origin properly. After 180 seconds without a user action, the request is automatically rejected. You will get a notification about the failure.

#### Success response
If the user accepted your token request you will get the following message.
  * Save the token somewhere for further use, it doesn't expire.
  * Be aware that a user can revoke the access. It will last for current connected sessions.
``` json
{
    "command" : "authorize-requestToken",
    "success" : true,
    "info": {
      "comment" : "OpenHab2 Binding",
      "id" : "T3c91",
      "token" : "YourPrivateTokenHere"
    }
}
```

#### Failed response
A request will fail when
  * Timeout - no user action for 180 seconds
  * User denied the request
``` json
{
    "command" : "authorize-requestToken",
    "success" : false,
    "error" : "Token request timeout or denied"
}
```

#### Request abort
You can abort the request by adding an "accept" property to the original request. The request will be deleted
``` json
{
    "command" : "authorize",
    "subcommand" : "requestToken",
    "comment" : "OpenHab 2 Binding",
    "id" : "T3c91",
    "accept" : false
}
```
  