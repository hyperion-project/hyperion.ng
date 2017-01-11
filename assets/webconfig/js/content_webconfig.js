 
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		webConfig : schema.webConfig
	}, true, true);

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
	
	if(showOptHelp)
		$('#opt_expl_webconfig').html(createHelpTable(schema.webConfig.properties, '<i class="fa fa-info-circle fa-fw"></i>'+$.i18n("edt_conf_webc_heading_title")+' '+$.i18n("conf_helptable_expl")));
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

