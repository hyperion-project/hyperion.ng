$(document).ready( function() {
	
	var storedLang;
	var storedAccess;
	var availLang = ['en','de'];
	var availAccess = ['default','expert']
	//$.i18n.debug = true;
	
	function storageComp(){
		if (typeof(Storage) !== "undefined")
			return true;
		return false;
	}
	
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
	
	//i18n
	if (storageComp())
	{
		storedLang = localStorage.getItem("langcode");
		if (storedLang == null)
		{
			localStorage.langcode = "auto";
			storedLang = 'auto';
			initTrans('auto');
		}
		else
		{
			initTrans(storedLang);
		}
	}
	else
	{
		showInfoDialog('warning', "Can't store settings", "Your browser doesn't support localStorage. You can't save a specific language setting (fallback to 'auto detection') and access level (fallback to 'default'). You could still use the webinterface without further issues");
		initTrans('auto');
		$('#btn_setlang').toggle();
		$('#btn_setaccess').toggle();
	}
	
	$('#btn_setlang').off().on('click',function() {
		$('#modal_select').html('');
		for (var lcx = 0; lcx<availLang.length; lcx++)
		{
			$('#modal_select').append(createSelOpt(availLang[lcx], $.i18n('general_speech_'+availLang[lcx])))
		}
		showInfoDialog('select', $.i18n('InfoDialog_lang_title'), $.i18n('InfoDialog_lang_text'),'btn_savelang');
		
		if (storedLang != "auto")
			$('#modal_select').val(storedLang);

		$('#btn_savelang').off().on('click',function() {
			var newLang = $('#modal_select').val();

			if (newLang != storedLang)
			{				
				initTrans(newLang);
				localStorage.langcode = newLang;
				storedLang = newLang;
			}
		});
	});

	//access
	function updateVisibility(){
		if(storedAccess == 'expert')
			$('#load_webconfig').toggle(true);
		else
			$('#load_webconfig').toggle(false);	
	}
	
	if (storageComp())
	{
		storedAccess = localStorage.getItem("accesslevel");
		if (storedAccess == null)
		{
			localStorage.accesslevel = "default";
			storedAccess = "default";
			updateVisibility();
		}
		else
		{
			updateVisibility();
		}
	}

	$('#btn_setaccess').off().on('click',function() {
		$('#modal_select').html('');
		for (var lcx = 0; lcx<availAccess.length; lcx++)
		{
			$('#modal_select').append(createSelOpt(availAccess[lcx], $.i18n('general_access_'+availAccess[lcx])))
		}
		showInfoDialog('select', $.i18n('InfoDialog_access_title'), $.i18n('InfoDialog_access_text'), 'btn_saveaccess');
		
		$('#modal_select').val(storedAccess);

		$('#btn_saveaccess').off().on('click',function() {
			storedAccess = $('#modal_select').val();
			localStorage.accesslevel = storedAccess;
			updateVisibility();
		});
	});
	
});