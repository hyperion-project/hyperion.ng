$(document).ready( function() {
	var uiLock = false;

	loadContentTo("#container_connection_lost","connection_lost");
	loadContentTo("#container_restart","restart");
	initWebSocket();

	$(hyperion).on("cmd-serverinfo",function(event){
		parsedServerInfoJSON = event.response;
		currentVersion = parsedServerInfoJSON.info.hyperion[0].version;
		$(hyperion).trigger("ready");
		
		if (parsedServerInfoJSON.info.hyperion[0].config_modified)
			$("#hyperion_reload_notify").fadeIn("fast");
		else
			$("#hyperion_reload_notify").fadeOut("fast");

		
		if ($("#logmessages").length == 0 && loggingStreamActive)
		{
			requestLoggingStop();
			loggingStreamActive = false;
		}

		if (!parsedServerInfoJSON.info.hyperion[0].config_writeable)
		{
			showInfoDialog('uilock',$.i18n('InfoDialog_nowrite_title'),$.i18n('InfoDialog_nowrite_text'));
			$('#wrapper').toggle(false);
			uiLock = true;
		}
		else if (uiLock)
		{
			$("#modal_dialog").modal('hide');
			$('#wrapper').toggle(true);
			uiLock = false;
		}

	}); // end cmd-serverinfo

	$(hyperion).one("cmd-config-getschema", function(event) {
		parsedConfSchemaJSON = event.response.result;
		requestServerConfig();
		
		schema = parsedConfSchemaJSON.properties;
	});

	$(hyperion).one("cmd-config-getconfig", function(event) {
		parsedConfJSON = event.response.result;
		requestServerInfo();
		
		showOptHelp = parsedConfJSON.general.showOptHelp;
	});

	$(hyperion).on("error",function(event){
		showInfoDialog("error","Error", event.reason);
	});

	$(hyperion).on("open",function(event){
		requestServerConfigSchema();
	});
	
	$(hyperion).one("ready", function(event) {
		loadContent();
	});
	
	$("#btn_hyperion_reload").on("click", function(){
		initRestart();
	});
	
	$(".mnava").on('click', function(e){
		loadContent(e);
	});

});

$(function(){
	var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
	sidebar.delegate('a.inactive','click',function(){
		sidebar.find('.active').toggleClass('active inactive');
		$(this).toggleClass('active inactive');
	});
});

