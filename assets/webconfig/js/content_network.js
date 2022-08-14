$(document).ready(function () {
  performTranslation();

  var FORWARDER_ENABLED = (jQuery.inArray("forwarder", window.serverInfo.services) !== -1);
  var FLATBUF_SERVER_ENABLED = (jQuery.inArray("flatbuffer", window.serverInfo.services) !== -1);
  var PROTOTBUF_SERVER_ENABLED = (jQuery.inArray("protobuffer", window.serverInfo.services) !== -1);

  var conf_editor_net = null;
  var conf_editor_json = null;
  var conf_editor_proto = null;
  var conf_editor_fbs = null;
  var conf_editor_forw = null;

  // Service properties , 2-dimensional array of [servicetype][id]
  discoveredRemoteServices = {};

  addJsonEditorHostValidation();

  if (window.showOptHelp) {
    //network
    $('#conf_cont').append(createRow('conf_cont_net'));
    $('#conf_cont_net').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_net_heading_title"), 'editor_container_net', 'btn_submit_net', 'panel-system'));
    $('#conf_cont_net').append(createHelpTable(window.schema.network.properties, $.i18n("edt_conf_net_heading_title")));

    //jsonserver
    $('#conf_cont').append(createRow('conf_cont_json'));
    $('#conf_cont_json').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver', 'panel-system'));
    $('#conf_cont_json').append(createHelpTable(window.schema.jsonServer.properties, $.i18n("edt_conf_js_heading_title")));

    //flatbufserver
    if (FLATBUF_SERVER_ENABLED) {
      $('#conf_cont').append(createRow('conf_cont_flatbuf'));
      $('#conf_cont_flatbuf').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver', 'panel-system'));
      $('#conf_cont_flatbuf').append(createHelpTable(window.schema.flatbufServer.properties, $.i18n("edt_conf_fbs_heading_title"), "flatbufServerHelpPanelId"));
    }

    //protoserver
    if (PROTOTBUF_SERVER_ENABLED) {
      $('#conf_cont').append(createRow('conf_cont_proto'));
      $('#conf_cont_proto').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_pbs_heading_title"), 'editor_container_protoserver', 'btn_submit_protoserver', 'panel-system'));
      $('#conf_cont_proto').append(createHelpTable(window.schema.protoServer.properties, $.i18n("edt_conf_pbs_heading_title"), "protoServerHelpPanelId"));
    }

    //forwarder
    if (FORWARDER_ENABLED) {
      if (storedAccess != 'default') {
        $('#conf_cont').append(createRow('conf_cont_fw'));
        $('#conf_cont_fw').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder', 'panel-system'));
        $('#conf_cont_fw').append(createHelpTable(window.schema.forwarder.properties, $.i18n("edt_conf_fw_heading_title"), "forwarderHelpPanelId"));
      }
    }
  }
  else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_net_heading_title"), 'editor_container_net', 'btn_submit_net'));
    $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_js_heading_title"), 'editor_container_jsonserver', 'btn_submit_jsonserver'));
    if (FLATBUF_SERVER_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fbs_heading_title"), 'editor_container_fbserver', 'btn_submit_fbserver'));
    }
    if (PROTOTBUF_SERVER_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_pbs_heading_title"), 'editor_container_protoserver', 'btn_submit_protoserver'));
    }
    if (FORWARDER_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_fw_heading_title"), 'editor_container_forwarder', 'btn_submit_forwarder'));
    }
    $("#conf_cont_tok").removeClass('row');
  }

  // net
  conf_editor_net = createJsonEditor('editor_container_net', {
    network: window.schema.network
  }, true, true);

  conf_editor_net.on('change', function () {
    conf_editor_net.validate().length || window.readOnlyMode ? $('#btn_submit_net').prop('disabled', true) : $('#btn_submit_net').prop('disabled', false);
  });

  $('#btn_submit_net').off().on('click', function () {
    requestWriteConfig(conf_editor_net.getValue());
  });

  //json
  conf_editor_json = createJsonEditor('editor_container_jsonserver', {
    jsonServer: window.schema.jsonServer
  }, true, true);

  conf_editor_json.on('change', function () {
    conf_editor_json.validate().length || window.readOnlyMode ? $('#btn_submit_jsonserver').prop('disabled', true) : $('#btn_submit_jsonserver').prop('disabled', false);
  });

  $('#btn_submit_jsonserver').off().on('click', function () {
    requestWriteConfig(conf_editor_json.getValue());
  });

  //flatbuffer
  if (FLATBUF_SERVER_ENABLED) {
    conf_editor_fbs = createJsonEditor('editor_container_fbserver', {
      flatbufServer: window.schema.flatbufServer
    }, true, true);

    conf_editor_fbs.on('change', function () {
      var flatbufServerEnable = conf_editor_fbs.getEditor("root.flatbufServer.enable").getValue();
      if (flatbufServerEnable) {
        showInputOptionsForKey(conf_editor_fbs, "flatbufServer", "enable", true);
        $('#flatbufServerHelpPanelId').show();
      } else {
        showInputOptionsForKey(conf_editor_fbs, "flatbufServer", "enable", false);
        $('#flatbufServerHelpPanelId').hide();
      }
      conf_editor_fbs.validate().length || window.readOnlyMode ? $('#btn_submit_fbserver').prop('disabled', true) : $('#btn_submit_fbserver').prop('disabled', false);
    });

    $('#btn_submit_fbserver').off().on('click', function () {
      requestWriteConfig(conf_editor_fbs.getValue());
    });
  }

  //protobuffer
  if (PROTOTBUF_SERVER_ENABLED) {
    conf_editor_proto = createJsonEditor('editor_container_protoserver', {
      protoServer: window.schema.protoServer
    }, true, true);

    conf_editor_proto.on('change', function () {
      var protoServerEnable = conf_editor_proto.getEditor("root.protoServer.enable").getValue();
      if (protoServerEnable) {
        showInputOptionsForKey(conf_editor_proto, "protoServer", "enable", true);
        $('#protoServerHelpPanelId').show();
      } else {
        showInputOptionsForKey(conf_editor_proto, "protoServer", "enable", false);
        $('#protoServerHelpPanelId').hide();
      }
      conf_editor_proto.validate().length || window.readOnlyMode ? $('#btn_submit_protoserver').prop('disabled', true) : $('#btn_submit_protoserver').prop('disabled', false);
    });

    $('#btn_submit_protoserver').off().on('click', function () {
      requestWriteConfig(conf_editor_proto.getValue());
    });
  }

  //forwarder
  if (FORWARDER_ENABLED) {
    if (storedAccess != 'default') {
      conf_editor_forw = createJsonEditor('editor_container_forwarder', {
        forwarder: window.schema.forwarder
      }, true, true);

      conf_editor_forw.on('ready', function () {

        updateServiceCacheForwarderConfiguredItems("jsonapi");
        updateServiceCacheForwarderConfiguredItems("flatbuffer");

        var forwarderEnable = conf_editor_forw.getEditor("root.forwarder.enable").getValue();
        if (forwarderEnable) {
          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        }
      });

      conf_editor_forw.on('change', function () {
        var forwarderEnable = conf_editor_forw.getEditor("root.forwarder.enable").getValue();
        if (forwarderEnable) {
          $('#forwarderHelpPanelId').show();
        } else {
          showInputOptionsForKey(conf_editor_forw, "forwarder", "enable", false);
          $('#forwarderHelpPanelId').hide();
        }
        conf_editor_forw.validate().length || window.readOnlyMode ? $('#btn_submit_forwarder').prop('disabled', true) : $('#btn_submit_forwarder').prop('disabled', false);
      });

      conf_editor_forw.watch('root.forwarder.jsonapiselect', () => {
        updateForwarderServiceSections("jsonapi");
      });

      conf_editor_forw.watch('root.forwarder.flatbufferselect', () => {
        updateForwarderServiceSections("flatbuffer");
      });

      conf_editor_forw.watch('root.forwarder.enable', () => {

        var forwarderEnable = conf_editor_forw.getEditor("root.forwarder.enable").getValue();
        if (forwarderEnable) {
          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        }
      });

      $('#btn_submit_forwarder').off().on('click', function () {
        requestWriteConfig(conf_editor_forw.getValue());
      });
    }
  }

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_network_net_intro'), "editor_container_net");
    createHint("intro", $.i18n('conf_network_json_intro'), "editor_container_jsonserver");
    if (FLATBUF_SERVER_ENABLED) {
      createHint("intro", $.i18n('conf_network_fbs_intro'), "editor_container_fbserver");
    }
    if (PROTOTBUF_SERVER_ENABLED) {
      createHint("intro", $.i18n('conf_network_proto_intro'), "editor_container_protoserver");
    }
    if (FORWARDER_ENABLED) {
      createHint("intro", $.i18n('conf_network_forw_intro'), "editor_container_forwarder");
    }
    createHint("intro", $.i18n('conf_network_tok_intro'), "tok_desc_cont");
  }

  // Token handling
  function buildTokenList() {
    $('.tktbody').html("");
    for (var key in tokenList) {
      var lastUse = (tokenList[key].last_use) ? tokenList[key].last_use : "-";
      var btn = '<button id="tok' + tokenList[key].id + '" type="button" class="btn btn-danger">' + $.i18n('general_btn_delete') + '</button>';
      $('.tktbody').append(createTableRow([tokenList[key].comment, lastUse, btn], false, true));
      $('#tok' + tokenList[key].id).off().on('click', handleDeleteToken);
    }
  }

  createTable('tkthead', 'tktbody', 'tktable');
  $('.tkthead').html(createTableRow([$.i18n('conf_network_tok_cidhead'), $.i18n('conf_network_tok_lastuse'), $.i18n('general_btn_delete')], true, true));
  buildTokenList();

  function handleDeleteToken(e) {
    var key = e.currentTarget.id.replace("tok", "");
    requestTokenDelete(key);
    $('#tok' + key).parent().parent().remove();
    // rm deleted token id
    tokenList = tokenList.filter(function (obj) {
      return obj.id !== key;
    });
  }

  $('#btn_create_tok').off().on('click', function () {
    requestToken(encodeHTML($('#tok_comment').val()))
    $('#tok_comment').val("")
    $('#btn_create_tok').prop('disabled', true)
  });
  $('#tok_comment').off().on('input', function (e) {
    (e.currentTarget.value.length >= 10) ? $('#btn_create_tok').prop('disabled', false) : $('#btn_create_tok').prop('disabled', true);
    if (10 - e.currentTarget.value.length >= 1 && 10 - e.currentTarget.value.length <= 9)
      $('#tok_chars_needed').html(10 - e.currentTarget.value.length + " " + $.i18n('general_chars_needed'))
    else
      $('#tok_chars_needed').html("<br />")
  });
  $(window.hyperion).off("cmd-authorize-createToken").on("cmd-authorize-createToken", function (event) {
    var val = event.response.info;
    showInfoDialog("newToken", $.i18n('conf_network_tok_diaTitle'), $.i18n('conf_network_tok_diaMsg') + '<br><div style="font-weight:bold">' + val.token + '</div>')
    tokenList.push(val)
    buildTokenList()
  });

  //Reorder hardcoded token div after the general token setting div
  $("#conf_cont_tok").insertAfter("#conf_cont_net");

  function checkApiTokenState(state) {
    if (state == false)
      $("#conf_cont_tok").attr('style', 'display:none')
    else
      $("#conf_cont_tok").removeAttr('style')
  }

  $('#root_network_apiAuth').on("change", function () {
    var state = $(this).is(":checked");
    checkApiTokenState(state);
  });

  checkApiTokenState(window.serverConfig.network.apiAuth);

  removeOverlay();

  function updateForwarderServiceSections(type) {

    var editorPath = "root.forwarder." + type;
    var selectedServices = conf_editor_forw.getEditor(editorPath + "select").getValue();

    if (jQuery.isEmptyObject(selectedServices) || selectedServices[0] === "NONE") {
      conf_editor_forw.getEditor(editorPath).setValue([]);
      showInputOptionForItem(conf_editor_forw, "forwarder", type, false);
    } else {

      var newServices = [];
      for (var i = 0; i < selectedServices.length; ++i) {

        var service = discoveredRemoteServices[type][selectedServices[i]];
        var newrecord = {};

        newrecord.name = service.name;
        newrecord.host = service.host;
        newrecord.port = service.port;

        newServices.push(newrecord);
      }
      conf_editor_forw.getEditor(editorPath).setValue(newServices);

      showInputOptionForItem(conf_editor_forw, "forwarder", type, true);
      conf_editor_forw.getEditor(editorPath).disable();
    }
  }

  function updateForwarderSelectList(type) {

    var selectionElement = type + "select";

    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVals = [];

    for (var key in discoveredRemoteServices[type]) {

      var service = discoveredRemoteServices[type][key];
      enumVals.push(service.host);
      enumTitelVals.push(service.name);

      if (service.inConfig == true) {
        enumDefaultVals.push(service.host);
      }
    }

    let addSchemaElements = {
      "uniqueItems": true
    };

    if (jQuery.isEmptyObject(enumVals)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_fw_remote_service_discovered_none'));
    }

    updateJsonEditorMultiSelection(conf_editor_forw, 'root.forwarder', selectionElement, addSchemaElements, enumVals, enumTitelVals, enumDefaultVals);
  };

  function updateServiceCacheForwarderConfiguredItems(serviceType) {

    var editor = conf_editor_forw.getEditor("root.forwarder." + serviceType);

    if (editor) {
      if (!discoveredRemoteServices[serviceType]) {
        discoveredRemoteServices[serviceType] = {};
      }

      var configuredServices = JSON.parse(JSON.stringify(editor.getValue('items')));
      for (const service of configuredServices) {

        //Handle not named sceanrios
        if (!service.name) {
          service.name = service.host;
        }

        service.inConfig = true;

        discoveredRemoteServices[serviceType][service.host] = service;
      }
    }
  }

  function updateRemoteServiceCache(discoveryInfo) {

    for (var serviceType in discoveryInfo) {

      if (!discoveredRemoteServices[serviceType]) {
        discoveredRemoteServices[serviceType] = {};
      }

      var discoveredServices = discoveryInfo[serviceType];
      for (const service of discoveredServices) {

        if (!service.sameHost)
        {
          //Handle non mDNS sceanrios
          if (!service.name) {
            service.name = service.host;
          } else {
            service.host = service.service;
          }

          if (discoveredRemoteServices[serviceType][service.host]) {
            service.inConfig = true;
          }

          discoveredRemoteServices[serviceType][service.host] = service;
        }
      }
    }
  };

  async function discoverRemoteHyperionServices(type, params) {

    const result = await requestServiceDiscovery(type, params);

    var discoveryResult;
    if (result && !result.error) {
      discoveryResult = result.info;
    }
    else {
      discoveryResult = {
        "services": []
      };
    }

    switch (type) {
      case "jsonapi":
      case "flatbuffer":
        updateRemoteServiceCache(discoveryResult.services);
        updateForwarderSelectList(type);
        break;
    }
  };

});

