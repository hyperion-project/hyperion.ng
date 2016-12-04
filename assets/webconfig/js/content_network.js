
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		jsonServer         : schema.jsonServer,
		protoServer        : schema.protoServer,
		boblightServer     : schema.boblightServer,
		udpListener        : schema.udpListener,
		forwarder          : schema.forwarder,
	}, true);

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

