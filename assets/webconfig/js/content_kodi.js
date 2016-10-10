
var conf_editor = null;
$(hyperion).one("cmd-config-getschema", function(event) {
	parsedConfSchemaJSON = event.response.result;
	schema = parsedConfSchemaJSON.properties;

	conf_editor = createJsonEditor('editor_container',
		{
			title:'',
			properties: {
				kodiVideoChecker: schema.kodiVideoChecker,
			}
		});

	$('#editor_container .well').css("background-color","white");
	$('#editor_container .well').css("border","none");
	$('#editor_container .well').css("box-shadow","none");
	$('#editor_container .btn').addClass("btn-primary");
	$('#editor_container h3').remove();

});


$(document).ready( function() {
	requestServerConfigSchema();

	document.getElementById('btn_submit').addEventListener('click',function() {
		// Get the value from the editor
		console.log(conf_editor.getValue());
	});
});

