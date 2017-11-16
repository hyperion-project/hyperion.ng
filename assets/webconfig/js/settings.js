var storedAccess;
var storedLang;
var availLang = ['en','de','es','it','cs'];
var availAccess = ['default','advanced','expert'];
//$.i18n.debug = true;

$(document).ready( function() {

	//i18n
	function initTrans(lc){
		if (lc == 'auto')
		{
			$.i18n().load().done(
			function() {
				performTranslation();
			});
		}
		else
		{
			$.i18n().locale = lc;
			$.i18n().load( "i18n", lc ).done(
			function() {
				performTranslation();
			});	
		}
	}
	
	if (storageComp())
	{
		storedLang = getStorage("langcode");
		if (storedLang == null)
		{
			setStorage("langcode", 'auto')
			storedLang = 'auto';
			initTrans(storedLang);
		}
		else
		{
			initTrans(storedLang);
		}
	}
	else
	{
		showInfoDialog('warning', "Can't store settings", "Your browser doesn't support localStorage. You can't save a specific language setting (fallback to 'auto detection') and access level (fallback to 'default'). Some wizards may be hidden. You could still use the webinterface without further issues");
		initTrans('auto');
		storedLang = 'auto';
		storedAccess = "default";
		$('#btn_setlang').attr("disabled", true);
		$('#btn_setaccess').attr("disabled", true);
	}
	
	$('#btn_setlang').off().on('click',function() {
		var newLang;
		showInfoDialog('select', $.i18n('InfoDialog_lang_title'), $.i18n('InfoDialog_lang_text'));
		
		for (var lcx = 0; lcx<availLang.length; lcx++)
		{
			$('#id_select').append(createSelOpt(availLang[lcx], $.i18n('general_speech_'+availLang[lcx])))
		}
		
		if (storedLang != "auto")
			$('#id_select').val(storedLang);

		$('#id_select').off().on('change',function() {
			newLang = $('#id_select').val();
			if (newLang == storedLang)
				$('#id_btn_saveset').attr('disabled', true);
			else
				$('#id_btn_saveset').attr('disabled', false);
		});
		
		$('#id_btn_saveset').off().on('click',function() {		
				setStorage("langcode", newLang);
				reload();
		});
		
		$('#id_select').trigger('change');
	});

	//access
	storedAccess = getStorage("accesslevel");
	if (storedAccess == null)
	{
		setStorage("accesslevel", "default");
		storedAccess = "default";
	}

	$('#btn_setaccess').off().on('click',function() {
		var newAccess;
		showInfoDialog('select', $.i18n('InfoDialog_access_title'), $.i18n('InfoDialog_access_text'));
		
		for (var lcx = 0; lcx<availAccess.length; lcx++)
		{
			$('#id_select').append(createSelOpt(availAccess[lcx], $.i18n('general_access_'+availAccess[lcx])));
		}
		
		$('#id_select').val(storedAccess);
		
		$('#id_select').off().on('change',function() {
			newAccess = $('#id_select').val();
			if (newAccess == storedAccess)
				$('#id_btn_saveset').attr('disabled', true);
			else
				$('#id_btn_saveset').attr('disabled', false);
		});
		
		$('#id_btn_saveset').off().on('click',function() {
			setStorage("accesslevel", newAccess);
			reload();
		});
		
		$('#id_select').trigger('change');
	});
	
	//hide menu elements
	if (storedAccess != 'expert')
		$('#load_webconfig').toggle(false);
	
	
	// instance switcher
	$('#btn_instanceswitch').off().on('click',function() {
		var lsys = sysInfo.system.hostName+':'+serverConfig.webConfig.port;
		showInfoDialog('iswitch', $.i18n('InfoDialog_iswitch_title'), $.i18n('InfoDialog_iswitch_text'));
		
		for (var i = 0; i<wSess.length; i++)
		{
			
			if(lsys != wSess[i].host+':'+wSess[i].port)
				$('#id_select').append(createSelOpt('http://'+wSess[i].address+':'+wSess[i].port, wSess[i].name))
		}

		$('#id_btn_saveset').off().on('click',function() {		
			$("#loading_overlay").addClass("overlay");
			window.location.href = $('#id_select').val()
		});
		
	});
});