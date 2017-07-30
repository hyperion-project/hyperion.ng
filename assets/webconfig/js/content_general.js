$(document).ready( function() {
	performTranslation();
	
	var importedConf;
	var confName;
	var conf_editor = null;
	
	$('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_gen_heading_title"), 'editor_container', 'btn_submit'));
	if(showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(schema.general.properties, $.i18n("edt_conf_gen_heading_title")));
	}
	else
		$('#conf_imp').appendTo('#conf_cont');
	
	conf_editor = createJsonEditor('editor_container', {
		general: schema.general
	}, true, true);
	
	conf_editor.on('change',function() {
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});
	
	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
	
	//import
	function dis_imp_btn(state)
	{
		state ? $('#btn_import_conf').attr('disabled', true) : $('#btn_import_conf').attr('disabled', false);
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
						//check config revision
						if(content.general.configVersion !== serverConfig.general.configVersion)
						{
							showInfoDialog('error', "", $.i18n('infoDialog_import_reverror_text', f.name, content.general.configVersion, serverConfig.general.configVersion));
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
		var name = serverConfig.general.name;
	
		var d = new Date();
		var month = d.getMonth()+1;
		var day = d.getDate();

		var timestamp = d.getFullYear() + '.' +
			(month<10 ? '0' : '') + month + '.' +
			(day<10 ? '0' : '') + day;
	
		download(JSON.stringify(serverConfig, null, "\t"), 'Hyperion-'+currentVersion+'-Backup ('+name+') '+timestamp+'.json', "application/json");
	});
	
	//create introduction
	if(showOptHelp)
		createHint("intro", $.i18n('conf_general_intro'), "editor_container");
	
	removeOverlay();
});

