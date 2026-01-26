//Constants
const INPUT = Object.freeze({
  ORIGIN: "Hyperion Web-UI",
  FG_PRIORITY: 1
});

// global vars (read and write in globalThis object)
globalThis.showOptHelp = true;
globalThis.gitHubReleaseApiUrl = "https://api.github.com/repos/hyperion-project/hyperion.ng/releases";
globalThis.currentChannel = null;
globalThis.currentVersion = null;
globalThis.latestVersion = null;
globalThis.latestStableVersion = null;
globalThis.latestBetaVersion = null;
globalThis.latestAlphaVersion = null;
globalThis.latestRcVersion = null;
globalThis.gitHubVersionList = null;
globalThis.serverInfo = {};
globalThis.serverSchema = {};
globalThis.serverConfig = {};
globalThis.schema = {};
globalThis.sysInfo = {};
globalThis.jsonPort = 8090;
globalThis.websocket = null;
globalThis.hyperion = {};
globalThis.wsTan = 1;
globalThis.ledStreamActive = false;
globalThis.imageStreamActive = false;
globalThis.loggingStreamActive = false;
globalThis.loggingHandlerInstalled = false;
globalThis.watchdog = 0;
globalThis.debugMessagesActive = true;
globalThis.currentHyperionInstance = null;
globalThis.currentHyperionInstanceName = "?";
globalThis.comps = [];
globalThis.defaultPasswordIsSet = null;

let tokenList = [];
globalThis.setTokenList = function(list) {
  globalThis.tokenList = list;
};
globalThis.getTokenList = function() {
  return globalThis.tokenList;
};
globalThis.addToTokenList = function(token) {
  const currentList = globalThis.getTokenList() || [];
  const updatedList = [...currentList, token];
  globalThis.setTokenList(updatedList);
};
globalThis.deleteFromTokenList = function(id) {
  const currentList = globalThis.getTokenList() || [];
  const updatedList = currentList.filter(token => token.id !== id);
  globalThis.setTokenList(updatedList);
};

function initRestart() {
  $(globalThis.hyperion).off();
  globalThis.watchdog = 10;
  connectionLostDetection('restart');
}

function connectionLostDetection(type) {
  if (globalThis.watchdog > 2) {
    const interval_id = globalThis.setInterval(function () { clearInterval(interval_id); }, 9999); // Get a reference to the last
    for (let i = 1; i < interval_id; i++)
      globalThis.clearInterval(i);
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
    $.get("/cgi/cfg_jsonserver", function () { globalThis.watchdog = 0 }).fail(function () { globalThis.watchdog++; });
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
  if ("WebSocket" in globalThis) {
    if (globalThis.websocket == null) {
      globalThis.jsonPort = '';
      if (document.location.port == '' && document.location.protocol == "http:")
        globalThis.jsonPort = '80';
      else if (document.location.port == '' && document.location.protocol == "https:")
        globalThis.jsonPort = '443';
      else
        globalThis.jsonPort = document.location.port;
      globalThis.websocket = (document.location.protocol == "https:") ? new WebSocket('wss://' + document.location.hostname + ":" + globalThis.jsonPort) : new WebSocket('ws://' + document.location.hostname + ":" + globalThis.jsonPort);

      globalThis.websocket.onopen = function (event) {
        $(globalThis.hyperion).trigger({ type: "open" });

        $(globalThis.hyperion).on("cmd-serverinfo", function (event) {
          globalThis.watchdog = 0;
        });
      };

      globalThis.websocket.onerror = (error) => {
          // Note: For security reasons, browsers don't give detailed error info here
          console.error("WebSocket Error detected.");
      };      

      globalThis.websocket.onclose = function (event) {
        // Check if the server sent the specific Policy Violation code (1008)
        if (event.code === 1008) {
          console.error("WebSocket closed due to policy violation (1008). Access forbidden.");
          const interval_id = globalThis.setInterval(function () { clearInterval(interval_id); }, 9999); // Get highest currently active timer ID.
          // Clear all other timers
          for (let i = 1; i < interval_id; i++) {
            globalThis.clearInterval(i);
          }
          $("body").html($("#container_forbidden").html());
          return;
        }

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

        if (event.code !== 1000) {
          console.warn("WebSocket closed: ", sanitizeForLog(reason));
          $(globalThis.hyperion).trigger({ type: "close", reason: reason });
          globalThis.watchdog = 10;
          connectionLostDetection();
        }
      };

      globalThis.websocket.onmessage = function (event) {
        try {
          const response = JSON.parse(event.data);
          const success = response.success;
          const cmd = response.command;
          const tan = response.tan
          if (success || typeof (success) == "undefined") {
            $(globalThis.hyperion).trigger({ type: "cmd-" + cmd, response: response });
          }
          else
            if (tan != -1) {
              // skip tan -1 error handling
              const error = response.hasOwnProperty("error") ? response.error : "unknown";
              if (error == "Service Unavailable") {
                globalThis.location.reload();
              } else {
                const errorData = Array.isArray(response.errorData) ? response.errorData : [];

                // Sanitize provided input
                const logError = error.replace(/\n|\r/g, "");
                const logErrorData = JSON.stringify(errorData).replace(/[\r\n\t]/g, ' ')
                console.error("[globalThis.websocket::onmessage] ", logError, ", Description:", logErrorData);

                $(globalThis.hyperion).trigger({
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
          console.error("[globalThis.websocket::onmessage] ", exception_error);
          showInfoDialog("error", $.i18n('Info_error_general_title'), $.i18n('Info_error_general_text', exception_error.message));
          $(globalThis.hyperion).trigger({
            type: "error",
            reason: {
              message: $.i18n("ws_processing_exception") + ": " + exception_error.message,
              details: [exception_error.stack]
            }
          });
        }
      };

      globalThis.websocket.onerror = function (error) {
        console.error("[globalThis.websocket::onerror] ", error);
        showInfoDialog("error", $.i18n('Info_error_con_lost_title'), $.i18n('Info_error_con_lost_text', "See browser console for details"));
        $(globalThis.hyperion).trigger({
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
    $(globalThis.hyperion).trigger({
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
  globalThis.websocket.send(JSON.stringify(message));
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
        console.error("[globalThis.websocket::onmessage] ", error)
        resolve(false)
      }
      if (rdata.command == cmd && rdata.tan == tan) {
        globalThis.websocket.removeEventListener('message', func)
        resolve(rdata)
      }
    }
    // after 7 sec we resolve false
    setTimeout(() => { globalThis.websocket.removeEventListener('message', func); resolve(false) }, 7000)
    globalThis.websocket.addEventListener('message', func)
    globalThis.websocket.send(JSON.stringify(data) + '\n')
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

function requestServerInfo(instance = null) {
  const subscriptions = [
    "components-update",
    "priorities-update",
    "imageToLedMapping-update",
    "adjustment-update",
    "videomode-update",
    "effects-update",
    "settings-update",
    "instance-update",
    "event-update"
  ];

  const data = { subscribe: subscriptions };
  const targetInstance = instance !== null ? Number(instance) : null;

  sendToHyperion("serverinfo", "getInfo", data, targetInstance);
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
  return Promise.resolve();
}

const requestServerConfig = {
  // Shared logic encapsulated in a helper function
  createFilter(globalTypes = [], instances = [], instanceTypes = []) {
    const filter = {
      configFilter: {
        global: { types: globalTypes }
      },
    };

    if (instances == null) {
      filter.configFilter.instances = null; // Return no instances
    } else if (instances.length > 0) {
      filter.configFilter.instances = { ids: instances }; // Return selected instances
    } else {
      filter.configFilter.instances = instances; // Return all instances
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

function requestLedColorsStart(instanceId = globalThis.currentHyperionInstance) {
  globalThis.ledStreamActive = true;
  sendToHyperion("ledcolors", "ledstream-start", {}, instanceId);
}

function requestLedColorsStop(instanceId = globalThis.currentHyperionInstance) {
  globalThis.ledStreamActive = false;
  sendToHyperion("ledcolors", "ledstream-stop", {}, instanceId);
}

function requestLedImageStart(instanceId = globalThis.currentHyperionInstance) {
  globalThis.imageStreamActive = true;
  sendToHyperion("ledcolors", "imagestream-start", {}, instanceId);
}

function requestLedImageStop(instanceId = globalThis.currentHyperionInstance) {
  globalThis.imageStreamActive = false;
  sendToHyperion("ledcolors", "imagestream-stop", {}, instanceId);
}

function requestPriorityClear(priority, instanceIds = [globalThis.currentHyperionInstance]) {
  if (typeof priority !== 'number')
    priority = INPUT.FG_PRIORITY;

  $(globalThis.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  sendToHyperion("clear", "", { priority }, instanceIds);
}

function requestClearAll(instanceIds = [globalThis.currentHyperionInstance]) {
  $(globalThis.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  requestPriorityClear(-1, instanceIds)
}

function requestPlayEffect(name, duration, instanceIds = [globalThis.currentHyperionInstance]) {
  $(globalThis.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  const data = {
    effect: { name },
    priority: INPUT.FG_PRIORITY,
    duration: validateDuration(duration),
    origin: INPUT.ORIGIN,
  };
  sendToHyperion("effect", "", data, instanceIds);
}

function requestSetColor(r, g, b, duration, instanceIds = [globalThis.currentHyperionInstance]) {
  $(globalThis.hyperion).trigger({ type: "stopBrowerScreenCapture" });
  const data = {
    color: [r, g, b],
    priority: INPUT.FG_PRIORITY,
    duration: validateDuration(duration),
    origin: INPUT.ORIGIN
  };
  sendToHyperion("color", "", data, instanceIds);
}

function requestSetImage(imagedata, duration, name, instanceIds = [globalThis.currentHyperionInstance]) {
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

function requestSetComponentState(component, state, instanceIds = [globalThis.currentHyperionInstance]) {
  sendToHyperion("componentstate", "", { componentstate: { component, state } }, instanceIds);
}

function requestSetSource(priority, instanceIds = [globalThis.currentHyperionInstance]) {
  if (priority == "auto")
    sendToHyperion("sourceselect", "", { auto: true }, instanceIds);
  else
    sendToHyperion("sourceselect", "", { priority }, instanceIds);
}

// Function to transform the legacy config into the new API format for a single instance
function transformConfig(configInput, instanceId = 0 ) {
  const globalConfig = {};
  const instanceSettings = {};

  for (const [key, value] of Object.entries(configInput)) {
    if (globalThis.schema.propertiesTypes.globalProperties.includes(key)) {
      globalConfig[key] = value;
    } else if (globalThis.schema.propertiesTypes.instanceProperties.includes(key)) {
      instanceSettings[key] = value;
    }
  }

  const transformedConfig = {};

  if (Object.keys(globalConfig).length > 0) {
    transformedConfig.global = { settings: globalConfig };
  }

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

// Function to transform a single config for multiple instance IDs and merge into one config object
function transformConfigForInstances(configInput, instanceIds = [0]) {
  const globalConfig = {};
  const instanceSettings = {};

  for (const [key, value] of Object.entries(configInput)) {
    if (globalThis.schema.propertiesTypes.globalProperties.includes(key)) {
      globalConfig[key] = value;
    } else if (globalThis.schema.propertiesTypes.instanceProperties.includes(key)) {
      instanceSettings[key] = value;
    }
  }

  const transformedConfig = {};

  if (Object.keys(globalConfig).length > 0) {
    transformedConfig.global = { settings: globalConfig };
  }

  if (Object.keys(instanceSettings).length > 0 && Array.isArray(instanceIds) && instanceIds.length > 0) {
    transformedConfig.instances = instanceIds.map(id => ({ id, settings: instanceSettings }));
  }

  return transformedConfig;
}

function requestWriteConfig(singleInstanceConfig, full, instances) {
  let newConfig = "";
  const instance = Number(globalThis.currentHyperionInstance);

  if (full === true) {
    globalThis.serverConfig = singleInstanceConfig;
    // If a list of instances is provided, transform for all; otherwise use current instance
    if (Array.isArray(instances) && instances.length > 0) {
      newConfig = transformConfigForInstances(globalThis.serverConfig, instances.map(Number));
    } else {
      newConfig = transformConfig(globalThis.serverConfig, instance);
    }
  }
  else {
    jQuery.each(singleInstanceConfig, function (i, val) {
      globalThis.serverConfig[i] = val;
    });
    // Build config targeting provided instance list or the current instance
    if (Array.isArray(instances) && instances.length > 0) {
      newConfig = transformConfigForInstances(singleInstanceConfig, instances.map(Number));
    } else {
      newConfig = transformConfig(singleInstanceConfig, instance);
    }
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

function requestTestEffect(name, pythonScript, args, imageData, instanceIds = [globalThis.currentHyperionInstance]) {
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
  globalThis.loggingStreamActive = true;
  sendToHyperion("logging", "start");
}

function requestLoggingStop() {
  globalThis.loggingStreamActive = false;
  sendToHyperion("logging", "stop");
}

function requestMappingType(mappingType, instanceIds = [globalThis.currentHyperionInstance]) {
  sendToHyperion("processing", "", { mappingType }, instanceIds);
}

function requestVideoMode(newMode) {
  sendToHyperion("videomode", "", { videoMode: newMode });
}

function requestAdjustment(type, value, complete, instanceIds = [globalThis.currentHyperionInstance]) {
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

function waitForEvent(eventName) {
  return new Promise((resolve) => {
    const handler = function (event) {
      $(globalThis.hyperion).off(eventName, handler);
      resolve(event);
    };
    $(globalThis.hyperion).on(eventName, handler);
  });
}

function waitForEventWithTimeout(eventName, timeout = 5000) {
  return new Promise((resolve, reject) => {
    const handler = (event) => {
      clearTimeout(timer);
      $(globalThis.hyperion).off(eventName, handler);
      resolve(event);
    };

    const timer = setTimeout(() => {
      $(globalThis.hyperion).off(eventName, handler);
      reject(new Error(`Timeout waiting for ${eventName}`));
    }, timeout);

    $(globalThis.hyperion).on(eventName, handler);
  });
}

