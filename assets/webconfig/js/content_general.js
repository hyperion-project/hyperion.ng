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
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});

	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});

	// Token handling
	function buildTokenList()
	{
		console.log(tokenList)
		$('.tktbody').html("");
		for(var key in tokenList)
		{
			var lastUse = (tokenList[key].last_use) ? tokenList[key].last_use : "-";
			var btn = '<button id="tok'+tokenList[key].id+'" type="button" class="btn btn-danger">'+$.i18n('general_btn_delete')+'</button>';
			$('.tktbody').append(createTableRow([tokenList[key].comment, lastUse, btn], false, true));
			$('#tok'+tokenList[key].id).off().on('click', handleDeleteToken);
		}
	}

	createTable('tkthead', 'tktbody', 'tktable');
	$('.tkthead').html(createTableRow([$.i18n('conf_general_tok_cidhead'), $.i18n('conf_general_tok_lastuse'), $.i18n('general_btn_delete')], true, true));
	buildTokenList();

	function handleDeleteToken(e)
	{
		var key = e.currentTarget.id.replace("tok","");
		requestTokenDelete(key);
		$('#tok'+key).parent().parent().remove();
		// rm deleted token id
		tokenList = tokenList.filter(function( obj ) {
    		return obj.id !== key;
		});
	}

	$('#btn_create_tok').off().on('click',function() {
		requestToken($('#tok_comment').val())
		$('#tok_comment').val("")
		$('#btn_create_tok').attr('disabled', true)
	});
	$('#tok_comment').off().on('input',function(e) {
		(e.currentTarget.value.length >= 10) ? $('#btn_create_tok').attr('disabled', false) : $('#btn_create_tok').attr('disabled', true);
	});
	$(window.hyperion).off("cmd-authorize-createToken").on("cmd-authorize-createToken", function(event) {
		var val = event.response.info;
		showInfoDialog("newToken",$.i18n('conf_general_tok_diaTitle'),$.i18n('conf_general_tok_diaMsg')+'<br><div style="font-weight:bold">'+val.token+'</div>')
		tokenList.push(val)
		buildTokenList()
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
		createHint("intro", $.i18n('conf_general_intro'), "editor_container");
		createHint("intro", $.i18n('conf_general_tok_desc'), "tok_desc_cont");

	removeOverlay();
});
