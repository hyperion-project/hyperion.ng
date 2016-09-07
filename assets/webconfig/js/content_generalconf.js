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
	blackborderdetector = schema.blackborderdetector;
	color = schema.color;
	effects = schema.effects;
	forwarder = schema.forwarder;
	initialEffect = schema.initialEffect;
	kodiVideoChecker = schema.kodiVideoChecker;
	smoothing = schema.smoothing;
	logger = schema.logger;
	jsonServer = schema.jsonServer;
	protoServer = schema.protoServer;
	boblightServer = schema.boblightServer;
	udpListener = schema.udpListener;
	webConfig = schema.webConfig;

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
				blackborderdetector//,
				/*color,
				effects,
				forwarder,
				initialEffect,
				kodiVideoChecker,
				smoothing,
				logger,
				jsonServer,
				protoServer,
				boblightServer,
				udpListener,
				webConfig*/
			}
		}
	});

	general_conf_editor.on('change',function() {
		/*var name = general_conf_editor.getEditor('root.schema_blackborderdetector');
	  console.log(name.getValue());
*/
	console.log(JSON.stringify(general_conf_editor.getValue()));
		//console.log('schema_blackborderdetector'.substr(7));
		websocket.send('{"command":"config","subcommand":"setconfig","config":"'+JSON.stringify(general_conf_editor.getValue())+'","create":false, "overwrite":false}');
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
