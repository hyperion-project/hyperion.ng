
// global vars
var webPrio = 1;
var webOrigin = "Web Configuration";
var showOptHelp;
var currentVersion;
var latestVersion;
var serverInfo = {};
var parsedUpdateJSON = {};
var serverSchema = {};
var serverConfig = {};
var schema;
var sysInfo = {};
var jsonPort = 19444;
var websocket = null;
var hyperion = {};
var wsTan = 1;
var ledStreamActive  = false;
var imageStreamActive = false;
var loggingStreamActive = false;
var loggingHandlerInstalled = false;
var watchdog = 0;
var debugMessagesActive = true;
var wSess = [];

function initRestart()
{
	$(hyperion).off();
	requestServerConfigReload();
	watchdog = 10;
	connectionLostDetection('restart');
}

function connectionLostDetection(type)
{
	if ( watchdog > 2 )
	{
		var interval_id = window.setInterval("", 9999); // Get a reference to the last
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
		$.get( "/cgi/cfg_jsonserver", function() {watchdog=0}).fail(function() {watchdog++;});
	}
}

setInterval(connectionLostDetection, 3000);

// init websocket to hyperion and bind socket events to jquery events of $(hyperion) object
function initWebSocket()
{
	if ("WebSocket" in window)
	{
		if (websocket == null)
		{
			$.ajax({ url: "/cgi/cfg_jsonserver" }).done(function(data) {
				jsonPort = data.substr(1);
				websocket = new WebSocket('ws://'+document.location.hostname+data);

				websocket.onopen = function (event) {
					$(hyperion).trigger({type:"open"});

					$(hyperion).on("cmd-serverinfo", function(event) {
						watchdog = 0;
					});
				};

				websocket.onclose = function (event) {
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
					console.log("[websocket::onclose] "+reason)
					$(hyperion).trigger({type:"close", reason:reason});
					watchdog = 10;
					connectionLostDetection();
				};

				websocket.onmessage = function (event) {
					try
					{
						response = JSON.parse(event.data);
						success = response.success;
						cmd = response.command;
						if (success)
						{
							$(hyperion).trigger({type:"cmd-"+cmd, response:response});
						}
						else
						{
							error = response.hasOwnProperty("error")? response.error : "unknown";
							$(hyperion).trigger({type:"error",reason:error});
							console.log("[websocket::onmessage] "+error)
						}
					}
					catch(exception_error)
					{
						$(hyperion).trigger({type:"error",reason:exception_error});
						console.log("[websocket::onmessage] "+exception_error)
					}
				};

				websocket.onerror = function (error) {
					$(hyperion).trigger({type:"error",reason:error});
					console.log("[websocket::onerror] "+error)
				};
			});
		}
	}
	else
	{
		$(hyperion).trigger("error");
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

	websocket.send(encode_utf8('{"command":"'+command+'", "tan":'+wsTan+subcommand+msg+'}'));
}

// -----------------------------------------------------------
// wrapped server commands

// also used for watchdog
function requestServerInfo()
{
	sendToHyperion("serverinfo");
}

function requestSysInfo()
{
	sendToHyperion("sysinfo");
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
	ledStreamActive=true;
	sendToHyperion("ledcolors", "ledstream-start");
}

function requestLedColorsStop()
{
	ledStreamActive=false;
	sendToHyperion("ledcolors", "ledstream-stop");
}

function requestLedImageStart()
{
	imageStreamActive=true;
	sendToHyperion("ledcolors", "imagestream-start");
}

function requestLedImageStop()
{
	imageStreamActive=false;
	sendToHyperion("ledcolors", "imagestream-stop");
}

function requestPriorityClear(prio)
{
	if(typeof prio !== 'number')
		prio = webPrio;

	sendToHyperion("clear", "", '"priority":'+prio+'');
}

function requestClearAll()
{
	sendToHyperion("clearall");
}

function requestPlayEffect(effectName, duration)
{
	sendToHyperion("effect", "", '"effect":{"name":"'+effectName+'"},"priority":'+webPrio+',"duration":'+validateDuration(duration)+',"origin":"'+webOrigin+'"');
}

function requestSetColor(r,g,b,duration)
{
	sendToHyperion("color", "",  '"color":['+r+','+g+','+b+'], "priority":'+webPrio+',"duration":'+validateDuration(duration)+',"origin":"'+webOrigin+'"');
}

function requestSetImage(data,width,height,duration)
{
	sendToHyperion("image", "",  '"imagedata":"'+data+'", "imagewidth":'+width+',"imageheight":'+height+', "priority":'+webPrio+',"duration":'+validateDuration(duration)+'');
}

function requestSetComponentState(comp, state)
{
	state_str = state ? "true" : "false";
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
		serverConfig = config;
	else
	{
		jQuery.each(config, function(i, val) {
			serverConfig[i] = val;
		});
	}

	sendToHyperion("config","setconfig", '"config":'+JSON.stringify(serverConfig));
}

function requestWriteEffect(effectName,effectPy,effectArgs)
{
	var cutArgs = effectArgs.slice(1, -1);
	sendToHyperion("create-effect", "", '"name":"'+effectName+'", "script":"'+effectPy+'", '+cutArgs);
}

function requestTestEffect(effectName,effectPy,effectArgs)
{
	sendToHyperion("effect", "", '"effect":{"name":"'+effectName+'", "args":'+effectArgs+'}, "priority":'+webPrio+', "origin":"'+webOrigin+'", "pythonScript":"'+effectPy+'"');
}

function requestDeleteEffect(effectName)
{
	sendToHyperion("delete-effect", "", '"name":"'+effectName+'"');
}

function requestLoggingStart()
{
	loggingStreamActive=true;
	sendToHyperion("logging", "start");
}

function requestLoggingStop()
{
	loggingStreamActive=false;
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
