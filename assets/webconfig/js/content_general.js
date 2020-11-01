$(document).ready( function() {
	performTranslation();

	var importedConf;
	var confName;
	var conf_editor = null;

	$('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_gen_heading_title"), 'editor_container', 'btn_submit'));
	if(window.showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(window.schema.general.properties, $.i18n("edt_conf_gen_heading_title")));
	}
	else
		$('#conf_imp').appendTo('#conf_cont');

	conf_editor = createJsonEditor('editor_container', {
		general: window.schema.general
	}, true, true);

	conf_editor.on('change',function() {
		conf_editor.validate().length || window.readOnlyMode ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});

	// Instance handling
	function handleInstanceRename(e)
	{
	
		conf_editor.on('change',function() {
		window.readOnlyMode ? $('#btn_cl_save').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
		window.readOnlyMode ? $('#btn_ma_save').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
		});
		
		var inst = e.currentTarget.id.split("_")[1];
		showInfoDialog('renInst', $.i18n('conf_general_inst_renreq_t'), getInstanceNameByIndex(inst));

		$("#id_btn_ok").off().on('click', function(){
			requestInstanceRename(inst, $('#renInst_name').val())
		});

		$('#renInst_name').off().on('input',function(e) {
			(e.currentTarget.value.length >= 5 && e.currentTarget.value != getInstanceNameByIndex(inst)) ? $('#id_btn_ok').attr('disabled', false) : $('#id_btn_ok').attr('disabled', true);
		});
	}

	function handleInstanceStartStop(e)
	{
		var inst = e.currentTarget.id.split("_")[1];
		var start = (e.currentTarget.className == "btn btn-danger")
		requestInstanceStartStop(inst, start)
	}

	function handleInstanceDelete(e)
	{
		var inst = e.currentTarget.id.split("_")[1];
		showInfoDialog('delInst',$.i18n('conf_general_inst_delreq_h'),$.i18n('conf_general_inst_delreq_t',getInstanceNameByIndex(inst)));
		$("#id_btn_yes").off().on('click', function(){
			requestInstanceDelete(inst)
		});
	}

	function buildInstanceList()
	{
		var inst = serverInfo.instance
		$('.itbody').html("");
		for(var key in inst)
		{
			var startBtnColor = inst[key].running ? "success" : "danger";
			var startBtnIcon = inst[key].running ? "stop" : "play";
			var startBtnText = inst[key].running ? $.i18n('general_btn_stop') : $.i18n('general_btn_start');
			var renameBtn = '<button id="instren_'+inst[key].instance+'" type="button" class="btn btn-primary"><i class="fa fa-fw fa-pencil"></i>'+$.i18n('general_btn_rename')+'</button>';
			var startBtn = ""
			var delBtn = "";
			if(inst[key].instance > 0)
			{
				delBtn = '<button id="instdel_'+inst[key].instance+'" type="button" class="btn btn-warning"><i class="fa fa-fw fa-remove"></i>'+$.i18n('general_btn_delete')+'</button>';
				startBtn = '<button id="inst_'+inst[key].instance+'" type="button" class="btn btn-'+startBtnColor+'"><i class="fa fa-fw fa-'+startBtnIcon+'"></i>'+startBtnText+'</button>';
			}
			$('.itbody').append(createTableRow([inst[key].friendly_name, renameBtn, startBtn, delBtn], false, true));
			$('#instren_'+inst[key].instance).off().on('click', handleInstanceRename);
			$('#inst_'+inst[key].instance).off().on('click', handleInstanceStartStop);
			$('#instdel_'+inst[key].instance).off().on('click', handleInstanceDelete);
			
			window.readOnlyMode ? $('#instren_'+inst[key].instance).attr('disabled', true) : $('#btn_submit').attr('disabled', false);
			window.readOnlyMode ? $('#inst_'+inst[key].instance).attr('disabled', true) : $('#btn_submit').attr('disabled', false);
			window.readOnlyMode ? $('#instdel_'+inst[key].instance).attr('disabled', true) : $('#btn_submit').attr('disabled', false);
		}
	}

	createTable('ithead', 'itbody', 'itable');
	$('.ithead').html(createTableRow([$.i18n('conf_general_inst_namehead'), "", $.i18n('conf_general_inst_actionhead'), ""], true, true));
	buildInstanceList();

	$('#inst_name').off().on('input',function(e) {
		(e.currentTarget.value.length >= 5) && !window.readOnlyMode ? $('#btn_create_inst').attr('disabled', false) : $('#btn_create_inst').attr('disabled', true);
		if(5-e.currentTarget.value.length >= 1 && 5-e.currentTarget.value.length <= 4)
			$('#inst_chars_needed').html(5-e.currentTarget.value.length + " " + $.i18n('general_chars_needed'))
		else
		$('#inst_chars_needed').html("<br />")
	});

	$('#btn_create_inst').off().on('click',function(e) {
		requestInstanceCreate($('#inst_name').val());
		$('#inst_name').val("");
		$('#btn_create_inst').attr('disabled', true)
	});

	$(hyperion).off("instance-updated").on("instance-updated", function(event) {
		buildInstanceList()
	});

	//import
	function dis_imp_btn(state)
	{
		state || window.readOnlyMode ? $('#btn_import_conf').attr('disabled', true) : $('#btn_import_conf').attr('disabled', false);
	}

	function readFile(evt)
	{
		var f = evt.target.files[0];

		if (f)
		{
			var r = new FileReader();
			r.onload = function(e)
			{
				var content = e.target.result.replace(/[^:]?\/\/.*/g, ''); //remove Comments

				//check file is json
				var check = isJsonString(content);
				if(check.length != 0)
				{
					showInfoDialog('error', "", $.i18n('infoDialog_import_jsonerror_text', f.name, JSON.stringify(check)));
					dis_imp_btn(true);
				}
				else
				{
					content = JSON.parse(content);
					//check for hyperion json
					if(typeof content.leds === 'undefined' || typeof content.general === 'undefined')
					{
						showInfoDialog('error', "", $.i18n('infoDialog_import_hyperror_text', f.name));
						dis_imp_btn(true);
					}
					else
					{
						dis_imp_btn(false);
						importedConf = content;
						confName = f.name;
					}
				}
			}
			r.readAsText(f);
		}
	}

	$('#btn_import_conf').off().on('click', function(){
		showInfoDialog('import', $.i18n('infoDialog_import_confirm_title'), $.i18n('infoDialog_import_confirm_text', confName));

		$('#id_btn_import').off().on('click', function(){
			requestWriteConfig(importedConf, true);
			setTimeout(initRestart, 100);
		});
	});

	$('#select_import_conf').off().on('change', function(e){
		if (window.File && window.FileReader && window.FileList && window.Blob)
			readFile(e);
		else
			showInfoDialog('error', "", $.i18n('infoDialog_import_comperror_text'));
	});

	//export
	$('#btn_export_conf').off().on('click', function(){
		var name = window.serverConfig.general.name;

		var d = new Date();
		var month = d.getMonth()+1;
		var day = d.getDate();

		var timestamp = d.getFullYear() + '.' +
			(month<10 ? '0' : '') + month + '.' +
			(day<10 ? '0' : '') + day;

		download(JSON.stringify(window.serverConfig, null, "\t"), 'Hyperion-'+window.currentVersion+'-Backup ('+name+') '+timestamp+'.json', "application/json");
	});

	//create introduction
	if(window.showOptHelp)
	{
		createHint("intro", $.i18n('conf_general_intro'), "editor_container");
		createHint("intro", $.i18n('conf_general_tok_desc'), "tok_desc_cont");
		createHint("intro", $.i18n('conf_general_inst_desc'), "inst_desc_cont");
	}

	removeOverlay();
});
