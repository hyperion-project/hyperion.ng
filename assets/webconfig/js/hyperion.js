// global vars (read and write in window object)
window.webPrio = 1;
window.webOrigin = "Web Configuration";
window.showOptHelp = true;
window.gitHubReleaseApiUrl = "https://api.github.com/repos/hyperion-project/hyperion.ng/releases";
window.currentChannel = null;
window.currentVersion = null;
window.latestVersion = null;
window.latestStableVersion = null;
window.latestBetaVersion = null;
window.latestAlphaVersion = null;
window.latestRcVersion = null;
window.gitHubVersionList = null;
window.serverInfo = {};
window.serverSchema = {};
window.serverConfig = {};
window.schema = {};
window.sysInfo = {};
window.jsonPort = 8090;
window.websocket = null;
window.hyperion = {};
window.wsTan = 1;
window.ledStreamActive = false;
window.imageStreamActive = false;
window.loggingStreamActive = false;
window.loggingHandlerInstalled = false;
window.watchdog = 0;
window.debugMessagesActive = true;
window.currentHyperionInstance = 0;
window.currentHyperionInstanceName = "?";
window.comps = [];
window.defaultPasswordIsSet = null;
tokenList = {};

const ENDLESS = -1;

function initRestart()
{
  $(window.hyperion).off();
  requestServerConfigReload();
  window.watchdog = 10;
  connectionLostDetection('restart');
}

function connectionLostDetection(type)
{
  if ( window.watchdog > 2 )
  {
    var interval_id = window.setInterval(function(){clearInterval(interval_id);}, 9999); // Get a reference to the last
    for (var i = 1; i < interval_id; i++)
      window.clearInterval(i);
    if(type == 'restart')
    {
      $("body").html($("#container_restart").html());
      // setTimeout delay for probably slower systems, some browser don't execute THIS action
      setTimeout(restartAction,250);
    }
    else
    {
      $("body").html($("#container_connection_lost").html());
      connectionLostAction();
    }
  }
  else
  {
    $.get( "/cgi/cfg_jsonserver", function() {window.watchdog=0}).fail(function() {window.watchdog++;});
  }
}

setInterval(connectionLostDetection, 3000);

// init websocket to hyperion and bind socket events to jquery events of $(hyperion) object

function initWebSocket()
{
  if ("WebSocket" in window)
  {
    if (window.websocket == null)
    {
      window.jsonPort = '';
      if(document.location.port == '' && document.location.protocol == "http:")
        window.jsonPort = '80';
      else if (document.location.port == '' && document.location.protocol == "https:")
        window.jsonPort = '443';
      else
        window.jsonPort = document.location.port;
      window.websocket = (document.location.protocol == "https:") ? new WebSocket('wss://'+document.location.hostname+":"+window.jsonPort) : new WebSocket('ws://'+document.location.hostname+":"+window.jsonPort);

      window.websocket.onopen = function (event) {
        $(window.hyperion).trigger({type:"open"});

        $(window.hyperion).on("cmd-serverinfo", function(event) {
          window.watchdog = 0;
        });
      };

      window.websocket.onclose = function (event) {
        // See http://tools.ietf.org/html/rfc6455#section-7.4.1
        var reason;
        switch(event.code)
        {
          case 1000: reason = "Normal closure, meaning that the purpose for which the connection was established has been fulfilled."; break;
          case 1001: reason = "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page."; break;
          case 1002: reason = "An endpoint is terminating the connection due to a protocol error"; break;
          case 1003: reason = "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message)."; break;
          case 1004: reason = "Reserved. The specific meaning might be defined in the future."; break;
          case 1005: reason = "No status code was actually present."; break;
          case 1006: reason = "The connection was closed abnormally, e.g., without sending or receiving a Close control frame"; break;
          case 1007: reason = "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message)."; break;
          case 1008: reason = "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy."; break;
          case 1009: reason = "An endpoint is terminating the connection because it has received a message that is too big for it to process."; break;
          case 1010: reason = "An endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. <br /> Specifically, the extensions that are needed are: " + event.reason; break;
          case 1011: reason = "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request."; break;
          case 1015: reason = "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified)."; break;
          default: reason = "Unknown reason";
        }
        $(window.hyperion).trigger({type:"close", reason:reason});
        window.watchdog = 10;
        connectionLostDetection();
      };

      window.websocket.onmessage = function (event) {
        try
        {
          var response = JSON.parse(event.data);
          var success = response.success;
          var cmd = response.command;
          var tan = response.tan
          if (success || typeof(success) == "undefined")
          {
            $(window.hyperion).trigger({type:"cmd-"+cmd, response:response});
          }
          else
          {
              // skip tan -1 error handling
              if(tan != -1){
                var error = response.hasOwnProperty("error")? response.error : "unknown";
                if (error == "Service Unavailable") {
                  window.location.reload();
                } else {
                  $(window.hyperion).trigger({type:"error",reason:error});
                }
                console.log("[window.websocket::onmessage] ",error)
              }
          }
        }
        catch(exception_error)
        {
          $(window.hyperion).trigger({type:"error",reason:exception_error});
          console.log("[window.websocket::onmessage] ",exception_error)
        }
      };

      window.websocket.onerror = function (error) {
        $(window.hyperion).trigger({type:"error",reason:error});
        console.log("[window.websocket::onerror] ",error)
      };
    }
  }
  else
  {
    $(window.hyperion).trigger("error");
    alert("Websocket is not supported by your browser");
    return;
  }
}

function sendToHyperion(command, subcommand, msg)
{
  if (typeof subcommand != 'undefined' && subcommand.length > 0)
    subcommand = ',"subcommand":"'+subcommand+'"';
  else
    subcommand = "";

  if (typeof msg != 'undefined' && msg.length > 0)
    msg = ","+msg;
  else
    msg = "";

  window.websocket.send('{"command":"'+command+'", "tan":'+window.wsTan+subcommand+msg+'}');
}

// Send a json message to Hyperion and wait for a matching response
// A response matches, when command(+subcommand) of request and response is the same
// command:    The string command
// subcommand: The optional string subcommand
// data:       The json data as Object
// tan:        The optional tan, default 1. If the tan is -1, we skip global response error handling
// Returns data of response or false if timeout
async function sendAsyncToHyperion (command, subcommand, data, tan = 1) {
  let obj = { command, tan }
  if (subcommand) {Object.assign(obj, {subcommand})}
  if (data) { Object.assign(obj, data) }

  //if (process.env.DEV || sstore.getters['common/getDebugState']) console.log('SENDAS', obj)
  return __sendAsync(obj)
}

// Send a json message to Hyperion and wait for a matching response
// A response matches, when command(+subcommand) of request and response is the same
// Returns data of response or false if timeout
async function __sendAsync (data) {
  return new Promise((resolve, reject) => {
    let cmd = data.command
    let subc = data.subcommand
    let tan = data.tan;
    if (subc)
      cmd = `${cmd}-${subc}`

    let func = (e) => {
      let rdata;
      try {
        rdata = JSON.parse(e.data)
      } catch (error) {
        console.error("[window.websocket::onmessage] ",error)
        resolve(false)
      }
      if (rdata.command == cmd && rdata.tan == tan) {
        window.websocket.removeEventListener('message', func)
        resolve(rdata)
      }
    }
    // after 7 sec we resolve false
    setTimeout(() => { window.websocket.removeEventListener('message', func); resolve(false) }, 7000)
    window.websocket.addEventListener('message', func)
    window.websocket.send(JSON.stringify(data) + '\n')
  })
}

// -----------------------------------------------------------
// wrapped server commands

// Test if admin requires authentication
function requestRequiresAdminAuth()
{
  sendToHyperion("authorize","adminRequired");
}
// Test if the default password needs to be changed
function requestRequiresDefaultPasswortChange()
{
  sendToHyperion("authorize","newPasswordRequired");
}
// Change password
function requestChangePassword(oldPw, newPw)
{
  sendToHyperion("authorize","newPassword",'"password": "'+oldPw+'", "newPassword":"'+newPw+'"');
}

function requestAuthorization(password)
{
  sendToHyperion("authorize","login",'"password": "' + password + '"');
}

function requestTokenAuthorization(token)
{
  sendToHyperion("authorize","login",'"token": "' + token + '"');
}

function requestToken(comment)
{
  sendToHyperion("authorize","createToken",'"comment": "'+comment+'"');
}

function requestTokenInfo()
{
  sendToHyperion("authorize","getTokenList","");
}

function requestGetPendingTokenRequests (id, state) {
  sendToHyperion("authorize", "getPendingTokenRequests", "");
}

function requestHandleTokenRequest(id, state)
{
  sendToHyperion("authorize","answerRequest",'"id":"'+id+'", "accept":'+state);
}

function requestTokenDelete(id)
{
  sendToHyperion("authorize","deleteToken",'"id":"'+id+'"');
}

function requestInstanceRename(inst, name)
{
  sendToHyperion("instance", "saveName",'"instance": '+inst+', "name": "'+name+'"');
}

function requestInstanceStartStop(inst, start)
{
  if(start)
    sendToHyperion("instance","startInstance",'"instance": '+inst);
  else
    sendToHyperion("instance","stopInstance",'"instance": '+inst);
}

function requestInstanceDelete(inst)
{
  sendToHyperion("instance","deleteInstance",'"instance": '+inst);
}

function requestInstanceCreate(name)
{
  sendToHyperion("instance","createInstance",'"name": "'+name+'"');
}

function requestInstanceSwitch(inst)
{
  sendToHyperion("instance","switchTo",'"instance": '+inst);
}

function requestServerInfo()
{
  sendToHyperion("serverinfo","",'"subscribe":["components-update", "priorities-update", "imageToLedMapping-update", "adjustment-update", "videomode-update", "effects-update", "settings-update", "instance-update"]');
}

function requestSysInfo()
{
  sendToHyperion("sysinfo");
}

function requestSystemSuspend()
{
  sendToHyperion("system","suspend");
}

function requestSystemResume()
{
  sendToHyperion("system","resume");
}

function requestSystemRestart()
{
  sendToHyperion("system","restart");
}

function requestServerConfigSchema()
{
  sendToHyperion("config","getschema");
}

function requestServerConfig()
{
  sendToHyperion("config", "getconfig");
}

function requestServerConfigReload()
{
  sendToHyperion("config", "reload");
}

function requestLedColorsStart()
{
  window.ledStreamActive=true;
  sendToHyperion("ledcolors", "ledstream-start");
}

function requestLedColorsStop()
{
  window.ledStreamActive=false;
  sendToHyperion("ledcolors", "ledstream-stop");
}

function requestLedImageStart()
{
  window.imageStreamActive=true;
  sendToHyperion("ledcolors", "imagestream-start");
}

function requestLedImageStop()
{
  window.imageStreamActive=false;
  sendToHyperion("ledcolors", "imagestream-stop");
}

function requestPriorityClear(prio)
{
  if(typeof prio !== 'number')
    prio = window.webPrio;

  $(window.hyperion).trigger({type:"stopBrowerScreenCapture"});   
  sendToHyperion("clear", "", '"priority":'+prio+'');
}

function requestClearAll()
{
  $(window.hyperion).trigger({type:"stopBrowerScreenCapture"});   
  requestPriorityClear(-1)
}

function requestPlayEffect(effectName, duration)
{
  $(window.hyperion).trigger({type:"stopBrowerScreenCapture"});   
  sendToHyperion("effect", "", '"effect":{"name":"'+effectName+'"},"priority":'+window.webPrio+',"duration":'+validateDuration(duration)+',"origin":"'+window.webOrigin+'"');
}

function requestSetColor(r,g,b,duration)
{
  $(window.hyperion).trigger({type:"stopBrowerScreenCapture"});   
  sendToHyperion("color", "",  '"color":['+r+','+g+','+b+'], "priority":'+window.webPrio+',"duration":'+validateDuration(duration)+',"origin":"'+window.webOrigin+'"');
}

function requestSetImage(data,duration,name)
{
  sendToHyperion("image", "",  '"imagedata":"'+data+'", "priority":'+window.webPrio+',"duration":'+validateDuration(duration)+', "format":"auto", "origin":"'+window.webOrigin+'", "name":"'+name+'"');
}

function requestSetComponentState(comp, state)
{
  var state_str = state ? "true" : "false";
  sendToHyperion("componentstate", "", '"componentstate":{"component":"'+comp+'","state":'+state_str+'}');
}

function requestSetSource(prio)
{
  if ( prio == "auto" )
    sendToHyperion("sourceselect", "", '"auto":true');
  else
    sendToHyperion("sourceselect", "", '"priority":'+prio);
}

function requestWriteConfig(config, full)
{
  if(full === true)
    window.serverConfig = config;
  else
  {
    jQuery.each(config, function(i, val) {
      window.serverConfig[i] = val;
    });
  }

  sendToHyperion("config","setconfig", '"config":'+JSON.stringify(window.serverConfig));
}

function requestRestoreConfig(config) {
  sendToHyperion("config", "restoreconfig", '"config":' + JSON.stringify(config));
}

function requestWriteEffect(effectName,effectPy,effectArgs,data)
{
  var cutArgs = effectArgs.slice(1, -1);
  sendToHyperion("create-effect", "", '"name":"'+effectName+'", "script":"'+effectPy+'", '+cutArgs+',"imageData":"'+data+'"');
}

function requestTestEffect(effectName,effectPy,effectArgs,data)
{
  sendToHyperion("effect", "", '"effect":{"name":"'+effectName+'", "args":'+effectArgs+'}, "priority":'+window.webPrio+', "origin":"'+window.webOrigin+'", "pythonScript":"'+effectPy+'", "imageData":"'+data+'"');
}

function requestDeleteEffect(effectName)
{
  sendToHyperion("delete-effect", "", '"name":"'+effectName+'"');
}

function requestLoggingStart()
{
  window.loggingStreamActive=true;
  sendToHyperion("logging", "start");
}

function requestLoggingStop()
{
  window.loggingStreamActive=false;
  sendToHyperion("logging", "stop");
}

function requestMappingType(type)
{
  sendToHyperion("processing", "", '"mappingType": "'+type+'"');
}

function requestVideoMode(newMode)
{
  sendToHyperion("videomode", "", '"videoMode": "'+newMode+'"');
}

function requestAdjustment(type, value, complete)
{
  if(complete === true)
    sendToHyperion("adjustment", "", '"adjustment": '+type+'');
  else
    sendToHyperion("adjustment", "", '"adjustment": {"'+type+'": '+value+'}');
}

async function requestLedDeviceDiscovery(type, params)
{
  let data = { ledDeviceType: type, params: params };

  return sendAsyncToHyperion("leddevice", "discover", data, Math.floor(Math.random() * 1000) );
}

async function requestLedDeviceProperties(type, params)
{
  let data = { ledDeviceType: type, params: params };

  return sendAsyncToHyperion("leddevice", "getProperties", data, Math.floor(Math.random() * 1000));
}

function requestLedDeviceIdentification(type, params)
{
    let data = { ledDeviceType: type, params: params };

  return sendAsyncToHyperion("leddevice", "identify", data, Math.floor(Math.random() * 1000));
}

async function requestLedDeviceAddAuthorization(type, params) {
  let data = { ledDeviceType: type, params: params };

  return sendAsyncToHyperion("leddevice", "addAuthorization", data, Math.floor(Math.random() * 1000));
}

async function requestInputSourcesDiscovery(type, params) {
  let data = { sourceType: type, params: params };

  return sendAsyncToHyperion("inputsource", "discover", data, Math.floor(Math.random() * 1000));
}

async function requestServiceDiscovery(type, params) {
  let data = { serviceType: type, params: params };

  return sendAsyncToHyperion("service", "discover", data, Math.floor(Math.random() * 1000));
}

