 
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	
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
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

