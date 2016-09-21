 
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

var grabber_conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	parsedConfSchemaJSON = event.response.result;
	schema = parsedConfSchemaJSON.properties;

	var element = document.getElementById('editor_container');
	
	grabber_conf_editor = new JSONEditor(element,{
		theme: 'bootstrap3',
		iconlib: "fontawesome4",
		disable_collapse: 'true',
		form_name_root: 'sa',
		disable_edit_json: 'true',
		disable_properties: 'true',
		no_additional_properties: 'true',
		schema: {
			title:'',
			properties: {
				framegrabber: schema.framegrabber,
				grabberV4L2 : schema["grabberV4L2"]
			}
		}
	});
	
	$('#editor_container .well').css("background-color","white");
	$('#editor_container .well').css("border","none");
	$('#editor_container .well').css("box-shadow","none");
	$('#editor_container .btn').addClass("btn-primary");
	$('#editor_container h3').first().remove();

});


$(document).ready( function() {
	requestServerConfigSchema();

	document.getElementById('btn_submit').addEventListener('click',function() {
		// Get the value from the editor
		console.log(grabber_conf_editor.getValue());
	});
//  $("[type='checkbox']").bootstrapSwitch();
});

