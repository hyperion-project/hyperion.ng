$(document).ready( function() {

	loadContentTo("#container_connection_lost","connection_lost");
	loadContentTo("#container_restart","restart");
	initWebSocket();

	$(hyperion).on("cmd-serverinfo",function(event){
		serverInfo = event.response.info;
		// comps
		comps = event.response.info.components

		$(hyperion).trigger("ready");

		comps.forEach( function(obj) {
			if (obj.name == "ALL")
			{
				if(obj.enabled)
					$("#hyperion_disabled_notify").fadeOut("fast");
				else
					$("#hyperion_disabled_notify").fadeIn("fast");
			}
		});

		if (serverInfo.hyperion.enabled)
			$("#hyperion_disabled_notify").fadeOut("fast");
		else
			$("#hyperion_disabled_notify").fadeIn("fast");

		updateSessions();
	}); // end cmd-serverinfo

	$(hyperion).on("cmd-sessions-update", function(event) {
		serverInfo.sessions = event.response.data;
		updateSessions();
	});

	$(hyperion).on("cmd-sysinfo", function(event) {
		requestServerInfo();
		sysInfo = event.response.info;

		currentVersion = sysInfo.hyperion.version;
	});

	$(hyperion).on("cmd-config-getschema", function(event) {
		serverSchema = event.response.info;
		requestServerConfig();

		schema = serverSchema.properties;
	});

	$(hyperion).on("cmd-config-getconfig", function(event) {
		serverConfig = event.response.info;
		requestSysInfo();

		showOptHelp = serverConfig.general.showOptHelp;
	});

	$(hyperion).on("error",function(event){
		showInfoDialog("error","Error", event.reason);
	});

	$(hyperion).on("open",function(event){
		requestServerConfigSchema();
	});

	$(hyperion).on("ready", function(event) {
		loadContent();
	});

	$(hyperion).on("cmd-adjustment-update", function(event) {
		serverInfo.adjustment = event.response.data
	});

	$(hyperion).on("cmd-videomode-update", function(event) {
		serverInfo.videomode = event.response.data.videomode
	});

	$(hyperion).on("cmd-components-update", function(event) {
		let obj = event.response.data

		// notfication in index
		if (obj.name == "ALL")
		{
			if(obj.enabled)
				$("#hyperion_disabled_notify").fadeOut("fast");
			else
				$("#hyperion_disabled_notify").fadeIn("fast");
		}

		comps.forEach((entry, index) => {
			if (entry.name === obj.name){
				comps[index] = obj;
			}
		});
		// notify the update
		$(hyperion).trigger("components-updated");
	});

	$(hyperion).on("cmd-effects-update", function(event){
		serverInfo.effects = event.response.data.effects
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
