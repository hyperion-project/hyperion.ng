
	var oldDelList = [];
	
	function updateDelEffectlist(event){
		var newDelList = event.response.info.effects
		if(newDelList.length != oldDelList.length)
		{
			var EffectHtml = null;
			for(var idx=0; idx<newDelList.length; idx++)
			{
				if(!/^\:/.test(newDelList[idx].file))
				{
					EffectHtml += '<option value="'+newDelList[idx].name+'">'+newDelList[idx].name+'</option>';
				}
			}
			$("#effectsdellist").html(EffectHtml);
			oldDelList = newDelList;
			$('#effectsdellist').trigger('change');
		}
	}

$(hyperion).one("cmd-config-getschema", function(event) {
	effects = parsedConfSchemaJSON.properties.effectSchemas.internal
	EffectsHtml = "";
	for(var idx=0; idx<effects.length; idx++)
		{
			EffectsHtml += '<option value="'+effects[idx].schemaContent.script+'">'+$.i18n(effects[idx].schemaContent.title)+'</option>';
		}
		$("#effectslist").html(EffectsHtml);
		$("#effectslist").trigger("change");
	});
	
	function validateEditor() {
		if(effects_editor.validate().length)
		{
			showInfoDialog('error', $.i18n('infoDialog_effconf_invalidvalue_title'), $.i18n('infoDialog_effconf_invalidvalue_text'));
			return false;
		}
		return true;
	};
	
	function triggerTestEffect() {
		var args = effects_editor.getEditor('root.args');
		requestTestEffect(effectName, ":/effects/" + effectPy.slice(1), JSON.stringify(args.getValue()));
	};
	
	effectName = "";
	effects_editor = null;	
	effectPy = "";
	
	$("#effectslist").off().on("change", function(event) {
		for(var idx=0; idx<effects.length; idx++){
			if (effects[idx].schemaContent.script == this.value){
				effects_editor = createJsonEditor('editor_container', {
				args : effects[idx].schemaContent,
				},false, true);
			effectPy = ':';
			effectPy += effects[idx].schemaContent.script;
			}
			$("#name-input").trigger("change");
		}
		effects_editor.on('change',function() {
			if ($("#btn_cont_test").hasClass("btn-success") && validateEditor())
			{
				triggerTestEffect();
			}
		});
	});
	
	$("#name-input").on('change keydown click focusout', function(event) {
		effectName = $(this).val();
		if ($(this).val() == '') {
            effects_editor.disable();
            $("#eff_footer").children().attr('disabled',true);
        } else {
            effects_editor.enable();
            $("#eff_footer").children().attr('disabled',false);
        }
    });
	
	$('#btn_write').off().on('click',function() {
		if(validateEditor())
		{
			requestWriteEffect(effectName,effectPy,JSON.stringify(effects_editor.getValue()));
			showInfoDialog('success', $.i18n('infoDialog_effconf_created_title'), $.i18n('infoDialog_effconf_created_text', effectName));
		}
	});

	$('#btn_start_test').off().on('click',function() {
		if(validateEditor())
		{
			triggerTestEffect();
		}
	});
	
	$('#btn_stop_test').off().on('click',function() {
		requestPriorityClear();
	});
	
	$('#btn_cont_test').off().on('click',function() {
		toggleClass('#btn_cont_test', "btn-success", "btn-danger");
	});
	
	$('#btn_delete').off().on('click',function() {
		var name = $("#effectsdellist").val();
		requestDeleteEffect(name);
		showInfoDialog('success', $.i18n('infoDialog_effconf_deleted_title'), $.i18n('infoDialog_effconf_deleted_text', name));
	});
	
	$('#effectsdellist').off().on('change', function(){
		if ($(this).val() == null) {
            $('#btn_delete').prop('disabled',true);
        } else {
            $('#btn_delete').prop('disabled',false);
        }
	});
	
$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
	$(hyperion).on("cmd-serverinfo",updateDelEffectlist);
});
