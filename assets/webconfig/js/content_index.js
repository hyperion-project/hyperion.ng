$(document).ready( function() {
	$("#main-nav").hide();
	$("#loading_overlay").addClass("overlay");
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
		parsedServerInfoJSON = event.response;
		currentVersion = parsedServerInfoJSON.info.hyperion[0].version;
		cleanCurrentVersion = currentVersion.replace(/\./g, '');

		// ToDo lock config menu and display appropriate message
		if (! parsedServerInfoJSON.info.hyperion[0].config_writeable)
		{
			console.log("ATTENTION config is not writable");
		}
		
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
		
		if ($("#logmessages").length == 0)
		{
			requestLoggingStop();
		}

		
		$("#loading_overlay").removeClass("overlay");
		$("#main-nav").show('slide', {direction: 'left'}, 1000);

	}); // end cmd-serverinfo

	$(hyperion).one("cmd-config-getschema", function(event) {
		parsedConfSchemaJSON = event.response.result;
		requestServerConfig();
	});

	$(hyperion).one("cmd-config-getconfig", function(event) {
		parsedConfJSON = event.response.result;
		delete parsedConfJSON["general"].configVersion;

		requestServerInfo();
	});

	$(hyperion).on("error",function(event){
		showInfoDialog("error","Error", event.reason);
	});

	$(hyperion).on("open",function(event){
		requestServerConfigSchema();
	});

	$("#btn_hyperion_reload").on("click", function(){
		$(hyperion).off();
		requestServerConfigReload();
		watchdog = 1;
		$("#wrapper").fadeOut("slow");
		cron();
	});
});

$(function(){
	var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
	sidebar.delegate('a.inactive','click',function(){
		sidebar.find('.active').toggleClass('active inactive');
		$(this).toggleClass('active inactive');
	});
});

