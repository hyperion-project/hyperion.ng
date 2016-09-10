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
				/*blackborderdetector,
				color,
				effects,
				forwarder,
				initialEffect,
				kodiVideoChecker,
				smoothing,*/
				logger//,
				/*jsonServer,
				protoServer,
				boblightServer,
				udpListener,
				webConfig*/
			}
		}
	});

	//Called everytime a Input Field is changed = No need for save button
	general_conf_editor.off().on('change',function() {
		console.log(JSON.stringify(general_conf_editor.getValue()));
		requestWriteConfig(general_conf_editor.getValue());
	});

	//Alternative Function with submit button to get Values
	$('btn_submit').off().on('click',function() {
		console.log(general_conf_editor.getValue());
	});

	$(hyperion).on("cmd-config-setconfig",function(event){
		parsedServerInfoJSON = event.response;
		console.log(parsedServerInfoJSON);
	});

});

$(document).ready( function() {
	requestServerConfigSchema();
	//$("[type='checkbox']").bootstrapSwitch();
});
