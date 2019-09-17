var instNameInit = false

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

		// determine button visibility
		var running = window.serverInfo.instance.filter(entry => entry.running);
		if(running.length > 1)
			$('#btn_hypinstanceswitch').toggle(true)
		else
			$('#btn_hypinstanceswitch').toggle(false)
		// update listing at button
		updateHyperionInstanceListing()
		if(!instNameInit)
		{
			window.currentHyperionInstanceName = getInstanceNameByIndex(0);
			instNameInit = true;
		}

		updateSessions();
	}); // end cmd-serverinfo

	$(window.hyperion).on("cmd-sessions-update", function(event) {
		window.serverInfo.sessions = event.response.data;
		updateSessions();
	});

	$(window.hyperion).one("cmd-authorize-getTokenList", function(event) {
		tokenList = event.response.info;
		requestServerInfo();
	});

	$(window.hyperion).on("cmd-sysinfo", function(event) {
		requestServerInfo();
		window.sysInfo = event.response.info;

		window.currentVersion = window.sysInfo.hyperion.version;
		window.currentChannel = window.sysInfo.hyperion.channel;
	});

	$(window.hyperion).one("cmd-config-getschema", function(event) {
		window.serverSchema = event.response.info;
		requestServerConfig();
		requestTokenInfo();

		window.schema = window.serverSchema.properties;
	});

	$(window.hyperion).on("cmd-config-getconfig", function(event) {
		window.serverConfig = event.response.info;
		requestSysInfo();

		window.showOptHelp = window.serverConfig.general.showOptHelp;
	});

	$(window.hyperion).on("cmd-config-setconfig", function(event) {
        if (event.response.success === true) {
			showNotification('success', $.i18n('dashboard_alert_message_confsave_success'), $.i18n('dashboard_alert_message_confsave_success_t'))
        }
    });

	$(window.hyperion).one("cmd-authorize-login", function(event) {
		$("#main-nav").removeAttr('style')
		$("#top-navbar").removeAttr('style')

		if(window.defaultPasswordIsSet === true)
			showNotification('warning',  $.i18n('dashboard_message_default_password'),  $.i18n('dashboard_message_default_password_t'), '<a style="cursor:pointer" onClick="changePassword()"> '+$.i18n('InfoDialog_changePassword_title')+'</a>')
		else
			//if logged on and pw != default show option to lock ui
			$("#btn_lock_ui").removeAttr('style')


		if (event.response.hasOwnProperty('info'))
			setStorage("loginToken", event.response.info.token, true);

		requestServerConfigSchema();
	});

	$(window.hyperion).on("cmd-authorize-newPassword", function(event) {
        if (event.response.success === true){
			showInfoDialog("success",$.i18n('InfoDialog_changePassword_success'));
			// not necessarily true, but better than nothing
			window.defaultPasswordIsSet = false;
		}
    });

	$(window.hyperion).one("cmd-authorize-newPasswordRequired", function(event) {
		var loginToken = getStorage("loginToken", true)

		if (event.response.info.newPasswordRequired == true)
		{
			window.defaultPasswordIsSet = true;

			if(loginToken)
				requestTokenAuthorization(loginToken)
			else
				requestAuthorization('hyperion');
		}
		else
		{
			$("#main-nav").attr('style', 'display:none')
			$("#top-navbar").attr('style', 'display:none')

			if(loginToken)
				requestTokenAuthorization(loginToken)
			else
				loadContentTo("#page-content", "login")

		}
	});

	$(window.hyperion).one("cmd-authorize-adminRequired", function(event) {
		//Check if a admin login is required.
		//If yes: check if default pw is set. If no: go ahead to get server config and render page
		if (event.response.info.adminRequired === true)
			requestRequiresDefaultPasswortChange();
		else
			requestServerConfigSchema();
	});

	$(window.hyperion).on("error",function(event){
		//If we are getting an error "No Authorization" back with a set loginToken we will forward to new Login (Token is expired.
		//e.g.: hyperiond was started new in the meantime)
		if (event.reason == "No Authorization" && getStorage("loginToken", true))
		{
			removeStorage("loginToken", true);
			requestRequiresAdminAuth();
		}
		else
		{
			showInfoDialog("error","Error", event.reason);
		}
	});

	$(window.hyperion).on("open",function(event){
		requestRequiresAdminAuth();
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

	$(window.hyperion).on("cmd-instance-update", function(event) {
		window.serverInfo.instance = event.response.data
		var avail = event.response.data;
		// notify the update
		$(window.hyperion).trigger("instance-updated");

		// if our current instance is no longer available we are at instance 0 again.
		var isInData = false;
		for(var key in avail)
		{
			if(avail[key].instance == currentHyperionInstance && avail[key].running)
			{
				isInData = true;
			}
		}

		if(!isInData)
		{
			//Delete Storage information about the last used but now stopped instance
			if (getStorage('lastSelectedInstance', false))
				removeStorage('lastSelectedInstance', false)

			currentHyperionInstance = 0;
			currentHyperionInstanceName = getInstanceNameByIndex(0);
			requestServerConfig();
			setTimeout(requestServerInfo,100)
			setTimeout(requestTokenInfo,200)
			setTimeout(loadContent,300, undefined, true)
		}

		// determine button visibility
		var running = serverInfo.instance.filter(entry => entry.running);
		if(running.length > 1)
			$('#btn_hypinstanceswitch').toggle(true)
		else
			$('#btn_hypinstanceswitch').toggle(false)

		// update listing for button
		updateHyperionInstanceListing()
	});

	$(window.hyperion).on("cmd-instance-switchTo", function(event){
		requestServerConfig();
		setTimeout(requestServerInfo,200)
		setTimeout(requestTokenInfo,400)
		setTimeout(loadContent,400, undefined, true)
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

// hotfix body padding when bs modals overlap
$(document.body).on('hide.bs.modal,hidden.bs.modal', function () {
    $('body').css('padding-right','0');
});
