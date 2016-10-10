
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		kodiVideoChecker: schema.kodiVideoChecker
	}, true);

	$('#editor_container h3').remove();

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
});


$(document).ready( function() {
	requestServerConfigSchema();
});

