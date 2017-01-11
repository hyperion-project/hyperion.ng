 
var conf_editor_v4l2 = null;
var conf_editor_fg = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	conf_editor_fg = createJsonEditor('editor_container_fg', {
		framegrabber: schema.framegrabber
	}, true, true);

	$('#btn_submit_fg').off().on('click',function() {
		requestWriteConfig(conf_editor_fg.getValue());
	});
	
	conf_editor_v4l2 = createJsonEditor('editor_container_v4l2', {
		grabberV4L2 : schema.grabberV4L2
	}, true, true);

	$('#btn_submit_v4l2').off().on('click',function() {
		requestWriteConfig(conf_editor_v4l2.getValue());
	});

	if(showOptHelp)
	{
		$('#opt_expl_fg').html(createHelpTable(schema.framegrabber.properties, '<i class="fa fa-info-circle fa-fw"></i>'+$.i18n("edt_conf_fg_heading_title")+' '+$.i18n("conf_helptable_expl")));
		$('#opt_expl_v4l2').html(createHelpTable(schema.grabberV4L2.items.properties, '<i class="fa fa-info-circle fa-fw"></i>'+$.i18n("edt_conf_v4l2_heading_title")+' '+$.i18n("conf_helptable_expl")));
	}
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

