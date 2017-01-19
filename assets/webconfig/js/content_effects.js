var olddEffects = [];
var editorReady = false;
var effects_editor = null;
var confFgEff = parsedConfJSON.foregroundEffect.effect;
var confBgEff = parsedConfJSON.backgroundEffect.effect;
var foregroundEffect_editor = null;
var backgroundEffect_editor = null;

$(hyperion).one("cmd-config-getschema", function(event) {
	schema = parsedConfSchemaJSON.properties;
	
	if(showOptHelp)
	{
		//effect path
		if(storedAccess != 'default')
		{
			$('#conf_cont').append(createRow('conf_cont_ef'))
			$('#conf_cont_ef').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
			$('#conf_cont_ef').append(createHelpTable(schema.effects.properties, $.i18n("edt_conf_effp_heading_title")));
		}
		
		//foreground effect
		$('#conf_cont').append(createRow('conf_cont_fge'))
		$('#conf_cont_fge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
		$('#conf_cont_fge').append(createHelpTable(schema.foregroundEffect.properties, $.i18n("edt_conf_fge_heading_title")));
		
		//background effect
		$('#conf_cont').append(createRow('conf_cont_bge'))
		$('#conf_cont_bge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
		$('#conf_cont_bge').append(createHelpTable(schema.backgroundEffect.properties, $.i18n("edt_conf_bge_heading_title")));
	}
	else
	{
		if(storedAccess != 'default')	
			$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
		
		$('#conf_cont').addClass('row');
		$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
		$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
	}
	
	if(storedAccess != 'default')
	{
		effects_editor = createJsonEditor('editor_container_effects', {
			effects            : schema.effects
		}, true, true);
		
		effects_editor.on('change',function() {
			effects_editor.validate().length ? $('#btn_submit_effects').attr('disabled', true) : $('#btn_submit_effects').attr('disabled', false);
		});
		
		$('#btn_submit_effects').off().on('click',function() {
			requestWriteConfig(effects_editor.getValue());
		});
	}

	foregroundEffect_editor = createJsonEditor('editor_container_foregroundEffect', {
		foregroundEffect   : schema.foregroundEffect
	}, true, true);
	
	backgroundEffect_editor = createJsonEditor('editor_container_backgroundEffect', {
		backgroundEffect   : schema.backgroundEffect
	}, true, true);
	
	
	foregroundEffect_editor.on('ready',function() {
		editorReady = true;
	});
	
	foregroundEffect_editor.on('change',function() {
		foregroundEffect_editor.validate().length ? $('#btn_submit_foregroundEffect').attr('disabled', true) : $('#btn_submit_foregroundEffect').attr('disabled', false);
		
		var type = foregroundEffect_editor.getEditor('root.foregroundEffect.type');
		if(type.value == "color")
		{
			foregroundEffect_editor.getEditor('root.foregroundEffect.effect').disable();
			foregroundEffect_editor.getEditor('root.foregroundEffect.color').enable();
		}
		else
		{
			foregroundEffect_editor.getEditor('root.foregroundEffect.effect').enable();
			foregroundEffect_editor.getEditor('root.foregroundEffect.color').disable();
		}
	});
	
	backgroundEffect_editor.on('change',function() {
		backgroundEffect_editor.validate().length ? $('#btn_submit_backgroundEffect').attr('disabled', true) : $('#btn_submit_backgroundEffect').attr('disabled', false);
		
		var type = backgroundEffect_editor.getEditor('root.backgroundEffect.type');
		if(type.value == "color")
		{
			backgroundEffect_editor.getEditor('root.backgroundEffect.effect').disable();
			backgroundEffect_editor.getEditor('root.backgroundEffect.color').enable();
		}
		else
		{
			backgroundEffect_editor.getEditor('root.backgroundEffect.effect').enable();
			backgroundEffect_editor.getEditor('root.backgroundEffect.color').disable();
		}
	});
	
	$('#btn_submit_foregroundEffect').off().on('click',function() {
		//requestWriteConfig(foregroundEffect_editor.getValue());
		console.log(foregroundEffect_editor.getValue());
	});
	
	$('#btn_submit_backgroundEffect').off().on('click',function() {
		//requestWriteConfig(backgroundEffect_editor.getValue());
		console.log(backgroundEffect_editor.getValue());
	});

});

function updateEffectlist(event){
	if(editorReady)
	{
		var newEffects = event.response.info.effects;
		if (newEffects.length != olddEffects.length)
		{
			$('#root_foregroundEffect_effect').html('');
			var usrEffArr = [];
			var sysEffArr = [];
		
			for(i = 0; i < newEffects.length; i++)
			{
				var effectName = newEffects[i].name;
				if(!/^\:/.test(newEffects[i].file))
					usrEffArr.push(effectName);
				else
					sysEffArr.push(effectName);
			}
			$('#root_foregroundEffect_effect').append(createSel(usrEffArr, $.i18n('remote_optgroup_usreffets')));
			$('#root_foregroundEffect_effect').append(createSel(sysEffArr, $.i18n('remote_optgroup_syseffets')));
			$('#root_backgroundEffect_effect').html($('#root_foregroundEffect_effect').html());
			olddEffects = newEffects;
			
			$('#root_foregroundEffect_effect').val(confFgEff).change();
			//$('select').trigger('change');
			//var fgeff = foregroundEffect_editor.getEditor('root.foregroundEffect.effect').setValue(confFgEff);
			//console.log(fgeff);
			
			$('#root_backgroundEffect_effect').val(confBgEff).change();
		}
	}
}

$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
	$(hyperion).on("cmd-serverinfo",updateEffectlist);
});
