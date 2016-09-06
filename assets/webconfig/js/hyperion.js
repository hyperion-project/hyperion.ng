
/**
* Enables translation for the form
* with the ID given in "formID"
* Generates token with the given token prefix
* and an underscore followed by the input id
* Example: input id = input_one
* token prefix = tokenprefix
* The translation token would be: "tokenprefix_input_one"
* Default language in "lang" attribute will always be "en"
* @param {String} tokenPrefix
* @param {String} formID

function enableFormTranslation(tokenPrefix, formID) {
var $inputs = $("#" + formID + " label");

$inputs.each(function() {
  console.log("InputID: " + $(this).attr('id'));
  var oldtext = $("label[for='" + $(this).attr('id') + "']").text();
  $("label[for='" + $(this).attr('id') + "']").html('<span lang="en" data-lang-token="' + tokenPrefix + "_" + $(this).attr('id') + '">' + oldtext + '</span>');
});
}
*/
// global vars
var currentVersion;
var cleanCurrentVersion;
var latestVersion;
var cleanLatestVersion;
var parsedServerInfoJSON = {};
var parsedUpdateJSON = {};
var parsedConfSchemaJSON = {};
var parsedConfJSON = {};
var hyperionport = 19444;
var websocket = null;
var hyperion = {};
var wsTan = 1;
var cronId = 0;
var ledStreamActive=false;
var watchdog = true;

// 
function cron()
{
	if ( ! watchdog )
	{
		var interval_id = window.setInterval("", 9999); // Get a reference to the last
		for (var i = 1; i < interval_id; i++)
			window.clearInterval(i);
		$("body").html($("#container_connection_lost").html());
		connectionLostAction();
	}

	requestServerInfo();
	$(hyperion).trigger({type:"cron"});
}

// init websocket to hyperion and bind socket events to jquery events of $(hyperion) object
function initWebSocket()
{
	if ("WebSocket" in window)
	{
		if (websocket == null)
		{
			$.ajax({ url: "/cgi/cfg_jsonserver" }).done(function(data) {
				hyperionport = data.substr(1);
				websocket = new WebSocket('ws://'+document.location.hostname+data);

				websocket.onopen = function (event) {
					$(hyperion).trigger({type:"open"});
					
					$(hyperion).on("cmd-serverinfo", function(event) {
						watchdog = true;
					});
					cronId = window.setInterval(cron,2000);
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
					$(hyperion).trigger({type:"close", reason:reason});
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



// -----------------------------------------------------------
// wrapped server commands

// also used for watchdog
function requestServerInfo() {
	watchdog = false;
	websocket.send('{"command":"serverinfo", "tan":'+wsTan+'}');
}

function requestServerConfigSchema() {
	websocket.send('{"command":"config", "tan":'+wsTan+',"subcommand":"getschema"}');
}

function requestServerConfig() {
	websocket.send('{"command":"config", "tan":'+wsTan+',"subcommand":"getconfig"}');
}

function requestLedColorsStart() {
	websocket.send('{"command":"ledcolors", "tan":'+wsTan+',"subcommand":"ledstream_start"}');
}

function requestLedColorsStop() {
	websocket.send('{"command":"ledcolors", "tan":'+wsTan+',"subcommand":"ledstream_stop"}');
}

function requestPriorityClear() {
	websocket.send('{"command":"clear", "tan":'+wsTan+', "priority":1}');
}

function requestPlayEffect(effectName) {
	websocket.send('{"command":"effect", "tan":'+wsTan+',"effect":{"name":"'+effectName+'"},"priority":1}');
}

function requestSetColor(r,g,b) {
	websocket.send('{"command":"color", "tan":'+wsTan+', "color":['+r+','+g+','+b+'], "priority":1}');
}

function requestSetComponentState(comp, state){
	state_str = state?"true":"false";
	websocket.send('{"command":"componentstate", "tan":'+wsTan+',"componentstate":{"component":"'+comp+'","state":'+state_str+'}}');
	console.log(comp+' state: '+state_str);
}

function requestSetSource( prio )
{
	if ( prio == "auto" )
		websocket.send('{"command":"sourceselect", "tan":'+wsTan+', "auto" : true}');
	else
		websocket.send('{"command":"sourceselect", "tan":'+wsTan+', "priority" : '+prio+'}');
}

