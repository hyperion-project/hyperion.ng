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
	var general_conf_editor = createJsonEditor('editor_container',
		{
			title:'',
			properties: {
				blackborderdetector: schema.blackborderdetector,
				color              : schema.color,
				effects            : schema.effects,
				forwarder          : schema.forwarder,
				initialEffect      : schema.initialEffect,
				kodiVideoChecker   : schema.kodiVideoChecker,
				smoothing          : schema.smoothing,
				logger             : schema.logger,
				jsonServer         : schema.jsonServer,
				protoServer        : schema.protoServer,
				boblightServer     : schema.boblightServer,
				udpListener        : schema.udpListener,
				webConfig          : schema.webConfig
			}
		});

// 	$('#editor_container .well').css("background-color","white");
// 	$('#editor_container .well').css("border","none");
// 	$('#editor_container .well').css("box-shadow","none");
	$('#editor_container .btn').addClass("btn-primary");
	$('#editor_container h3').first().remove();

	//Called everytime a Input Field is changed = No need for save button
// 	general_conf_editor.off().on('change',function() {
// 		console.log(JSON.stringify(general_conf_editor.getValue()));
// 		requestWriteConfig(general_conf_editor.getValue());
// 	});

	//Alternative Function with submit button to get Values
	$('#btn_submit').off().on('click',function() {
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
