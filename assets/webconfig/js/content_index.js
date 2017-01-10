$(document).ready( function() {
	var uiLock = false;

	$("#main-nav").hide();
	loadContentTo("#container_connection_lost","connection_lost");
	initWebSocket();
	bindNavToContent("#load_dashboard","dashboard",true);
	bindNavToContent("#load_remote","remote",false);
	bindNavToContent("#load_huebridge","huebridge",false);
	bindNavToContent("#load_support","support",false);
	bindNavToContent("#load_update","update",false);
	bindNavToContent("#load_confGeneral","general",false);
	bindNavToContent("#load_confEffects","effects",false);
	bindNavToContent("#load_confKodi","kodiconf",false);
	bindNavToContent("#load_confLeds","leds",false);
	bindNavToContent("#load_confGrabber","grabber",false);
	bindNavToContent("#load_confColors","colors",false);
	bindNavToContent("#load_confNetwork","network",false);
	bindNavToContent("#load_effectsconfig","effects_configurator",false);
	bindNavToContent("#load_logging","logging",false);
	bindNavToContent("#load_webconfig","webconfig",false);

	$(hyperion).on("cmd-serverinfo",function(event){
		showOptHelp = parsedConfJSON.general.showOptHelp;
		parsedServerInfoJSON = event.response;
		currentVersion = parsedServerInfoJSON.info.hyperion[0].version;
		cleanCurrentVersion = currentVersion.replace(/\./g, '');
		
		if (parsedServerInfoJSON.info.hyperion[0].config_modified)
			$("#hyperion_reload_notify").fadeIn("fast");
		else
			$("#hyperion_reload_notify").fadeOut("fast");

		// get active led device
		var leddevice = parsedServerInfoJSON.info.ledDevices.active;
		if ($("#content_dashboard").length > 0)
		{
			$('#dash_leddevice').html(leddevice);
		}

		// get host
		var hostname = parsedServerInfoJSON.info.hostname;
		if ($("#content_dashboard").length > 0)
		{
			$('#dash_systeminfo').html(hostname+':'+hyperionport);
		}


		if ($("#content_dashboard").length > 0)
		{
			var components = parsedServerInfoJSON.info.components;
			components_html = "";
			for ( idx=0; idx<components.length;idx++)
			{
				components_html += '<tr><td>'+$.i18n('general_comp_'+components[idx].name)+'</td><td><i class="fa fa-circle component-'+(components[idx].enabled?"on":"off")+'"></i></td></tr>';
			}
			$("#tab_components").html(components_html);

			$.get( "https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/version.json", function( data ) {
				parsedUpdateJSON = JSON.parse(data);
				latestVersion = parsedUpdateJSON[0].versionnr;
				cleanLatestVersion = latestVersion.replace(/\./g, '');

				$('#currentversion').html(' V'+currentVersion);
				$('#latestversion').html(' V'+latestVersion);

				if ( cleanCurrentVersion < cleanLatestVersion )
				{
					$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-warning">'+$.i18n('dashboard_infobox_message_updatewarning', latestVersion)+'</div>');
				}
				else
				{
					$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-success">'+$.i18n('dashboard_infobox_message_updatesuccess')+'</div>');
				}
			});
		}
		
		if ($("#logmessages").length == 0 && loggingStreamActive)
		{
			requestLoggingStop();
		}

		
		$("#loading_overlay").removeClass("overlay");
		$("#main-nav").show('slide', {direction: 'left'}, 1000);

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
	});

	$(hyperion).one("cmd-config-getconfig", function(event) {
		parsedConfJSON = event.response.result;
		requestServerInfo();
	});

	$(hyperion).on("error",function(event){
		showInfoDialog("error","Error", event.reason);
	});

	$(hyperion).on("open",function(event){
		requestServerConfigSchema();
	});

	$("#btn_hyperion_reload").on("click", function(){
		initRestart();
	});
});

$(function(){
	var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
	sidebar.delegate('a.inactive','click',function(){
		sidebar.find('.active').toggleClass('active inactive');
		$(this).toggleClass('active inactive');
	});
});

