$(document).ready( function() {
	loadContentTo("#container_connection_lost","connection_lost");
	initWebSocket();
	bindNavToContent("#load_dashboard","dashboard",true);
	bindNavToContent("#load_remote","remote",false);
	bindNavToContent("#load_huebridge","huebridge",false);
	bindNavToContent("#load_support","support",false);
	bindNavToContent("#load_confKodi","kodiconf",false);
	bindNavToContent("#load_update","update",false);
	bindNavToContent("#load_confGeneral","generalconf",false);
	bindNavToContent("#load_confLeds","leds",false);
	bindNavToContent("#load_confGrabber","grabber",false);


	//Change all Checkboxes to Switches
	$("[type='checkbox']").bootstrapSwitch();

	$(hyperion).on("cmd-serverinfo",function(event){
		parsedServerInfoJSON = event.response;
		currentVersion = parsedServerInfoJSON.info.hyperion[0].version;
		cleanCurrentVersion = currentVersion.replace(/\./g, '');
		// get active led device
		var leddevice = parsedServerInfoJSON.info.ledDevices.active;
		$('#dash_leddevice').html(leddevice);
		// get host
		var hostname = parsedServerInfoJSON.info.hostname;
		$('#dash_systeminfo').html(hostname+':'+hyperionport);

		var components = parsedServerInfoJSON.info.components;
		components_html = "";
		for ( idx=0; idx<components.length;idx++)
		{
			components_html += '<tr><td>'+(components[idx].title)+'</td><td><i class="fa fa-circle component-'+(components[idx].enabled?"on":"off")+'"></i></td></tr>';
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
				$('#versioninforesult').html('<div lang="en" data-lang-token="dashboard_message_infobox_updatewarning" style="margin:0px;" class="alert alert-warning">A newer version of Hyperion is available!</div>');
			}
			else
			{
				$('#versioninforesult').html('<div  lang="en" data-lang-token="dashboard_message_infobox_updatesuccess" style="margin:0px;" class="alert alert-success">You run the latest version of Hyperion.</div>');
			}
		});
	}); // end cmd-serverinfo

	$(hyperion).one("cmd-config-getschema", function(event) {
		parsedConfSchemaJSON = event.response.result;
	});

	
	$(hyperion).on("error",function(event){
		showErrorDialog("error", event.reason);
	});

	$(hyperion).on("open",function(event){
		requestServerConfigSchema();
		requestServerInfo();
	});

});

$(function(){
	var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
	sidebar.delegate('a.inactive','click',function(){
		sidebar.find('.active').toggleClass('active inactive');
		$(this).toggleClass('active inactive');
	});
});

