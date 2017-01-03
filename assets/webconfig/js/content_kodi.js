
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		kodiVideoChecker: schema.kodiVideoChecker
	}, true, true);

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
	
	$('#opt_expl').html(createHelpTable(schema.kodiVideoChecker.properties, '<i class="fa fa-play-circle-o fa-fw"></i>'+$.i18n("conf_kodi_label_title")));
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();

});

