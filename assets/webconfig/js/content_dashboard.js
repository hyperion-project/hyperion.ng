$(document).ready( function() {
	performTranslation();

// 	function newsCont(t,e,l)
// 	{
// 		var h = '<div style="padding-left:9px;border-left:6px solid #0088cc;">';
// 		h += '<h4 style="font-weight:bold;font-size:17px">'+t+'</h4>';
// 		h += e;
// 		h += '<a href="'+l+'" class="" target="_blank"><i class="fa fa-fw fa-newspaper-o"></i>'+$.i18n('dashboard_newsbox_readmore')+'</a>';
// 		h += '</div><hr/>';
// 		$('#dash_news').append(h);
// 	}

// 	function createNews(d)
// 	{
// 		for(var i = 0; i<d.length; i++)
// 		{
// 			if(i > 5)
// 				break;
//
// 			var title = d[i].title.rendered;
// 			var excerpt = d[i].excerpt.rendered;
// 			var link = d[i].link+'?pk_campaign=WebUI&pk_kwd=news_'+d[i].slug;
//
// 			newsCont(title,excerpt,link);
// 		}
// 	}

// 	function getNews()
// 	{
// 		var h = '<span style="color:red;font-weight:bold">'+$.i18n('dashboard_newsbox_noconn')+'</span>';
// 		$.ajax({
// 			url: 'https://hyperion-project.org/wp-json/wp/v2/posts?_embed',
// 			dataType: 'json',
// 			type: 'GET',
// 			timeout: 2000
// 		})
// 		.done( function( data, textStatus, jqXHR ) {
// 			if(jqXHR.status == 200)
// 				createNews(data);
// 			else
// 				$('#dash_news').html(h);
// 		})
// 		.fail( function( jqXHR, textStatus ) {
// 				$('#dash_news').html(h);
// 		});
// 	}

//	getNews();


	function updateComponents()
	{
		var inst = serverInfo.instance
		$(".instance").remove();

		for(var key = inst.length; key--;)
		{
			var instances_html = '<div class="col-md-6 col-xxl-3 instance"><div class="panel panel-default">';
			instances_html += '<div class="panel-heading panel-instance"><span>'+inst[key].friendly_name+'</span></div>';
			instances_html += '<div class="panel-body">';

			instances_html += '<table class="table borderless">';
			instances_html += '<thead><tr><th style="vertical-align:middle"><i class="mdi mdi-lightbulb-on fa-fw"></i>';
			instances_html += '<span>'+$.i18n('dashboard_instance_label_status')+'</span></th>';

			const enable_style = inst[key].running ? "checked" : "";
			const inst_btn_id  = inst[key].instance;

			var instBtn = '<span style="display:block;margin:3px">'
					+'<input id="'+inst_btn_id+'"'+enable_style+' type="checkbox"'
					+'data-toggle="toggle" data-size="small" data-onstyle="success" data-on="'+$.i18n('general_btn_on')+'" data-off="'+$.i18n('general_btn_off')+'">'
					+'</span>';

			instances_html += '<th style="width:1px;text-align:right">'+instBtn+'</th></tr></thead></table>';

			instances_html += '<table class="table borderless">';
			instances_html += '<thead><tr><th colspan="3">';
			instances_html += '<i class="fa fa-info-circle fa-fw"></i>';
			instances_html += '<span>'+$.i18n('dashboard_infobox_label_title')+'</span>';
			instances_html += '</th></tr></thead>';
			instances_html += '<tbody><tr><td></td>';
			instances_html += '<td>'+$.i18n('conf_leds_contr_label_contrtype')+'</td>';
			instances_html += '<td>'+window.serverConfig.device.type+'</td>'; //TODO get servcerConfig from right instance before
			instances_html += '</tr><tr><td></td>';
			instances_html += '<td>'+$.i18n('dashboard_infobox_label_ports')+'</td>';
			instances_html += '<td>'+window.serverConfig.flatbufServer.port+' | '+window.serverConfig.protoServer.port+'</td>';
			instances_html += '</tr></tbody></table>';

			instances_html += '<table class="table first_cell_borderless">';
			instances_html += '<thead><tr><th colspan="3">';
			instances_html += '<i class="fa fa-eye fa-fw"></i>';
			instances_html += '<span>'+$.i18n('dashboard_componentbox_label_title')+'</span>';
			instances_html += '</th></tr></thead>';

			var components = window.comps;
			var tab_components = "";
			for (var idx=0; idx<components.length;idx++) {
				if(components[idx].name != "ALL")
				{
					const enable_style = components[idx].enabled ? "checked" : "";
					const comp_btn_id  = "comp_btn_"+components[idx].name;

					var componentBtn = '<input id="'+comp_btn_id+'"'+enable_style+' type="checkbox"'
					+'data-toggle="toggle" data-size="mini" data-onstyle="success" data-on="'+$.i18n('general_btn_on')+'" data-off="'+$.i18n('general_btn_off')+'">';

					tab_components += '<tr><td></td><td>'+$.i18n('general_comp_'+components[idx].name)+'</td><td>'+componentBtn+'</td></tr>';
				}
			}

			instances_html += '<tbody>'+tab_components+'</tbody></table>';
			instances_html += '</div></div></div>';

			$('.instances').prepend(instances_html);
			$('#dash_config_status').html(window.serverConfig.general.name + ' Status');

			$(`#${inst_btn_id}`).bootstrapToggle();
			$(`#${inst_btn_id}`).change(e => {
				requestInstanceStartStop(e.currentTarget.id.split('_').pop(), e.currentTarget.checked);
			});

			for (var idx=0; idx<components.length;idx++) {
				if(components[idx].name != "ALL")
				{
					$("#comp_btn_"+components[idx].name).bootstrapToggle();
					$("#comp_btn_"+components[idx].name).change(e => {
						requestSetComponentState(e.currentTarget.id.split('_').pop(), e.currentTarget.checked);
					});
				}
			}
		}

		

		// //info
		// var hyperion_enabled = true;

		// components.forEach( function(obj) {
			// if (obj.name == "ALL")
			// {
				// hyperion_enabled = obj.enabled
			// }
		// });

		// var instancename = window.currentHyperionInstanceName;

		// $('#dash_statush').html(hyperion_enabled ? '<span style="color:green">'+$.i18n('general_btn_on')+'</span>' : '<span style="color:red">'+$.i18n('general_btn_off')+'</span>');
		// $('#btn_hsc').html(hyperion_enabled ? '<button class="btn btn-sm btn-danger" onClick="requestSetComponentState(\'ALL\',false)">'+$.i18n('dashboard_infobox_label_disableh', instancename)+'</button>' : '<button class="btn btn-sm btn-success" onClick="requestSetComponentState(\'ALL\',true)">'+$.i18n('dashboard_infobox_label_enableh', instancename)+'</button>');
	}

	// add more info
	$('#dash_currv').html(window.currentVersion);
	// $('#dash_instance').html(window.currentHyperionInstanceName);
	$('#dash_ports').html(window.serverConfig.flatbufServer.port+' | '+window.serverConfig.protoServer.port);
	$('#dash_watchedversionbranch').html(window.serverConfig.general.watchedVersionBranch);

	getReleases(function(callback){
		if(callback)
		{
			$('#dash_latev').html(window.latestVersion.tag_name);

			if (semverLite.gt(window.latestVersion.tag_name, window.currentVersion))
				$('#versioninforesult').html('<div class="bs-callout bs-callout-warning" style="margin:0px"><a target="_blank" href="' + window.latestVersion.html_url + '">'+$.i18n('dashboard_infobox_message_updatewarning', window.latestVersion.tag_name) + '</a></div>');
			else
				$('#versioninforesult').html('<div class="bs-callout bs-callout-success" style="margin:0px">'+$.i18n('dashboard_infobox_message_updatesuccess')+'</div>');

			}
	});



/* 	
	//determine platform
	var grabbers = window.serverInfo.grabbers.available;
	var html = "";

	if(grabbers.indexOf('dispmanx') > -1)
		html += 'Raspberry Pi';
	else if(grabbers.indexOf('x11') > -1 || grabbers.indexOf('xcb') > -1)
		html += 'X86';
	else if(grabbers.indexOf('osx')  > -1)
		html += 'OSX';
	else if(grabbers.indexOf('amlogic')  > -1)
		html += 'Amlogic';
	else
		html += 'Framebuffer';

	$('#dash_platform').html(html);
*/


	//interval update
	updateComponents();
	$(window.hyperion).on("components-updated",updateComponents);

	if(window.showOptHelp)
		createHintH("intro", $.i18n('dashboard_label_intro'), "dash_intro");

	removeOverlay();
});
