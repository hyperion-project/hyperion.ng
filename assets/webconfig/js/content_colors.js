
var editor_color = null;
var editor_smoothing = null;
var editor_blackborder = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	
	if(showOptHelp)
	{
		//color
		$('#conf_cont').append(createRow('conf_cont_color'))
		$('#conf_cont_color').append(createOptPanel('fa-photo', $.i18n("edt_conf_color_heading_title"), 'editor_container_color', 'btn_submit_color'));
		$('#conf_cont_color').append(createHelpTable(schema.color.properties, $.i18n("edt_conf_color_heading_title")));
		
		//smoothing
		$('#conf_cont').append(createRow('conf_cont_smoothing'))
		$('#conf_cont_smoothing').append(createOptPanel('fa-photo', $.i18n("edt_conf_smooth_heading_title"), 'editor_container_smoothing', 'btn_submit_smoothing'));
		$('#conf_cont_smoothing').append(createHelpTable(schema.smoothing.properties, $.i18n("edt_conf_smooth_heading_title")));
		
		//blackborder
		$('#conf_cont').append(createRow('conf_cont_blackborder'))
		$('#conf_cont_blackborder').append(createOptPanel('fa-photo', $.i18n("edt_conf_bb_heading_title"), 'editor_container_blackborder', 'btn_submit_blackborder'));
		$('#conf_cont_blackborder').append(createHelpTable(schema.blackborderdetector.properties, $.i18n("edt_conf_bb_heading_title")));
	}
	else
	{
		$('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_color_heading_title"), 'editor_container_color', 'btn_submit_color'));
		$('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_smooth_heading_title"), 'editor_container_smoothing', 'btn_submit_smoothing'));
		$('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_bb_heading_title"), 'editor_container_blackborder', 'btn_submit_blackborder'));
	}
	
	//color
	editor_color = createJsonEditor('editor_container_color', {
		color              : schema.color
	}, true, true);

	editor_color.on('change',function() {
		editor_color.validate().length ? $('#btn_submit_color').attr('disabled', true) : $('#btn_submit_color').attr('disabled', false);
	});
	
	$('#btn_submit_color').off().on('click',function() {
		requestWriteConfig(editor_color.getValue());
	});
	
	//smoothing
	editor_smoothing = createJsonEditor('editor_container_smoothing', {
		smoothing          : schema.smoothing
	}, true, true);

	editor_smoothing.on('change',function() {
		editor_smoothing.validate().length ? $('#btn_submit_smoothing').attr('disabled', true) : $('#btn_submit_smoothing').attr('disabled', false);
	});
	
	$('#btn_submit_smoothing').off().on('click',function() {
		requestWriteConfig(editor_smoothing.getValue());
	});

	//blackborder
	editor_blackborder = createJsonEditor('editor_container_blackborder', {
		blackborderdetector: schema.blackborderdetector
	}, true, true);

	editor_blackborder.on('change',function() {
		editor_blackborder.validate().length ? $('#btn_submit_blackborder').attr('disabled', true) : $('#btn_submit_blackborder').attr('disabled', false);
	});
	
	$('#btn_submit_blackborder').off().on('click',function() {
		requestWriteConfig(editor_blackborder.getValue());
	});
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

