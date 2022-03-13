 $(document).ready( function() {
	performTranslation();

	var conf_editor = null;

	$('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_webc_heading_title"), 'editor_container', 'btn_submit', 'panel-system'));
	if(window.showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(window.schema.webConfig.properties, $.i18n("edt_conf_webc_heading_title")));
	}

	conf_editor = createJsonEditor('editor_container', {
		webConfig : window.schema.webConfig
	}, true, true);

	conf_editor.on('change',function() {
		conf_editor.validate().length || window.readOnlyMode ? $('#btn_submit').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
	});

	$('#btn_submit').off().on('click',function() {
		// store the last webui port for correct reconnect url (connection_lost.html)
		var val = conf_editor.getValue();
		window.fastReconnect = true;
		window.jsonPort = val.webConfig.port;
		requestWriteConfig(val);
	});

	if(window.showOptHelp)
		createHint("intro", $.i18n('conf_webconfig_label_intro'), "editor_container");

	removeOverlay();
});
