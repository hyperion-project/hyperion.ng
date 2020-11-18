## Authorization
Hyperion has an authorization system allowing users to login via password, and
applications to login with tokens. The user can configure how strong or weak the Hyperion API
should be protected from the `Configuration` -> `Network Services` panel on the Web UI.

[[toc]]

### Token System
Tokens are a simple way to authenticate an App for API access. They can be created in
the UI on the `Configuration` -> `Network Services` panel (the panel appears when `API
Authentication` options is checked). Your application can also [request a
token](#request-a-token) via the API.

### Authorization Check

Callers can check whether authorization is required to work with the API, by sending:
```json
{
    "command" : "authorize",
    "subcommand" : "tokenRequired"
}
```
If the property `required` is true, authentication is required. An example response:
```json
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
Login with a token as follows -- the caller will receive a [Login response](#login-response).
```json
{
    "command" : "authorize",
    "subcommand" : "login",
    "token" : "YourPrivateTokenHere"
}
```

### Login with Token over HTTP/S
Add the HTTP Authorization header to every request. On error, the user will get a failed [Login response](#login-response).
```http
  Authorization : token YourPrivateTokenHere
```

#### Login response
A successful login response:
```json
{
    "command" : "authorize-login",
    "success" : true,
    "tan" : 0
}
```

A failed login response:
```json
{
    "command" : "authorize-login",
    "error" : "No Authorization",
    "success" : false,
    "tan" : 0
}
```

### Logout
Users can also logout. Hyperion doesn't verify the login state, this call will always
return a success.

```json
{
    "command" : "authorize",
    "subcommand" : "logout"
}
```

Response:
```json
{
    "command" : "authorize-logout",
    "success" : true,
    "tan" : 0
}
```
::: warning
Logging out will stop all streaming data services and subscriptions
:::

### Request a Token

Here is the recommended workflow for your application to authenticate:
   * Ask Hyperion for a token along with a comment (a short meaningful string that
     identifies the caller is the most useful, e.g. includes an application name and
     device) and a short randomly created `id` (numbers/letters).
   * Wait for the response. The user will need to accept the token request from the Web UI.
   * On success: The call will return a UUID token that can be repeatedly used. Note that
  access could be revoked by the user at any time, but will continue to last for
  currently connected sessions in this case.
   * On error: The call won't get a token, which means the user either denied the request or it timed out (180s).

Request a token using the follow command, being sure to add a comment that is
descriptive enough for the Web UI user to make a decision as to whether to grant or deny
the request. The `id` field has 5 random chars created by the caller, which will appear
in the Web UI as the user considers granting their approval.
```json
{
    "command" : "authorize",
    "subcommand" : "requestToken",
    "comment" : "OpenHab 2 Binding",
    "id" : "T3c91"
}
```

After the call, a popup will appear in the Web UI to accept/reject the token request.
The calling application should show the comment and the id so that the user can confirm
the origin properly in the Hyperion UI. After 180 seconds without a user action, the
request is automatically rejected, and the caller will get a failure response (see below).

#### Success response
If the user accepted the token request the caller will get the following response:
```json
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
  * Save the token somewhere for further use. The token does not expire.
  * Be aware that a user can revoke the token. It will continue to function for currently connected sessions.

#### Failed response
A request will fail when either:
   * It times out (i.e. user neither approves nor rejects for 180 seconds after the request
     is sent).
   * User rejects the request.
```json
{
    "command" : "authorize-requestToken",
    "success" : false,
    "error" : "Token request timeout or denied"
}
```

#### Request abort
You can abort the token request by adding an "accept" property to the original request.
The request will be deleted:
```json
{
    "command" : "authorize",
    "subcommand" : "requestToken",
    "comment" : "OpenHab 2 Binding",
    "id" : "T3c91",
    "accept" : false
}
```
