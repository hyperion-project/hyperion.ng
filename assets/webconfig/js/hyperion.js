//Constants
const INPUT = Object.freeze({
  ORIGIN: "Hyperion Web-UI",
  FG_PRIORITY: 1
});

// global vars (read and write in window object)
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
window.currentHyperionInstance = null;
window.currentHyperionInstanceName = "?";
window.comps = [];
window.defaultPasswordIsSet = null;
let tokenList = {};

function initRestart() {
  $(window.hyperion).off();
  window.watchdog = 10;
  connectionLostDetection('restart');
}

function connectionLostDetection(type) {
  if (window.watchdog > 2) {
    const interval_id = window.setInterval(function () { clearInterval(interval_id); }, 9999); // Get a reference to the last
    for (let i = 1; i < interval_id; i++)
      window.clearInterval(i);
    if (type == 'restart') {
      $("body").html($("#container_restart").html());
      // setTimeout delay for probably slower systems, some browser don't execute THIS action
      setTimeout(restartAction, 250);
    }
    else {
      $("body").html($("#container_connection_lost").html());
      connectionLostAction();
    }
  }
  else {
    $.get("/cgi/cfg_jsonserver", function () { window.watchdog = 0 }).fail(function () { window.watchdog++; });
  }
}

// Utility function to sanitize strings for safe logging
function sanitizeForLog(input) {
  if (typeof input !== 'string') return '';
  return input
    .replace(/[\n\r\t]/g, ' ') // Replace newlines, carriage returns, and tabs with space
    .replace(/[\u001b\u009b][[()#;?]*(?:[0-9]{1,4}(?:;[0-9]{0,4})*)?[0-9A-ORZcf-nqry=><]/g, ''); // Remove ANSI escape codes
}

setInterval(connectionLostDetection, 3000);

// init websocket to hyperion and bind socket events to jquery events of $(hyperion) object

function initWebSocket() {
  if ("WebSocket" in window) {
    if (window.websocket == null) {
      window.jsonPort = '';
      if (document.location.port == '' && document.location.protocol == "http:")
        window.jsonPort = '80';
      else if (document.location.port == '' && document.location.protocol == "https:")
        window.jsonPort = '443';
      else
        window.jsonPort = document.location.port;
      window.websocket = (document.location.protocol == "https:") ? new WebSocket('wss://' + document.location.hostname + ":" + window.jsonPort) : new WebSocket('ws://' + document.location.hostname + ":" + window.jsonPort);

      window.websocket.onopen = function (event) {
        $(window.hyperion).trigger({ type: "open" });

        $(window.hyperion).on("cmd-serverinfo", function (event) {
          window.watchdog = 0;
        });
      };

      window.websocket.onclose = function (event) {
        // See http://tools.ietf.org/html/rfc6455#section-7.4.1
        let reason;
        switch (event.code) {
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
        $(window.hyperion).trigger({ type: "close", reason: reason });
        window.watchdog = 10;
        connectionLostDetection();
      };

      window.websocket.onmessage = function (event) {
        try {
          const response = JSON.parse(event.data);
          const success = response.success;
          const cmd = response.command;
          const tan = response.tan
          if (success || typeof (success) == "undefined") {
            $(window.hyperion).trigger({ type: "cmd-" + cmd, response: response });
          }
          else
            if (tan != -1) {
              // skip tan -1 error handling
              const error = response.hasOwnProperty("error") ? response.error : "unknown";
              if (error == "Service Unavailable") {
                window.location.reload();
              } else {
                const errorData = Array.isArray(response.errorData) ? response.errorData : [];

                // Sanitize provided input
                const logError = error.replace(/\n|\r/g, "");
                const logErrorData = JSON.stringify(errorData).replace(/[\r\n\t]/g, ' ')
                console.log("[window.websocket::onmessage] ", logError, ", Description:", logErrorData);

                $(window.hyperion).trigger({
                  type: "error",
                  reason: {
                    cmd: cmd,
                    message: error,
                    details: errorData.map((item) => item.description || "")
                  }
                });
              }
            }
        }
        catch (exception_error) {
          console.log("[window.websocket::onmessage] ", exception_error);
          $(window.hyperion).trigger({
            type: "error",
            reason: {
              message: $.i18n("ws_processing_exception") + ": " + exception_error.message,
              details: [exception_error.stack]
            }
          });
        }
      };

      window.websocket.onerror = function (error) {
        console.log("[window.websocket::onerror] ", error);
        $(window.hyperion).trigger({
          type: "error",
          reason: {
            message: $.i18n("ws_error_occured"),
            details: [error]
          }
        });
      };
    }
  }
  else {
    $(window.hyperion).trigger({
      type: "error",
      reason: {
        message: $.i18n("ws_not_supported"),
        details: []
      }
    });
  }
}

function sendToHyperion(command, subcommand, msg, instanceIds = null) {
  const tan = Math.floor(Math.random() * 1000); // Generate a transaction number

  // Build the base object
  const message = {
    command: command,
    tan: tan,
  };

  // Add the subcommand if provided
  if (subcommand) {
    message.subcommand = subcommand;
  }

  // Add the instanceID(s) the command is to be applied to
  if (instanceIds != null) {
      message.instance = instanceIds;
  }

  // Merge the msg object into the final message if provided
  if (msg && typeof msg === "object") {
    Object.assign(message, msg);
  }

  // Send the serialized message over WebSocket
  window.websocket.send(JSON.stringify(message));
}

// Send a json message to Hyperion and wait for a matching response
// A response matches, when command(+subcommand) of request and response is the same
// command:    The string command
// subcommand: The optional string subcommand
// data:       The json data as Object
// tan:        The optional tan, default 1. If the tan is -1, we skip global response error handling
// Returns data of response or false if timeout
async function sendAsyncToHyperion(command, subcommand, data, tan = Math.floor(Math.random() * 1000)) {
  let obj = { command, tan }
  if (subcommand) { Object.assign(obj, { subcommand }) }
  if (data) { Object.assign(obj, data) }

  return __sendAsync(obj)
}

// Send a json message to Hyperion and wait for a matching response
// A response matches, when command(+subcommand) of request and response is the same
// Returns data of response or false if timeout
async function __sendAsync(data) {
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
        console.error("[window.websocket::onmessage] ", error)
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
function requestRequiresAdminAuth() {
  sendToHyperion("authorize", "adminRequired");
}
// Test if the default password needs to be changed
function requestRequiresDefaultPasswortChange() {
  sendToHyperion("authorize", "newPasswordRequired");
}

// Change password
function requestChangePassword(password, newPassword) {
  sendToHyperion("authorize", "newPassword", { password, newPassword });
}

function requestAuthorization(password) {
  sendToHyperion("authorize", "login", { password });
}

function requestTokenAuthorization(token) {
  sendToHyperion("authorize", "login", { token });
}

function requestToken(comment) {
  sendToHyperion("authorize", "createToken", { comment });
}

function requestTokenInfo() {
  sendToHyperion("authorize", "getTokenList", {});
  return Promise.resolve();
}

function requestGetPendingTokenRequests(id, state) {
  sendToHyperion("authorize", "getPendingTokenRequests", {});
}

function requestHandleTokenRequest(id, state) {
  sendToHyperion("authorize", "answerRequest", { id, accept: state });
}

function requestTokenDelete(id) {
  sendToHyperion("authorize", "deleteToken", { id });
}

function requestInstanceRename(instance, name) {
  sendToHyperion("instance", "saveName", { name }, Number(instance));
}

function requestInstanceStartStop(instance, start) {
  if (start)
    sendToHyperion("instance", "startInstance", {}, Number(instance));
  else
    sendToHyperion("instance", "stopInstance", {}, Number(instance));
}

function requestInstanceDelete(instance) {
  sendToHyperion("instance", "deleteInstance", {}, Number(instance));
}

function requestInstanceCreate(name) {
  sendToHyperion("instance", "createInstance", { name });
}

function requestInstanceSwitch(instance) {
  sendToHyperion("instance", "switchTo", {}, Number(instance));
}

function requestServerInfo(instance) {
  let data = {
    subscribe: [
      "components-update",
      "priorities-update",
      "imageToLedMapping-update",
      "adjustment-update",
      "videomode-update",
      "effects-update",
      "settings-update",
      "instance-update",
      "event-update"
    ]
  };

  sendToHyperion("serverinfo", "getInfo", data, Number(instance));
  return Promise.resolve();
}

function requestSysInfo() {
  sendToHyperion("sysinfo");
}

function requestSystemSuspend() {
  sendToHyperion("system", "suspend");
}

function requestSystemResume() {
  sendToHyperion("system", "resume");
}

function requestSystemRestart() {
  sendToHyperion("system", "restart");
}

function requestServerConfigSchema() {
  sendToHyperion("config", "getschema");
}

const requestServerConfig = {
  // Shared logic encapsulated in a helper function
  createFilter(globalTypes = [], instances = [], instanceTypes = []) {
    const filter = {
      configFilter: {
        global: { types: globalTypes }
      },
    };

    // Handle instances: remove "null" if present and add to filter if not empty
    if (instances.length) {
      filter.configFilter.instances = { ids: instances };
    }

    if (instanceTypes.length > 0) {
      filter.configFilter.instances = filter.configFilter.instances || {};
      filter.configFilter.instances.types = instanceTypes;
    }

    return filter;
  },

  // Synchronous function
  sync(globalTypes, instances, instanceTypes) {
    const filter = this.createFilter(globalTypes, instances, instanceTypes);
    sendToHyperion("config", "getconfig", filter);
    return Promise.resolve();
  },

  // Asynchronous function
  async async(globalTypes, instances, instanceTypes) {
    const filter = this.createFilter(globalTypes, instances, instanceTypes);
    return sendAsyncToHyperion("config", "getconfig", filter);
  }
};

function requestServerConfigReload() {
  sendToHyperion("config", "reload");
}

function requestLedColorsStart(instanceId = window.currentHyperionInstance) {
  window.ledStreamActive = true;
  sendToHyperion("ledcolors", "ledstream-start", {} , instanceId);
}

function requestLedColorsStop(instanceId = window.currentHyperionInstance) {
  window.ledStreamActive = false;
  sendToHyperion("ledcolors", "ledstream-stop", {} , instanceId);
}

function requestLedImageStart(instanceId = window.currentHyperionInstance) {
  window.imageStreamActive = true;
  sendToHyperion("ledcolors", "imagestream-start", {} , instanceId);
}

function requestLedImageStop(instanceId = window.currentHyperionInstance) {
  window.imageStreamActive = false;
  sendToHyperion("ledcolors", "imagestream-stop", {} , instanceId);
}

function requestPriorityClear(priority, instanceIds = [window.currentHyperionInstance]) {
  if (typeof priority !== 'number')
    priority = INPUT.FG_PRIORITY;

  $(window.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  sendToHyperion("clear", "", { priority }, instanceIds);
}

function requestClearAll(instanceIds = [window.currentHyperionInstance]) {
  $(window.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  requestPriorityClear(-1, instanceIds)
}

function requestPlayEffect(name, duration, instanceIds = [window.currentHyperionInstance]) {
  $(window.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  const data = {
    effect: { name },
    priority: INPUT.FG_PRIORITY,
    duration: validateDuration(duration),
    origin: INPUT.ORIGIN,
  };
  sendToHyperion("effect", "", data, instanceIds);
}

function requestSetColor(r, g, b, duration, instanceIds = [window.currentHyperionInstance]) {
  $(window.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  const data = {
    color: [r, g, b],
    priority: INPUT.FG_PRIORITY,
    duration: validateDuration(duration),
    origin: INPUT.ORIGIN
  };
  sendToHyperion("color", "", data, instanceIds);
}

function requestSetImage(imagedata, duration, name, instanceIds = [window.currentHyperionInstance]) {
  const data = {
    imagedata,
    priority: INPUT.FG_PRIORITY,
    duration: validateDuration(duration),
    format: "auto",
    origin: INPUT.ORIGIN,
    name
  };
  sendToHyperion("image", "", data, instanceIds);
}

function requestSetComponentState(component, state, instanceIds = [window.currentHyperionInstance]) {
  sendToHyperion("componentstate", "", { componentstate: { component, state } }, instanceIds);
}

function requestSetSource(priority, instanceIds = [window.currentHyperionInstance]) {
  if (priority == "auto")
    sendToHyperion("sourceselect", "", { auto: true }, instanceIds);
  else
    sendToHyperion("sourceselect", "", { priority }, instanceIds);
}

// Function to transform the legacy config into thee new API format
function transformConfig(configInput, instanceId = 0) {
  const globalConfig = {};
  const instanceSettings = {};

  // Populate globalConfig and instanceSettings based on the specified properties
  for (const [key, value] of Object.entries(configInput)) {
    if (window.schema.propertiesTypes.globalProperties.includes(key)) {
      globalConfig[key] = value;
    } else if (window.schema.propertiesTypes.instanceProperties.includes(key)) {
      instanceSettings[key] = value;
    }
  }

  // Initialize the final transformed configuration
  const transformedConfig = {};

  // Add `global` only if it has properties
  if (Object.keys(globalConfig).length > 0) {
    transformedConfig.global = { settings: globalConfig };
  }

  // Add `instance` only if there are instance settings
  if (Object.keys(instanceSettings).length > 0) {
    transformedConfig.instances = [
      {
        id: instanceId,
        settings: instanceSettings
      }
    ];
  }

  return transformedConfig;
}

function requestWriteConfig(singleInstanceConfig, full) {
  let newConfig = "";
  const instance = Number(window.currentHyperionInstance);

  if (full === true) {
    window.serverConfig = singleInstanceConfig;
    newConfig = transformConfig(window.serverConfig, instance);
  }
  else {
    jQuery.each(singleInstanceConfig, function (i, val) {
      window.serverConfig[i] = val;
    });
    newConfig = transformConfig(singleInstanceConfig, instance);
  }

  sendToHyperion("config", "setconfig", { config: newConfig });
}

function requestRestoreConfig(newConfig) {
  sendToHyperion("config", "restoreconfig", { config: newConfig });
}

function requestWriteEffect(name, script, args, imageData) {
  const data = {
    name,
    script,
    args,
    imageData
  };
  sendToHyperion("create-effect", "", data);
}

function requestTestEffect(name, pythonScript, args, imageData, instanceIds = [window.currentHyperionInstance]) {
  const data = {
    effect: { name, args },
    priority: INPUT.FG_PRIORITY,
    origin: INPUT.ORIGIN,
    pythonScript,
    imageData
  };
  sendToHyperion("effect", "", data, instanceIds);
}

function requestDeleteEffect(name) {
  sendToHyperion("delete-effect", "", { name });
}

function requestLoggingStart() {
  window.loggingStreamActive = true;
  sendToHyperion("logging", "start");
}

function requestLoggingStop() {
  window.loggingStreamActive = false;
  sendToHyperion("logging", "stop");
}

function requestMappingType(mappingType, instanceIds = [window.currentHyperionInstance]) {
  sendToHyperion("processing", "", { mappingType }, instanceIds);
}

function requestVideoMode(newMode) {
  sendToHyperion("videomode", "", { videoMode: newMode });
}

function requestAdjustment(type, value, complete, instanceIds = [window.currentHyperionInstance]) {
  if (complete === true)
    sendToHyperion("adjustment", "", { adjustment: type }, useCurrentInstance);
  else
    sendToHyperion("adjustment", "", { adjustment: { [type]: value } }, instanceIds);
}

async function requestLedDeviceDiscovery(ledDeviceType, params) {
  return sendAsyncToHyperion("leddevice", "discover", { ledDeviceType, params });
}
async function requestLedDeviceProperties(ledDeviceType, params) {
  return sendAsyncToHyperion("leddevice", "getProperties", { ledDeviceType, params });
}

function requestLedDeviceIdentification(ledDeviceType, params) {
  return sendAsyncToHyperion("leddevice", "identify", { ledDeviceType, params });
}

async function requestLedDeviceAddAuthorization(ledDeviceType, params) {
  return sendAsyncToHyperion("leddevice", "addAuthorization", { ledDeviceType, params });
}

async function requestInputSourcesDiscovery(sourceType, params) {
  return sendAsyncToHyperion("inputsource", "discover", { sourceType, params });
}

async function requestServiceDiscovery(serviceType, params) {
  return sendAsyncToHyperion("service", "discover", { serviceType, params });
}

