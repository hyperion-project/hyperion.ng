$(document).ready( function() {
	var conf_editor = null;
	performTranslation();
	
	$('#conf_cont').append(createOptPanel('fa-play-circle-o', $.i18n("conf_kodi_label_title"), 'editor_container', 'btn_submit'));
	if(showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(schema.kodiVideoChecker.properties, $.i18n("conf_kodi_label_title")));
	}
	
	conf_editor = createJsonEditor('editor_container', {
		kodiVideoChecker: schema.kodiVideoChecker
	}, true, true);

	conf_editor.on('change',function() {
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});
	
	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
	
	//create introduction
	if(showOptHelp)
		createHint("intro", $.i18n('conf_kodi_intro'), "editor_container");
	
	removeOverlay();
});

