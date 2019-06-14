$(document).ready( function() {
	performTranslation();
	var oldDelList = [];
	var effectName = "";
	var imageData = "";
	var effects_editor = null;
	var effectPy = "";
	var testrun;

	if(window.showOptHelp)
		createHintH("intro", $.i18n('effectsconfigurator_label_intro'), "intro_effc");

	function updateDelEffectlist(){
		var newDelList = window.serverInfo.effects;
		if(newDelList.length != oldDelList.length)
		{
			$('#effectsdellist').html("");
			var usrEffArr = [];
			var sysEffArr = [];
			for(var idx=0; idx<newDelList.length; idx++)
			{
				if(!/^\:/.test(newDelList[idx].file))
					usrEffArr.push('ext_'+newDelList[idx].name+':'+newDelList[idx].name);
				else
					sysEffArr.push('int_'+newDelList[idx].name+':'+newDelList[idx].name);
			}
			$('#effectsdellist').append(createSel(usrEffArr, $.i18n('remote_optgroup_usreffets'), true)).append(createSel(sysEffArr, $.i18n('remote_optgroup_syseffets'), true)).trigger('change');
			oldDelList = newDelList;
		}
	}

	function triggerTestEffect() {
		testrun = true;
		var args = effects_editor.getEditor('root.args');
		requestTestEffect(effectName, ":/effects/" + effectPy.slice(1), JSON.stringify(args.getValue()), imageData);
	};

	// Specify upload handler for image files
	JSONEditor.defaults.options.upload = function(type, file, cbs) {
		var fileReader = new FileReader();

		//check file
		if (!file.type.startsWith('image')) {
			imageData = "";
			cbs.failure('File upload error');
			// TODO clear file dialog.
			showInfoDialog('error', "", $.i18n('infoDialog_writeimage_error_text', file.name));
			return;
		}

		fileReader.onload = function () {
			imageData = this.result.split(',')[1];
			cbs.success(file.name);
		};

		fileReader.readAsDataURL(file);
	};

	$("#effectslist").off().on("change", function(event) {
		if(effects_editor != null)
			effects_editor.destroy();

		for(var idx=0; idx<effects.length; idx++){
			if (effects[idx].schemaContent.script == this.value)
			{
				effects_editor = createJsonEditor('editor_container', {
				args : effects[idx].schemaContent,
				},false, true, false);

				effectPy = ':';
				effectPy += effects[idx].schemaContent.script;
				imageData = "";
				$("#name-input").trigger("change");

				$("#eff_desc").html(createEffHint($.i18n(effects[idx].schemaContent.title),$.i18n(effects[idx].schemaContent.title+'_desc')));
				break;
			}
		}
		effects_editor.on('change',function() {
			if ($("#btn_cont_test").hasClass("btn-success") && effects_editor.validate().length == 0 && effectName != "")
			{
				triggerTestEffect();
			}
			if( effects_editor.validate().length == 0 && effectName != "")
			{
				$('#btn_start_test, #btn_write').attr('disabled', false);
			}
			else
			{
				$('#btn_start_test, #btn_write').attr('disabled', true);
			}
		});
	});

	// disable or enable control elements
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

	// Save Effect
	$('#btn_write').off().on('click',function() {
		requestWriteEffect(effectName,effectPy,JSON.stringify(effects_editor.getValue()),imageData);
		$(window.hyperion).one("cmd-create-effect", function(event) {
			if (event.response.success)
				showInfoDialog('success', "", $.i18n('infoDialog_effconf_created_text', effectName));
		});

		if (testrun)
			setTimeout(requestPriorityClear,100);

	});

	// Start test
	$('#btn_start_test').off().on('click',function() {
		triggerTestEffect();
	});

	// Stop test
	$('#btn_stop_test').off().on('click',function() {
		requestPriorityClear();
		testrun = false;
	});

	// Continuous test
	$('#btn_cont_test').off().on('click',function() {
		toggleClass('#btn_cont_test', "btn-success", "btn-danger");
	});

	// Delete Effect
	$('#btn_delete').off().on('click',function() {
		var name = $("#effectsdellist").val().split("_")[1];
		requestDeleteEffect(name);
		$(window.hyperion).one("cmd-delete-effect", function(event) {
			if (event.response.success)
				showInfoDialog('success', "", $.i18n('infoDialog_effconf_deleted_text', name));
		});
	});

	// disable or enable Delete Effect Button
	$('#effectsdellist').off().on('change', function(){
		$(this).val() == null ? $('#btn_edit, #btn_delete').prop('disabled',true) : "";
		$(this).val().startsWith("int_") ? $('#btn_delete').prop('disabled',true) : $('#btn_delete').prop('disabled',false);
	});

	// Load Effect
	$('#btn_edit').off().on('click', function(){
		var name = $("#effectsdellist").val().replace("ext_","");

		if(name.startsWith("int_"))
		{	name = name.split("_").pop();
			$("#name-input").val("My Modded Effect");
		}
		else
		{
			name = name.split("_").pop();
			$("#name-input").val(name);
		}

		var efx = window.serverInfo.effects;
		for(var i = 0; i<efx.length; i++)
		{
			if(efx[i].name == name)
			{
				var py = efx[i].script.split("/").pop();
				$("#effectslist").val(py).trigger("change");

				for(var key in efx[i].args)
				{
					var ed = effects_editor.getEditor('root.args.'+[key]);
					if(ed)
						ed.setValue(efx[i].args[key]);
				}
				break;
			}
		}
	});

	//create basic effect list
	var effects = window.serverSchema.properties.effectSchemas.internal;
	for(var idx=0; idx<effects.length; idx++)
	{
		$("#effectslist").append(createSelOpt(effects[idx].schemaContent.script, $.i18n(effects[idx].schemaContent.title)));
	}
	$("#effectslist").trigger("change");

	updateDelEffectlist();

	//interval update
	$(window.hyperion).on("cmd-effects-update", function(event){
		window.serverInfo.effects = event.response.data.effects
		updateDelEffectlist();
	});

	removeOverlay();
});
