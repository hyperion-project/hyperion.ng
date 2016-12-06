
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		color              : schema.color,
		smoothing          : schema.smoothing,
		blackborderdetector: schema.blackborderdetector,
	}, true);

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

