$(document).ready( function() {
	performTranslation();
	
	function updateComponents()
	{
		var components = parsedServerInfoJSON.info.components;
		components_html = "";
		for ( idx=0; idx<components.length;idx++)
		{
			components_html += '<tr><td>'+$.i18n('general_comp_'+components[idx].name)+'</td><td><i class="fa fa-circle component-'+(components[idx].enabled?"on":"off")+'"></i></td></tr>';
		}
		$("#tab_components").html(components_html);
	}
	
	// get active led device
	var leddevice = parsedServerInfoJSON.info.ledDevices.active;
	$('#dash_leddevice').html(leddevice);

	// get host
	var hostname = parsedServerInfoJSON.info.hostname;
	$('#dash_systeminfo').html(hostname+':'+jsonPort);
	
	$.get( "https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/version.json", function( data ) {
		parsedUpdateJSON = JSON.parse(data);
		latestVersion = parsedUpdateJSON[0].versionnr;
		var cleanLatestVersion = latestVersion.replace(/\./g, '');
		var cleanCurrentVersion = currentVersion.replace(/\./g, '');

		$('#currentversion').html(currentVersion);
		$('#latestversion').html(latestVersion);
		
		if ( cleanCurrentVersion < cleanLatestVersion )
			$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-warning">'+$.i18n('dashboard_infobox_message_updatewarning', latestVersion)+'</div>');
		else
			$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-success">'+$.i18n('dashboard_infobox_message_updatesuccess')+'</div>');
	});
	
	//interval update
	updateComponents();
	$(hyperion).on("cmd-serverinfo",updateComponents);
	
	if(showOptHelp)
		createHintH("intro", $.i18n('dashboard_label_intro'), "dash_intro");
	
	removeOverlay();
});