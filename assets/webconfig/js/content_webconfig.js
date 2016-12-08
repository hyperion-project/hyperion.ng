 
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		webConfig : schema.webConfig
	}, true);

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

