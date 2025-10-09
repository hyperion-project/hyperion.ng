$(document).ready(function () {
  performTranslation();

  let conf_editor = null;

  $('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_webConfig_heading_title"), 'editor_container', 'btn_submit', 'panel-system'));
  if (window.showOptHelp) {
    $('#conf_cont').append(createHelpTable(window.schema.webConfig.properties, $.i18n("edt_conf_webConfig_heading_title")));
  }

  conf_editor = createJsonEditor('editor_container', {
    webConfig: window.schema.webConfig
  }, true, true);

  // Validate for conflicting ports
  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    let errors = [];
    const conflictingPorts = {
      "root.webConfig.port": ["jsonServer", "flatbufServer", "protoServer", "webConfig_sslPort"],
      "root.webConfig.sslPort": ["jsonServer", "flatbufServer", "protoServer", "webConfig_port"]
    };

    if (!(path in conflictingPorts)) {
      return [];
    }

    conflictingPorts[path].forEach(conflictKey => {
      let conflictPort;

      const isWebConfigPort = conflictKey.startsWith("webConfig");
      if (isWebConfigPort) {
        conflictPort = conf_editor?.getEditor(`root.${conflictKey.replace("_", ".")}`)?.getValue();
      } else {
        conflictPort = window.serverConfig?.[conflictKey]?.port;
      }

      if (conflictPort != null && value === conflictPort) {
        let errorText;

        if (isWebConfigPort) {
          errorText = $.i18n(`edt_conf_${conflictKey}_title`);
        } else {
          errorText = $.i18n("main_menu_network_conf_token") + " - " + $.i18n(`edt_conf_${conflictKey}_heading_title`);
        }

        errors.push({
          path: path,
          property: "port",
          message: $.i18n('edt_conf_network_port_validation_error', errorText)
        });
      }

    });

    return errors;
  });

  conf_editor.on('change', function () {
    conf_editor.validate().length || window.readOnlyMode ? $('#btn_submit').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
  });

  $('#btn_submit').off().on('click', function () {
    // store the last webui port for correct reconnect url (connection_lost.html)
    const val = conf_editor.getValue();
    window.fastReconnect = true;
    window.jsonPort = val.webConfig.port;
    requestWriteConfig(val);
  });

  if (window.showOptHelp)
    createHint("intro", $.i18n('conf_webconfig_label_intro'), "editor_container");

  removeOverlay();
});
