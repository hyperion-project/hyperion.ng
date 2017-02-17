$(document).ready( function() {
	performTranslation();
	var oldDelList = [];
	var effectName = "";
	var effects_editor = null;	
	var effectPy = "";
	var testrun;
	
	if(showOptHelp)
		createHintH("intro", $.i18n('effectsconfigurator_label_intro'), "intro_effc");
	
	function updateDelEffectlist(){
		var newDelList = serverInfo.info.effects;
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
	
	function triggerTestEffect() {
		testrun = true;
		var args = effects_editor.getEditor('root.args');
		requestTestEffect(effectName, ":/effects/" + effectPy.slice(1), JSON.stringify(args.getValue()));
	};
	
	
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
			if ($("#btn_cont_test").hasClass("btn-success") && effects_editor.validate().length == 0 && effectName != "")
			{
				triggerTestEffect();
			}
			if( effects_editor.validate().length == 0 && effectName != "")
			{
				$('#btn_start_test').attr('disabled', false);
				$('#btn_write').attr('disabled', false);
			}
			else
			{
				$('#btn_start_test').attr('disabled', true);
				$('#btn_write').attr('disabled', true);
			}
		});
	});
	
	$("#name-input").on('change keyup', function(event) {
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
		requestWriteEffect(effectName,effectPy,JSON.stringify(effects_editor.getValue()));
		$(hyperion).one("cmd-create-effect", function(event) {
			if (event.response.success)
				showInfoDialog('success', "", $.i18n('infoDialog_effconf_created_text', effectName));
		});
		
		if (testrun)
			setTimeout(requestPriorityClear,100);
		
	});

	$('#btn_start_test').off().on('click',function() {
		triggerTestEffect();
	});
	
	$('#btn_stop_test').off().on('click',function() {
		requestPriorityClear();
		testrun = false;
	});
	
	$('#btn_cont_test').off().on('click',function() {
		toggleClass('#btn_cont_test', "btn-success", "btn-danger");
	});
	
	$('#btn_delete').off().on('click',function() {
		var name = $("#effectsdellist").val();
		requestDeleteEffect(name);
		$(hyperion).one("cmd-delete-effect", function(event) {
			if (event.response.success)
				showInfoDialog('success', "", $.i18n('infoDialog_effconf_deleted_text', name));
		});
	});
	
	$('#effectsdellist').off().on('change', function(){
		if ($(this).val() == null) {
            $('#btn_delete').prop('disabled',true);
        } else {
            $('#btn_delete').prop('disabled',false);
        }
	});
	
	//create basic effect list
	var effects = serverSchema.properties.effectSchemas.internal
	for(var idx=0; idx<effects.length; idx++)
		{
			$("#effectslist").append(createSelOpt(effects[idx].schemaContent.script, $.i18n(effects[idx].schemaContent.title)));
		}
	$("#effectslist").trigger("change");
	
	updateDelEffectlist();
	
	//interval update
	$(hyperion).on("cmd-serverinfo",updateDelEffectlist);
	
	removeOverlay();
});