$(document).ready( function() {
	performTranslation();
	var oldEffects = [];
	var effects_editor = null;
	var confFgEff = window.serverConfig.foregroundEffect.effect;
	var confBgEff = window.serverConfig.backgroundEffect.effect;
	var foregroundEffect_editor = null;
	var backgroundEffect_editor = null;

	if(window.showOptHelp)
	{
		//foreground effect
		$('#conf_cont').append(createRow('conf_cont_fge'));
		$('#conf_cont_fge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
		$('#conf_cont_fge').append(createHelpTable(window.schema.foregroundEffect.properties, $.i18n("edt_conf_fge_heading_title")));

		//background effect
		$('#conf_cont').append(createRow('conf_cont_bge'));
		$('#conf_cont_bge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
		$('#conf_cont_bge').append(createHelpTable(window.schema.backgroundEffect.properties, $.i18n("edt_conf_bge_heading_title")));

		//effect path
		if(storedAccess != 'default')
		{
			$('#conf_cont').append(createRow('conf_cont_ef'));
			$('#conf_cont_ef').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
			$('#conf_cont_ef').append(createHelpTable(window.schema.effects.properties, $.i18n("edt_conf_effp_heading_title")));
		}
	}
	else
	{
		$('#conf_cont').addClass('row');
		$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
		$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
		if(storedAccess != 'default')
			$('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
	}

	if(storedAccess != 'default')
	{
		effects_editor = createJsonEditor('editor_container_effects', {
			effects            : window.schema.effects
		}, true, true);

		effects_editor.on('change',function() {
			effects_editor.validate().length ? $('#btn_submit_effects').attr('disabled', true) : $('#btn_submit_effects').attr('disabled', false);
		});

		$('#btn_submit_effects').off().on('click',function() {
			requestWriteConfig(effects_editor.getValue());
		});
	}

	foregroundEffect_editor = createJsonEditor('editor_container_foregroundEffect', {
		foregroundEffect   : window.schema.foregroundEffect
	}, true, true);

	backgroundEffect_editor = createJsonEditor('editor_container_backgroundEffect', {
		backgroundEffect   : window.schema.backgroundEffect
	}, true, true);


	foregroundEffect_editor.on('ready',function() {
		updateEffectlist();
	});

	foregroundEffect_editor.on('change',function() {
		foregroundEffect_editor.validate().length ? $('#btn_submit_foregroundEffect').attr('disabled', true) : $('#btn_submit_foregroundEffect').attr('disabled', false);
	});

	backgroundEffect_editor.on('change',function() {
		backgroundEffect_editor.validate().length ? $('#btn_submit_backgroundEffect').attr('disabled', true) : $('#btn_submit_backgroundEffect').attr('disabled', false);
	});

	$('#btn_submit_foregroundEffect').off().on('click',function() {
		var value = foregroundEffect_editor.getValue();
		if(typeof value.foregroundEffect.effect == 'undefined')
			value.foregroundEffect.effect = window.serverConfig.foregroundEffect.effect;
		requestWriteConfig(value);
	});

	$('#btn_submit_backgroundEffect').off().on('click',function() {
		var value = backgroundEffect_editor.getValue();
		if(typeof value.backgroundEffect.effect == 'undefined')
			value.backgroundEffect.effect = window.serverConfig.backgroundEffect.effect;
		requestWriteConfig(value);
	});

	//create introduction
	if(window.showOptHelp)
	{
		createHint("intro", $.i18n('conf_effect_path_intro'), "editor_container_effects");
		createHint("intro", $.i18n('conf_effect_fgeff_intro'), "editor_container_foregroundEffect");
		createHint("intro", $.i18n('conf_effect_bgeff_intro'), "editor_container_backgroundEffect");
	}

	function updateEffectlist(){
		var newEffects = window.serverInfo.effects;
		if (newEffects.length != oldEffects.length)
		{
			$('#root_foregroundEffect_effect').html('');
			var usrEffArr = [];
			var sysEffArr = [];

			for(var i = 0; i < newEffects.length; i++)
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
			oldEffects = newEffects;

			$('#root_foregroundEffect_effect').val(confFgEff);
			$('#root_backgroundEffect_effect').val(confBgEff);
		}
	}

	//interval update
	$(window.hyperion).on("cmd-effects-update", function(event){
		window.serverInfo.effects = event.response.data.effects
		updateEffectlist();
	});

	removeOverlay();
});
