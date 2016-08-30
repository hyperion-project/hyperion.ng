
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
*/
function enableFormTranslation(tokenPrefix, formID) {
var $inputs = $("#" + formID + " label");

$inputs.each(function() {
  console.log("InputID: " + $(this).attr('id'));
  var oldtext = $("label[for='" + $(this).attr('id') + "']").text();
  $("label[for='" + $(this).attr('id') + "']").html('<span lang="en" data-lang-token="' + tokenPrefix + "_" + $(this).attr('id') + '">' + oldtext + '</span>');
});
}

// global vars
var currentVersion;
var cleanCurrentVersion;
var latestVersion;
var cleanLatestVersion;
var parsedServerInfoJSON;
var parsedUpdateJSON;
var hyperionport = 19444;

function button_reloaddata(){
	hyperionport = $("#json_port").val();
	loaddata();
	};

function loaddata() {

		webSocket = new WebSocket('ws://'+document.location.hostname+':'+hyperionport);

		webSocket.onerror = function(event) {
			$('#con_error_modal').modal('show');
		};

		webSocket.onopen = function(event) {
			webSocket.send('{"command":"serverinfo"}');
		};

		webSocket.onmessage = function(response){
			parsedServerInfoJSON = JSON.parse(response.data );
			currentVersion = parsedServerInfoJSON.info.hyperion[0].version;
			cleanCurrentVersion = currentVersion.replace(/\./g, '');
			// get active led device
			var leddevice = parsedServerInfoJSON.info.ledDevices.active;
			$('#dash_leddevice').html(leddevice);
			// get host
			var hostname = parsedServerInfoJSON.info.hostname;
			$('#dash_systeminfo').html(hostname+':'+hyperionport);

			$.get( "https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/version.json", function( data ) {
				parsedUpdateJSON = JSON.parse(data);
				latestVersion = parsedUpdateJSON[0].versionnr;
				cleanLatestVersion = latestVersion.replace(/\./g, '');

				$('#currentversion').html(' V'+currentVersion);
				$('#latestversion').html(' V'+latestVersion);

				if ( cleanCurrentVersion < cleanLatestVersion ) {
					$('#versioninforesult').html('<div lang="en" data-lang-token="dashboard_message_infobox_updatewarning" style="margin:0px;" class="alert alert-warning">A newer version of Hyperion is available!</div>');
					}
				else{
					$('#versioninforesult').html('<div  lang="en" data-lang-token="dashboard_message_infobox_updatesuccess" style="margin:0px;" class="alert alert-success">You run the latest version of Hyperion.</div>');
					}
			});

		};
};
