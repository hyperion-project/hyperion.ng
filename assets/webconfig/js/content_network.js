
var conf_editor_json = null;
var conf_editor_proto = null;
var conf_editor_bobl = null;
var conf_editor_udpl = null;
var conf_editor_forw = null;

$(hyperion).one("cmd-config-getschema", function(event) {
	
	schema = parsedConfSchemaJSON.properties;
	
	conf_editor_json = createJsonEditor('editor_container_jsonserver', {
		jsonServer         : schema.jsonServer
	}, true, true);

	$('#btn_submit_jsonserver').off().on('click',function() {
		requestWriteConfig(conf_editor_json.getValue());
	});
	
	conf_editor_proto = createJsonEditor('editor_container_protoserver', {
		protoServer        : schema.protoServer
	}, true, true);

	$('#btn_submit_protoserver').off().on('click',function() {
		requestWriteConfig(conf_editor_proto.getValue());
	});
	
	conf_editor_bobl = createJsonEditor('editor_container_boblightserver', {
		boblightServer     : schema.boblightServer
	}, true, true);

	$('#btn_submit_boblightserver').off().on('click',function() {
		requestWriteConfig(conf_editor_bobl.getValue());
	});
	
	conf_editor_udpl = createJsonEditor('editor_container_udplistener', {
		udpListener        : schema.udpListener
	}, true, true);

	$('#btn_submit_udplistener').off().on('click',function() {
		requestWriteConfig(conf_editor_udpl.getValue());
	});
	
	conf_editor_forw = createJsonEditor('editor_container_forwarder', {
		forwarder          : schema.forwarder
	}, true, true);

	$('#btn_submit_forwarder').off().on('click',function() {
		requestWriteConfig(conf_editor_forw.getValue());
	});
	
	$('#opt_expl_jsonserver').html(createHelpTable(schema.jsonServer.properties, '<i class="fa fa-sitemap fa-fw"></i>'+$.i18n("edt_conf_js_heading_title")));
	$('#opt_expl_protoserver').html(createHelpTable(schema.protoServer.properties, '<i class="fa fa-sitemap fa-fw"></i>'+$.i18n("edt_conf_ps_heading_title")));
	$('#opt_expl_boblightserver').html(createHelpTable(schema.boblightServer.properties, '<i class="fa fa-sitemap fa-fw"></i>'+$.i18n("edt_conf_bobls_heading_title")));
	$('#opt_expl_udplistener').html(createHelpTable(schema.udpListener.properties, '<i class="fa fa-sitemap fa-fw"></i>'+$.i18n("edt_conf_udpl_heading_title")));
	$('#opt_expl_forwarder').html(createHelpTable(schema.forwarder.properties, '<i class="fa fa-sitemap fa-fw"></i>'+$.i18n("edt_conf_fw_heading_title")));
});


$(document).ready( function() {
	performTranslation();
	requestServerConfigSchema();
});

