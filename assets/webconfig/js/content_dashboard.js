$(document).ready( function() {
	performTranslation();
	
	function newsCont(t,e,l)
	{
		var h = '<div style="padding-left:9px;border-left:6px solid #0088cc;">';
		h += '<h4 style="font-weight:bold;font-size:17px">'+t+'</h4>';
		h += e;
		h += '<a href="'+l+'" class="" target="_blank"><i class="fa fa-fw fa-newspaper-o"></i>'+$.i18n('dashboard_newsbox_readmore')+'</a>';
		h += '</div><hr/>';
		$('#dash_news').append(h);
	}
	
	function createNews(d)
	{
		for(var i = 0; i<d.length; i++)
		{
			if(i > 5)
				break;

			title = d[i].title.rendered;
			excerpt = d[i].excerpt.rendered;
			link = d[i].link+'?pk_campaign=WebUI&pk_kwd=news_'+d[i].slug;
			
			newsCont(title,excerpt,link);
		}
	}
	
	function getNews()
	{
		var h = '<span style="color:red;font-weight:bold">'+$.i18n('dashboard_newsbox_noconn')+'</span>';
		$.ajax({
			url: 'https://hyperion-project.org/wp-json/wp/v2/posts?_embed',
			dataType: 'json',
			type: 'GET',
			timeout: 2000
		})
		.done( function( data, textStatus, jqXHR ) {
			if(jqXHR.status == 200)
				createNews(data);
			else
				$('#dash_news').html(h);
		})
		.fail( function( jqXHR, textStatus ) {
				$('#dash_news').html(h);
		});
	}
	
	//getNews();
	
	function updateComponents()
	{
		var components = serverInfo.components;
		components_html = "";
		for ( idx=0; idx<components.length;idx++)
		{
			components_html += '<tr><td>'+$.i18n('general_comp_'+components[idx].name)+'</td><td><i class="fa fa-circle component-'+(components[idx].enabled?"on":"off")+'"></i></td></tr>';
		}
		$("#tab_components").html(components_html);
		
		//info
		$('#dash_statush').html(serverInfo.hyperion.off? '<span style="color:red">'+$.i18n('general_btn_off')+'</span>':'<span style="color:green">'+$.i18n('general_btn_on')+'</span>');
		$('#btn_hsc').html(serverInfo.hyperion.off? '<button class="btn btn-sm btn-success" onClick="requestSetComponentState(\'ALL\',true)">'+$.i18n('dashboard_infobox_label_enableh')+'</button>' : '<button class="btn btn-sm btn-danger" onClick="requestSetComponentState(\'ALL\',false)">'+$.i18n('dashboard_infobox_label_disableh')+'</button>');
	}
	
	// add more info
	$('#dash_leddevice').html(serverInfo.ledDevices.active);
	$('#dash_currv').html(currentVersion);
	$('#dash_instance').html(serverConfig.general.name);
	$('#dash_ports').html(jsonPort+' | '+serverConfig.protoServer.port);
	
	$.get( "https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/version.json", function( data ) {
		parsedUpdateJSON = JSON.parse(data);
		latestVersion = parsedUpdateJSON[0].versionnr;
		var cleanLatestVersion = latestVersion.replace(/\./g, '');
		var cleanCurrentVersion = currentVersion.replace(/\./g, '');

		$('#dash_latev').html(latestVersion);
		
		if ( cleanCurrentVersion < cleanLatestVersion )
			$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-warning">'+$.i18n('dashboard_infobox_message_updatewarning', latestVersion)+'</div>');
		else
			$('#versioninforesult').html('<div style="margin:0px;" class="alert alert-success">'+$.i18n('dashboard_infobox_message_updatesuccess')+'</div>');
	});
	
	//determine platform
	var grabbers = serverInfo.grabbers.available;
	var html = "";

	if(grabbers.indexOf('dispmanx') > -1)
		html += 'Raspberry Pi';
	else if(grabbers.indexOf('x11') > -1)
		html += 'X86';
	else if(grabbers.indexOf('osx')  > -1)
		html += 'OSX';
	else if(grabbers.indexOf('amlogic')  > -1)
		html += 'Amlogic';
	else
		html += 'Framebuffer';
	
	$('#dash_platform').html(html); 
	
	
	//interval update
	updateComponents();
	$(hyperion).on("cmd-serverinfo",updateComponents);
	
	if(showOptHelp)
		createHintH("intro", $.i18n('dashboard_label_intro'), "dash_intro");
	
	removeOverlay();
});