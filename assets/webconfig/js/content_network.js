$(document).ready( function() {
	performTranslation();

	var conf_editor_net = null;
	var conf_editor_json = null;
	var conf_editor_proto = null;
	var conf_editor_fbs = null;
	var conf_editor_bobl = null;
	var conf_editor_udpl = null;
	var conf_editor_forw = null;

	if(showOptHelp)
	{
		//jsonserver
		$('#conf_cont').append(createRow('conf_cont_json'))
		$('#conf_cont_json').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver'));
		$('#conf_cont_json').append(createHelpTable(schema.jsonServer.properties, $.i18n("edt_conf_js_heading_title")));

		//flatbufserver
		$('#conf_cont').append(createRow('conf_cont_flatbuf'))
		$('#conf_cont_flatbuf').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver'));
		$('#conf_cont_flatbuf').append(createHelpTable(schema.flatbufServer.properties, $.i18n("edt_conf_fbs_heading_title")));

		//boblight
		$('#conf_cont').append(createRow('conf_cont_bobl'))
		$('#conf_cont_bobl').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver'));
		$('#conf_cont_bobl').append(createHelpTable(schema.boblightServer.properties, $.i18n("edt_conf_bobls_heading_title")));

		//udplistener
		$('#conf_cont').append(createRow('conf_cont_udpl'))
		$('#conf_cont_udpl').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_udpl_heading_title"), 'editor_container_udplistener', 'btn_submit_udplistener'));
		$('#conf_cont_udpl').append(createHelpTable(schema.udpListener.properties, $.i18n("edt_conf_udpl_heading_title")));

		//forwarder
		if(storedAccess != 'default')
		{
			$('#conf_cont').append(createRow('conf_cont_fw'))
			$('#conf_cont_fw').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder'));
			$('#conf_cont_fw').append(createHelpTable(schema.forwarder.properties, $.i18n("edt_conf_fw_heading_title")));
		}
	}
	else
	{
		$('#conf_cont').addClass('row');
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver'));
		$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_udpl_heading_title"), 'editor_container_udplistener', 'btn_submit_udplistener'));
		if(storedAccess != 'default')
			$('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder'));
	}
	
	//json
	conf_editor_json = createJsonEditor('editor_container_jsonserver', {
		jsonServer         : schema.jsonServer
	}, true, true);

	conf_editor_json.on('change',function() {
		conf_editor_json.validate().length ? $('#btn_submit_jsonserver').attr('disabled', true) : $('#btn_submit_jsonserver').attr('disabled', false);
	});

	$('#btn_submit_jsonserver').off().on('click',function() {
		requestWriteConfig(conf_editor_json.getValue());
	});

	//flatbuffer
	conf_editor_fbs = createJsonEditor('editor_container_fbserver', {
		flatbufServer        : schema.flatbufServer
	}, true, true);

	conf_editor_fbs.on('change',function() {
		conf_editor_fbs.validate().length ? $('#btn_submit_fbserver').attr('disabled', true) : $('#btn_submit_fbserver').attr('disabled', false);
	});

	$('#btn_submit_fbserver').off().on('click',function() {
		requestWriteConfig(conf_editor_fbs.getValue());
	});

	//boblight
	conf_editor_bobl = createJsonEditor('editor_container_boblightserver', {
		boblightServer     : schema.boblightServer
	}, true, true);

	conf_editor_bobl.on('change',function() {
		conf_editor_bobl.validate().length ? $('#btn_submit_boblightserver').attr('disabled', true) : $('#btn_submit_boblightserver').attr('disabled', false);
	});

	$('#btn_submit_boblightserver').off().on('click',function() {
		requestWriteConfig(conf_editor_bobl.getValue());
	});

	//udplistener
	conf_editor_udpl = createJsonEditor('editor_container_udplistener', {
		udpListener        : schema.udpListener
	}, true, true);

	conf_editor_udpl.on('change',function() {
		conf_editor_udpl.validate().length ? $('#btn_submit_udplistener').attr('disabled', true) : $('#btn_submit_udplistener').attr('disabled', false);
	});

	$('#btn_submit_udplistener').off().on('click',function() {
		requestWriteConfig(conf_editor_udpl.getValue());
	});

	if(storedAccess != 'default')
	{
		//forwarder
		conf_editor_forw = createJsonEditor('editor_container_forwarder', {
			forwarder          : schema.forwarder
		}, true, true);

		conf_editor_forw.on('change',function() {
			conf_editor_forw.validate().length ? $('#btn_submit_forwarder').attr('disabled', true) : $('#btn_submit_forwarder').attr('disabled', false);
		});

		$('#btn_submit_forwarder').off().on('click',function() {
			requestWriteConfig(conf_editor_forw.getValue());
		});
	}

	//create introduction
	if(showOptHelp)
	{
		createHint("intro", $.i18n('conf_network_json_intro'), "editor_container_jsonserver");
		createHint("intro", $.i18n('conf_network_fbs_intro'), "editor_container_fbserver");
		createHint("intro", $.i18n('conf_network_bobl_intro'), "editor_container_boblightserver");
		createHint("intro", $.i18n('conf_network_udpl_intro'), "editor_container_udplistener");
		createHint("intro", $.i18n('conf_network_forw_intro'), "editor_container_forwarder");
	}

	removeOverlay();
});
