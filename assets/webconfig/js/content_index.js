$(document).ready( function() {

	loadContentTo("#container_connection_lost","connection_lost");
	loadContentTo("#container_restart","restart");
	initWebSocket();

	$(window.hyperion).on("cmd-serverinfo",function(event){
		window.serverInfo = event.response.info;
		// comps
		window.comps = event.response.info.components

		$(window.hyperion).trigger("ready");

		window.comps.forEach( function(obj) {
			if (obj.name == "ALL")
			{
				if(obj.enabled)
					$("#hyperion_disabled_notify").fadeOut("fast");
				else
					$("#hyperion_disabled_notify").fadeIn("fast");
			}
		});

		if (window.serverInfo.hyperion.enabled)
			$("#hyperion_disabled_notify").fadeOut("fast");
		else
			$("#hyperion_disabled_notify").fadeIn("fast");

		updateSessions();
	}); // end cmd-serverinfo

	$(window.hyperion).on("cmd-sessions-update", function(event) {
		window.serverInfo.sessions = event.response.data;
		updateSessions();
	});

	$(window.hyperion).on("cmd-sysinfo", function(event) {
		requestServerInfo();
		window.sysInfo = event.response.info;

		window.currentVersion = window.sysInfo.hyperion.version;
	});

	$(window.hyperion).one("cmd-config-getschema", function(event) {
		window.serverSchema = event.response.info;
		requestServerConfig();

		window.schema = window.serverSchema.properties;
	});

	$(window.hyperion).on("cmd-config-getconfig", function(event) {
		window.serverConfig = event.response.info;
		requestSysInfo();

		window.showOptHelp = window.serverConfig.general.showOptHelp;
	});
	
	$(window.hyperion).on("cmd-config-setconfig", function(event) {
        if (event.response.success === true) {
            $('#hyperion_config_write_success_notify').fadeIn().delay(5000).fadeOut();
        }
    });

	$(window.hyperion).on("error",function(event){
		showInfoDialog("error","Error", event.reason);
	});

	$(window.hyperion).on("open",function(event){
		requestServerConfigSchema();
	});

	$(window.hyperion).one("ready", function(event) {
		loadContent();
	});

	$(window.hyperion).on("cmd-adjustment-update", function(event) {
		window.serverInfo.adjustment = event.response.data
	});

	$(window.hyperion).on("cmd-videomode-update", function(event) {
		window.serverInfo.videomode = event.response.data.videomode
	});

	$(window.hyperion).on("cmd-components-update", function(event) {
		let obj = event.response.data

		// notfication in index
		if (obj.name == "ALL")
		{
			if(obj.enabled)
				$("#hyperion_disabled_notify").fadeOut("fast");
			else
				$("#hyperion_disabled_notify").fadeIn("fast");
		}

		window.comps.forEach((entry, index) => {
			if (entry.name === obj.name){
				window.comps[index] = obj;
			}
		});
		// notify the update
		$(window.hyperion).trigger("components-updated");
	});

	$(window.hyperion).on("cmd-effects-update", function(event){
		window.serverInfo.effects = event.response.data.effects
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
