$(document).ready( function() {
	var uiLock = false;
	var prevSess = 0;

	loadContentTo("#container_connection_lost","connection_lost");
	loadContentTo("#container_restart","restart");
	initWebSocket();

	$(hyperion).on("cmd-serverinfo",function(event){
		serverInfo = event.response.info;
		$(hyperion).trigger("ready");

		if (serverInfo.hyperion.config_modified)
			$("#hyperion_reload_notify").fadeIn("fast");
		else
			$("#hyperion_reload_notify").fadeOut("fast");

		if (serverInfo.hyperion.off)
			$("#hyperion_disabled_notify").fadeIn("fast");
		else
			$("#hyperion_disabled_notify").fadeOut("fast");

		if (!serverInfo.hyperion.config_writeable)
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

		var sess = serverInfo.hyperion.sessions;
		if (sess.length != prevSess)
		{
			wSess = [];
			prevSess = sess.length;
			for(var i = 0; i<sess.length; i++)
			{
				if(sess[i].type == "_hyperiond-http._tcp.")
				{
					wSess.push(sess[i]);
				}
			}

			if (wSess.length > 1)
				$('#btn_instanceswitch').toggle(true);
			else
				$('#btn_instanceswitch').toggle(false);
		}

	}); // end cmd-serverinfo

	$(hyperion).one("cmd-sysinfo", function(event) {
		requestServerInfo();
		sysInfo = event.response.info;

		currentVersion = sysInfo.hyperion.version;
	});

	$(hyperion).one("cmd-config-getschema", function(event) {
		serverSchema = event.response.result;
		requestServerConfig();

		schema = serverSchema.properties;
	});

	$(hyperion).one("cmd-config-getconfig", function(event) {
		serverConfig = event.response.result;
		requestSysInfo();

		showOptHelp = serverConfig.general.showOptHelp;
	});

	$(hyperion).on("cmd-kodiResponse",function(event){

		if (event.response.hasOwnProperty("error")){
			$('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodidiscon')+'</p><p>'+$.i18n('wiz_cc_kodidisconlink')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">'+$.i18n('wiz_cc_link')+'</p>');
			$('#btn_wiz_cont').attr('disabled', true);
			withKodi = false;
			showInfoDialog("error","connection error", event.response.error);
		}else {
			kodiResponse = 	JSON.parse(event.response.kodiResponse);
			if (kodiResponse.result == "OK") {
				$('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodicon')+'</p>');
				$('#btn_wiz_cont').attr('disabled', false);
				withKodi = true;
			}else{
				showInfoDialog("warning","request to kodi not successfull", JSON.stringify(kodiResponse));
			}
		}
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

	$(".mnava").bind('click.menu', function(e){
		loadContent(e);
		window.scrollTo(0, 0);
	});

});

$(function(){
	var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
	sidebar.delegate('a.inactive','click',function(){
		sidebar.find('.active').toggleClass('active inactive');
		$(this).toggleClass('active inactive');
	});
});
