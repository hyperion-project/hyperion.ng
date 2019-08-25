$(document).ready( function() {
	performTranslation();

	var conf_editor_net = null;
	var conf_editor_json = null;
	var conf_editor_proto = null;
	var conf_editor_fbs = null;
	var conf_editor_bobl = null;
	var conf_editor_forw = null;

	if(window.showOptHelp)
	{
		//network
		$('#conf_cont').append(createRow('conf_cont_net'))
		$('#conf_cont_net').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_net_heading_title"), 'editor_container_net', 'btn_submit_net'));
		$('#conf_cont_net').append(createHelpTable(window.schema.network.properties, $.i18n("edt_conf_net_heading_title")));

		//jsonserver
		$('#conf_cont').append(createRow('conf_cont_json'))
		$('#conf_cont_json').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver'));
		$('#conf_cont_json').append(createHelpTable(window.schema.jsonServer.properties, $.i18n("edt_conf_js_heading_title")));

		//flatbufserver
		$('#conf_cont').append(createRow('conf_cont_flatbuf'))
		$('#conf_cont_flatbuf').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver'));
		$('#conf_cont_flatbuf').append(createHelpTable(window.schema.flatbufServer.properties, $.i18n("edt_conf_fbs_heading_title")));

		//protoserver
		$('#conf_cont').append(createRow('conf_cont_proto'))
		$('#conf_cont_proto').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_pbs_heading_title"), 'editor_container_protoserver', 'btn_submit_protoserver'));
		$('#conf_cont_proto').append(createHelpTable(window.schema.protoServer.properties, $.i18n("edt_conf_pbs_heading_title")));

		//boblight
		$('#conf_cont').append(createRow('conf_cont_bobl'))
		$('#conf_cont_bobl').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver'));
		$('#conf_cont_bobl').append(createHelpTable(window.schema.boblightServer.properties, $.i18n("edt_conf_bobls_heading_title")));

		//forwarder
		if(storedAccess != 'default')
		{
			$('#conf_cont').append(createRow('conf_cont_fw'))
			$('#conf_cont_fw').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder'));
			$('#conf_cont_fw').append(createHelpTable(window.schema.forwarder.properties, $.i18n("edt_conf_fw_heading_title")));
		}
	}
	else
	{
		$('#conf_cont').addClass('row');
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_net_heading_title"), 'editor_container_net', 'btn_submit_net'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_pbs_heading_title"), 'editor_container_protoserver', 'btn_submit_protoserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder'));

		$("#conf_cont_tok").removeClass('row');
	}

	// net
	conf_editor_net = createJsonEditor('editor_container_net', {
		network         : window.schema.network
	}, true, true);

	conf_editor_net.on('change',function() {
		conf_editor_net.validate().length ? $('#btn_submit_net').attr('disabled', true) : $('#btn_submit_net').attr('disabled', false);
	});

	$('#btn_submit_net').off().on('click',function() {
		requestWriteConfig(conf_editor_net.getValue());
	});

	//json
	conf_editor_json = createJsonEditor('editor_container_jsonserver', {
		jsonServer         : window.schema.jsonServer
	}, true, true);

	conf_editor_json.on('change',function() {
		conf_editor_json.validate().length ? $('#btn_submit_jsonserver').attr('disabled', true) : $('#btn_submit_jsonserver').attr('disabled', false);
	});

	$('#btn_submit_jsonserver').off().on('click',function() {
		requestWriteConfig(conf_editor_json.getValue());
	});

	//flatbuffer
	conf_editor_fbs = createJsonEditor('editor_container_fbserver', {
		flatbufServer        : window.schema.flatbufServer
	}, true, true);

	conf_editor_fbs.on('change',function() {
		conf_editor_fbs.validate().length ? $('#btn_submit_fbserver').attr('disabled', true) : $('#btn_submit_fbserver').attr('disabled', false);
	});

	$('#btn_submit_fbserver').off().on('click',function() {
		requestWriteConfig(conf_editor_fbs.getValue());
	});

	//protobuffer
	conf_editor_proto = createJsonEditor('editor_container_protoserver', {
		protoServer        : window.schema.protoServer
	}, true, true);

	conf_editor_proto.on('change',function() {
		conf_editor_proto.validate().length ? $('#btn_submit_protoserver').attr('disabled', true) : $('#btn_submit_protoserver').attr('disabled', false);
	});

	$('#btn_submit_protoserver').off().on('click',function() {
		requestWriteConfig(conf_editor_proto.getValue());
	});

	//boblight
	conf_editor_bobl = createJsonEditor('editor_container_boblightserver', {
		boblightServer     : window.schema.boblightServer
	}, true, true);

	conf_editor_bobl.on('change',function() {
		conf_editor_bobl.validate().length ? $('#btn_submit_boblightserver').attr('disabled', true) : $('#btn_submit_boblightserver').attr('disabled', false);
	});

	$('#btn_submit_boblightserver').off().on('click',function() {
		requestWriteConfig(conf_editor_bobl.getValue());
	});

	if(storedAccess != 'default')
	{
		//forwarder
		conf_editor_forw = createJsonEditor('editor_container_forwarder', {
			forwarder          : window.schema.forwarder
		}, true, true);

		conf_editor_forw.on('change',function() {
			conf_editor_forw.validate().length ? $('#btn_submit_forwarder').attr('disabled', true) : $('#btn_submit_forwarder').attr('disabled', false);
		});

		$('#btn_submit_forwarder').off().on('click',function() {
			requestWriteConfig(conf_editor_forw.getValue());
		});
	}

	//create introduction
	if(window.showOptHelp)
	{
		createHint("intro", $.i18n('conf_network_net_intro'), "editor_container_net");
		createHint("intro", $.i18n('conf_network_json_intro'), "editor_container_jsonserver");
		createHint("intro", $.i18n('conf_network_fbs_intro'), "editor_container_fbserver");
		createHint("intro", $.i18n('conf_network_proto_intro'), "editor_container_protoserver");
		createHint("intro", $.i18n('conf_network_bobl_intro'), "editor_container_boblightserver");
		createHint("intro", $.i18n('conf_network_forw_intro'), "editor_container_forwarder");
		createHint("intro", $.i18n('conf_network_tok_intro'), "tok_desc_cont");
	}

	// Token handling
	function buildTokenList()
	{
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
	$('.tkthead').html(createTableRow([$.i18n('conf_network_tok_cidhead'), $.i18n('conf_network_tok_lastuse'), $.i18n('general_btn_delete')], true, true));
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
		if(10-e.currentTarget.value.length >= 1 && 10-e.currentTarget.value.length <= 9)
			$('#tok_chars_needed').html(10-e.currentTarget.value.length + " " + $.i18n('general_chars_needed'))
		else
		$('#tok_chars_needed').html("<br />")
	});
	$(window.hyperion).off("cmd-authorize-createToken").on("cmd-authorize-createToken", function(event) {
		var val = event.response.info;
		showInfoDialog("newToken",$.i18n('conf_network_tok_diaTitle'),$.i18n('conf_network_tok_diaMsg')+'<br><div style="font-weight:bold">'+val.token+'</div>')
		tokenList.push(val)
		buildTokenList()
	});

	//Reorder hardcoded token div after the general token setting div
	$("#conf_cont_tok").insertAfter("#conf_cont_net");

	function checkApiTokenState(state)
	{
		if(state == false)
			$("#conf_cont_tok").attr('style', 'display:none')
		else
			$("#conf_cont_tok").removeAttr('style')
	}

	$('#root_network_apiAuth').change(function () {
		var state = $(this).is(":checked");
		checkApiTokenState(state);
	})

	checkApiTokenState(window.serverConfig.network.apiAuth);
	
	removeOverlay();
});
