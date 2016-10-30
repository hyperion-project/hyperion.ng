$(hyperion).one("cmd-config-getschema", function(event) {
	effects = parsedConfSchemaJSON.properties.effectSchemas.internal
	EffectsHtml = "";
	for(var idx=0; idx<effects.length; idx++)
		{
			EffectsHtml += '<option value="'+effects[idx].schemaContent.script+'">'+effects[idx].schemaContent.title+'</option>';
		}
		$("#effectslist").html(EffectsHtml);
		$("#effectslist").trigger("change");
	});

	effects_editor = null;	
	effectPy = "";
	
	$("#effectslist").off().on("change", function(event) {
		for(var idx=0; idx<effects.length; idx++){
			if (effects[idx].schemaContent.script == this.value){
				effects_editor = createJsonEditor('editor_container', {
				args : effects[idx].schemaContent,
				},false);
			effectPy = ':';
			effectPy += effects[idx].schemaContent.script;
			}
		}

	});
	
	$('#btn_write').off().on('click',function() {
		
		effectName = $('#name-input').val();
		if (effectName == "")
		{
			showInfoDialog('error','INVALID NAME FIELD','Effect name is empty! Please fill in a name and try again.')
		}
		else
		{
			var errors = effects_editor.validate();
			if(errors.length)
			{
				showInfoDialog('error','INVALID VALUES','Please check for red marked inputs and try again.')
			}
			else
			{
				requestWriteEffect(effectName,effectPy,JSON.stringify(effects_editor.getValue()));
				showInfoDialog('success','SUCCESS!','Your effect has been created successfully!')
			}
		}

	});

	$('#btn_test').off().on('click',function() {
		
		effectName = $('#name-input').val();
		if (effectName == "")
		{
			showInfoDialog('error','INVALID NAME FIELD','Effect name is empty! Please fill in a name and try again.')
		}
		else
		{
			var errors = effects_editor.validate();
			if(errors.length)
			{
				showInfoDialog('error','INVALID VALUES','Please check for red marked inputs and try again.')
			}
			else
			{
				var args = effects_editor.getEditor('root.args');
				requestTestEffect(effectName, ":/effects/" + effectPy.slice(1), JSON.stringify(args.getValue()));
				showInfoDialog('success','SUCCESS!','Your effect has been started!')
			}
		}

	});

$(document).ready( function() {
	requestServerConfigSchema();
});
