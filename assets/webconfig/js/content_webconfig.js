 $(document).ready( function() {
	performTranslation();
	
	var conf_editor = null;
	
	$('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_webc_heading_title"), 'editor_container', 'btn_submit'));
	if(showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(schema.webConfig.properties, $.i18n("edt_conf_webc_heading_title")));
	}
	
	conf_editor = createJsonEditor('editor_container', {
		webConfig : schema.webConfig
	}, true, true);

	conf_editor.on('change',function() {
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});	

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});

	if(showOptHelp)
		createHint("intro", $.i18n('conf_webconfig_label_intro'), "editor_container");
	
	removeOverlay();
});

