/*
function removeAdvanced(obj,searchStack)
{
	searchStack = [];
	$.each(obj, function(key, val) {
		if ( typeof(val) == 'object' )
		{
			searchStack.push(key);
			if (! removeAdvanced(val,searchStack) )
				searchStack.pop();
		}
		else if ( key == "advanced" && val == true )
		{
			console.log(searchStack);
			return true;
		}
	});
	return false;
}
*/

$(hyperion).one("cmd-config-getschema", function(event) {
	parsedConfSchemaJSON = event.response.result;
	// remove all "advanced" options from schema
	//removeAdvanced(parsedConfSchemaJSON, []); // not working atm
	//console.log(JSON.stringify(parsedConfSchemaJSON));
	schema = parsedConfSchemaJSON.properties;
	schema_blackborderdetector = schema.blackborderdetector;
	schema_color = schema.color;
	schema_effects = schema.effects;
	schema_forwarder = schema.forwarder;
	schema_initialEffect = schema.initialEffect;
	schema_kodiVideoChecker = schema.kodiVideoChecker;
	schema_smoothing = schema.smoothing;
	schema_logger = schema.logger;
	schema_jsonServer = schema.jsonServer;
	schema_protoServer = schema.protoServer;
	schema_boblightServer = schema.boblightServer;
	schema_udpListener = schema.udpListener;
	schema_webConfig = schema.webConfig;

	var element = document.getElementById('editor_holder');
	//JSONEditor.defaults.options.theme = 'bootstrap3';
	
	var general_conf_editor = new JSONEditor(element,{
		theme: 'bootstrap3',
		disable_collapse: 'true',
		form_name_root: 'sa',
		disable_edit_json: 'true',
		disable_properties: 'true',
		no_additional_properties: 'true',
		schema: {
			title:' ',
			properties: {
				schema_blackborderdetector,
				schema_color,
				schema_effects,
				schema_forwarder,
				schema_initialEffect,
				schema_kodiVideoChecker,
				schema_smoothing,
				schema_logger,
				schema_jsonServer,
				schema_protoServer,
				schema_boblightServer,
				schema_udpListener,
				schema_webConfig
			}
		}
	});
});


$(document).ready( function() {
	requestServerConfigSchema();

	document.getElementById('submit').addEventListener('click',function() {
		// Get the value from the editor
		//console.log(general_conf_editor.getValue());
	});
//  $("[type='checkbox']").bootstrapSwitch();
});

