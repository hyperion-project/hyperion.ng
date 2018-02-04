var kodiwebsocket = null;
var withKodi = false;
var retrycount = 0;

// init kodiwebsocket to hyperion and bind socket events to jquery events of $(hyperion) object
function initKodiWebSocket()
{
	if ("WebSocket" in window) {
		//default port is 9090
		port = 9090;
		kodiaddress = getStorage("kodiAddress");
		kodiaddress = kodiaddress.split(":");
		//if the user specify a port --> use it
		if (kodiaddress.length > 1) {
				port = kodiaddress[1];
		}

		//close connection if still open
		closeKodiWebsoket()

		if (kodiwebsocket == null)
		{
			try {
				kodiwebsocket = new WebSocket('ws://'+ kodiaddress[0] + ':' + port + '/jsonrpc');
			}
			catch(exception_error) {
				$(hyperion).trigger({type:"error",exception_error});
				console.log("[kodiwebsocket::init] "+exception_error);
			}

			kodiwebsocket.onopen = function (event) {
				enableKodi(false)
				console.log("[kodiwebsocket::onopen] "+event.data);
			};

			kodiwebsocket.onclose = function (event) {
				disableKodi()
				//always 1006 here?!
				console.log("[kodiwebsocket::onclose] "+event.code);
			};

			kodiwebsocket.onmessage = function (event) {
				enableKodi(JSON.parse(event.data));
				console.log("[kodiwebsocket::onmessage] "+event.data);
			};

			kodiwebsocket.onerror = function (error) {
				disableKodi();
				showInfoDialog("error", "No connection to Kodi", error.data);
				console.log("[kodiwebsocket::onerror] "+error.data);
			};
		}
	}
	else {
		$(hyperion).trigger("error");
		alert("Websocket is not supported by your browser");
	}
}

function sendMessageToKodi(method, params) {
	timout = setTimeout(function() {
		if (withKodi && kodiwebsocket.readyState === 1) {
			var msg = {
				"jsonrpc": "2.0",
				"method": method,
				"id": method
			};
			if (params) {
				msg.params = JSON.parse(params);
			}
			kodiwebsocket.send(encode_utf8(JSON.stringify(msg)));
			retrycount = 0;
		}
		else {
			sendMessageToKodi(method, params)
			//give up after one second
			if (retrycount > 100){
				retrycount = 0;
				clearTimeout(timout);
			}
			retrycount++;
	  }
	}, 10); // wait 10 milisecond for the connection...
}

function enableKodi(response)
{
	withKodi = true;
	if (response.id == "GUI.ShowNotification" & response.result == "OK") {
		$('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodicon')+'</p>');
		$('#btn_wiz_cont').attr('disabled', false);
	}
}

function disableKodi()
{
	withKodi = false;

	$('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodidiscon')+'</p><p>'+$.i18n('wiz_cc_kodidisconlink')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">'+$.i18n('wiz_cc_link')+'</p>');
	$('#btn_wiz_cont').attr('disabled', true);
}

function closeKodiWebsoket()
{
	withKodi = false;
	if (kodiwebsocket != null) {
		if (kodiwebsocket.readyState !== 3)
			kodiwebsocket.close()

		console.log("close KodiWebSocket")
		kodiwebsocket = null;
	}
	console.log("nothing close KodiWebSocket")
}
