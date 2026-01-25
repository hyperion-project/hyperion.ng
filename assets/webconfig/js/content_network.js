$(document).ready(function () {
  performTranslation();

  const services = globalThis.serverInfo.services;
  const isServiceEnabled = (service) => services.includes(service);
  const isForwarderEnabled = isServiceEnabled("forwarder");
  const isFlatbufEnabled = isServiceEnabled("flatbuffer");
  const isProtoBufEnabled = isServiceEnabled("protobuffer");

  let editors = {}; // Store JSON editors in a structured way

  // Service properties , 2-dimensional array of [servicetype][id]
  let discoveredRemoteServices = new Map();

  addJsonEditorHostValidation();
  initializeUI();
  setupEditors();
  setupTokenManagement();

  removeOverlay();

  function initializeUI() {
    if (globalThis.showOptHelp) {
      createSection("network", "edt_conf_network_heading_title", globalThis.schema.network.properties);
      createSection("jsonServer", "edt_conf_jsonServer_heading_title", globalThis.schema.jsonServer.properties);
      if (isFlatbufEnabled) createSection("flatbufServer", "edt_conf_flatbufServer_heading_title", globalThis.schema.flatbufServer.properties, "flatbufServerHelpPanelId");
      if (isProtoBufEnabled) createSection("protoServer", "edt_conf_protoServer_heading_title", globalThis.schema.protoServer.properties, "protoServerHelpPanelId");
      if (isForwarderEnabled && storedAccess !== 'default') createSection("forwarder", "edt_conf_forwarder_heading_title", globalThis.schema.forwarder.properties, "forwarderHelpPanelId");
    } else {
      $('#conf_cont').addClass('row');
      appendPanel("network", "edt_conf_network_heading_title");
      appendPanel("jsonServer", "edt_conf_jsonServer_heading_title");
      if (isFlatbufEnabled) appendPanel("flatbufServer", "edt_conf_flatbufSserver_heading_title");
      if (isProtoBufEnabled) appendPanel("protoServer", "edt_conf_protoServer_heading_title");
      if (isForwarderEnabled) appendPanel("forwarder", "edt_conf_forwarder_heading_title");
      $("#conf_cont_tok").removeClass('row');
    }
  }

  function setupEditors() {
    createEditor("network", "network");
    createEditor("jsonServer", "jsonServer");
    if (isFlatbufEnabled) createEditor("flatbufServer", "flatbufServer", handleFlatbufChange);
    if (isProtoBufEnabled) createEditor("protoServer", "protoServer", handleProtoBufChange);
    if (isForwarderEnabled && storedAccess !== 'default') createEditor("forwarder", "forwarder", handleForwarderChange);

    const editorConfigs = [
      { key: "network", schemaKey: "network" },
      { key: "jsonServer", schemaKey: "jsonServer" },
      { key: "flatbufServer", schemaKey: "flatbufServer", enabled: isFlatbufEnabled, handler: handleFlatbufChange },
      { key: "protoServer", schemaKey: "protoServer", enabled: isProtoBufEnabled, handler: handleProtoBufChange },
      { key: "forwarder", schemaKey: "forwarder", enabled: isForwarderEnabled && storedAccess !== 'default', handler: handleForwarderChange }
    ];

    editorConfigs.forEach(({ key, schemaKey, enabled = true, handler }) => {
      if (enabled) createEditor(key, schemaKey, handler);
    });


    function handleFlatbufChange(editor) {
      editor.on('change', () => toggleHelpPanel(editor, "flatbufServer", "flatbufServerHelpPanelId"));

      editor.watch('root.flatbufServer.enable', () => {
        const enable = editor.getEditor("root.flatbufServer.enable").getValue();
        showInputOptionsForKey(editor, "flatbufServer", "enable", enable);
      });
    }

    function handleProtoBufChange(editor) {
      editor.on('change', () => toggleHelpPanel(editor, "protoServer", "protoServerHelpPanelId"));

      editor.watch('root.protoServer.enable', () => {
        const enable = editor.getEditor("root.protoServer.enable").getValue();
        showInputOptionsForKey(editor, "protoServer", "enable", enable);
      });
    }



    function handleForwarderChange(editor) {
      editor.on('ready', () => {
        updateServiceCacheForwarderConfiguredItems("jsonapi");
        updateServiceCacheForwarderConfiguredItems("flatbuffer");

        if (editor.getEditor("root.forwarder.enable").getValue()) {
          updateConfiguredInstancesList();
          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        } else {
          showInputOptionsForKey(editor, "forwarder", "enable", false);
        }
      });

      editor.on('change', () => {
        toggleHelpPanel(editor, "forwarder", "forwarderHelpPanelId");
      });

      ["jsonapi", "flatbuffer"].forEach(function (type) {
        editor.watch(`root.forwarder.${type}select`, () => {
          updateForwarderServiceSections(type);
        });
        editor.watch(`root.forwarder.${type}`, () => {
          onChangeForwarderServiceSections(type);
        });
      });

      editor.watch('root.forwarder.enable', () => {
        const isEnabled = editor.getEditor("root.forwarder.enable").getValue();
        if (isEnabled) {

          updateConfiguredInstancesList();

          const instanceId = editor.getEditor("root.forwarder.instanceList").getValue();
          if (["NONE", "SELECT", "", undefined].includes(instanceId)) {
            editor.getEditor("root.forwarder.instance").setValue(-1);
          }

          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        } else {
          const instance = editor.getEditor("root.forwarder.instance").getValue();
          if (instance === -1) {
            editor.getEditor("root.forwarder.instance").setValue(255);
          }
        }
        showInputOptionsForKey(editor, "forwarder", "enable", isEnabled);
      });

      editor.watch('root.forwarder.instanceList', () => {
        const instanceId = editor.getEditor("root.forwarder.instanceList").getValue();
        if (!["NONE", "SELECT", "", undefined].includes(instanceId)) {
          editor.getEditor("root.forwarder.instance").setValue(Number.parseInt((instanceId, 10)));
        }
      });
    }

  }

  // Validate for conflicting ports
  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    let errors = [];

    const conflictingPorts = {
      "root.jsonServer.port": ["flatbufServer", "protoServer", "webConfig_port", "webConfig_sslPort"],
      "root.flatbufServer.port": ["jsonServer", "protoServer", "webConfig_port", "webConfig_sslPort"],
      "root.protoServer.port": ["jsonServer", "flatbufServer", "webConfig_port", "webConfig_sslPort"]
    };

    if (!(path in conflictingPorts)) {
      return [];
    }

    conflictingPorts[path].forEach(conflictKey => {
      let conflictPort;

      const isWebConfigPort = conflictKey.startsWith("webConfig");
      if (isWebConfigPort) {
        conflictPort = globalThis.serverConfig?.webConfig?.[conflictKey.replace("webConfig_", "")];
      } else {
        conflictPort = editors?.[conflictKey]?.getEditor(`root.${conflictKey}.port`)?.getValue();
      }

      if (conflictPort != null && value === conflictPort) {
        let errorText;

        if (isWebConfigPort) {
          errorText = $.i18n("edt_conf_webConfig_heading_title") + " - " + $.i18n(`edt_conf_${conflictKey}_title`);
        } else {
          errorText = $.i18n(`edt_conf_${conflictKey}_heading_title`);
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

  function setupTokenManagement() {
    createTable('tkthead', 'tktbody', 'tktable');
    $('.tkthead').html(createTableRow([$.i18n('conf_network_tok_idhead'), $.i18n('conf_network_tok_cidhead'), $.i18n('conf_network_tok_lastuse'), $.i18n('general_btn_delete')], true, true));

    buildTokenList();

    // Reorder hardcoded token div after the general token setting div
    $("#conf_cont_tok").insertAfter("#conf_cont_network");

    // Initial state check based on server config
    checkApiTokenState(globalThis.serverConfig.network.localApiAuth || storedAccess === 'expert');

    // Listen for changes on the local API Auth toggle
    $('#root_network_localApiAuth').on("change", function () {
      checkApiTokenState($(this).is(":checked"));
    });

    $('#btn_create_tok').off().on('click', function () {
      requestToken(encodeHTML($('#tok_comment').val()));
      $('#tok_comment').val("").prop('disabled', true);
    });

    $('#tok_comment').off().on('input', function (e) {
      const charsNeeded = 10 - e.currentTarget.value.length;
      $('#btn_create_tok').prop('disabled', charsNeeded > 0);
      $('#tok_chars_needed').html(charsNeeded > 0 ? `${charsNeeded} ${$.i18n('general_chars_needed')}` : "<br />");
    });

    $(globalThis.hyperion).off("cmd-authorize-createToken").on("cmd-authorize-createToken", function (event) {
      const val = event.response.info;
      showInfoDialog("newToken", $.i18n('conf_network_tok_diaTitle'), $.i18n('conf_network_tok_diaMsg') + `<br><div style="font-weight:bold">${val.token}</div>`);
      addToTokenList(val);

      buildTokenList();
    });

    function buildTokenList(tokenList = null) {
      $('.tktbody').empty();

      const list = tokenList || getTokenList();
      list.forEach(token => {
        const lastUse = token.last_use || "-";
        const btn = `<button id="tok${token.id}" type="button" class="btn btn-danger">${$.i18n('general_btn_delete')}</button>`;
        $('.tktbody').append(createTableRow([token.id, token.comment, lastUse, btn], false, true));
        $(`#tok${token.id}`).off().on('click', () => handleDeleteToken(token.id));
      });
    }

    function handleDeleteToken(id) {
      requestTokenDelete(id);

      deleteFromTokenList(id);
      buildTokenList();
    }

    function checkApiTokenState(state) {
      if (!state && storedAccess !== 'expert') {
        $("#conf_cont_tok").hide();
      } else {
        $("#conf_cont_tok").show();
      }
    }
  }

  function onChangeForwarderServiceSections(type) {
    const editor = editors["forwarder"].getEditor(`root.forwarder.${type}`);
    const configuredServices = JSON.parse(JSON.stringify(editor?.getValue('items')));

    configuredServices.forEach((serviceConfig, i) => {
      const itemEditor = editors["forwarder"].getEditor(`root.forwarder.${type}.${i}`);
      const service = discoveredRemoteServices.get(type)?.get(serviceConfig.host);

      if (service?.wasDiscovered) {
        itemEditor.disable();

        const instanceIdsEditor = editors["forwarder"].getEditor(`root.forwarder.${type}.${i}.instanceIds`);
        instanceIdsEditor?.enable();

        showInputOptions(`root.forwarder.${type}.${i}`, ["name"], true);
      } else {
        itemEditor.enable();
        itemEditor.getEditor(`root.forwarder.${type}.${i}.name`).setValue(serviceConfig.name);
        itemEditor.getEditor(`root.forwarder.${type}.${i}.host`).setValue(serviceConfig.host);
        itemEditor.getEditor(`root.forwarder.${type}.${i}.port`).setValue(serviceConfig.port);
        itemEditor.getEditor(`root.forwarder.${type}.${i}.instanceIds`).setValue(serviceConfig.instanceIds);
      }
    });
  }

  function updateForwarderServiceSections(type) {
    const editorPath = `root.forwarder.${type}`;
    const selectedServices = editors["forwarder"].getEditor(`${editorPath}select`).getValue();

    if (!selectedServices || selectedServices.length === 0 || ["NONE", "SELECT"].includes(selectedServices[0])) {
      return;
    }

    const newServices = selectedServices.map((serviceKey) => {
      const service = discoveredRemoteServices.get(type).get(serviceKey);
      return {
        name: service.name,
        host: service.host,
        port: service.port,
        instanceIds: service.instanceIds,
        wasDiscovered: service.wasDiscovered
      };
    });

    editors["forwarder"].getEditor(editorPath).setValue(newServices);
  }

  function updateForwarderSelectList(type) {
    const selectionElement = `${type}select`;

    const enumVals = [];
    const enumTitleVals = [];
    const enumDefaultVals = [];

    if (discoveredRemoteServices.has(type)) {
      for (const service of discoveredRemoteServices.get(type).values()) {
        enumVals.push(service.host);
        enumTitleVals.push(service.name);
        if (service.inConfig) {
          enumDefaultVals.push(service.host);
        }
      }
    }

    const addSchemaElements = { "uniqueItems": true };

    if (enumVals.length === 0) {
      enumVals.push("NONE");
      enumTitleVals.push($.i18n('edt_conf_forwarder_remote_service_discovered_none'));
    }

    updateJsonEditorMultiSelection(
      editors["forwarder"],
      'root.forwarder',
      selectionElement,
      addSchemaElements,
      enumVals,
      enumTitleVals,
      enumDefaultVals
    );
  }

  function updateServiceCacheForwarderConfiguredItems(serviceType) {
    const editor = editors["forwarder"].getEditor(`root.forwarder.${serviceType}`);

    if (editor) {
      if (!discoveredRemoteServices.has(serviceType)) {
        discoveredRemoteServices.set(serviceType, new Map());
      }

      const configuredServices = JSON.parse(JSON.stringify(editor.getValue('items')));
      configuredServices.forEach((service) => {
        service.inConfig = true;
        let existingService = discoveredRemoteServices.get(serviceType).get(service.host) || {};
        discoveredRemoteServices.get(serviceType).set(service.host, { ...existingService, ...service });
      });
    }
  }

  function updateRemoteServiceCache(discoveryInfo) {

    debugger;
    Object.entries(discoveryInfo).forEach(([serviceType, discoveredServices]) => {
      if (!discoveredRemoteServices.has(serviceType)) {
        discoveredRemoteServices.set(serviceType, new Map());
      }

      discoveredServices.forEach((service) => {
        if (!service.sameHost) {
          service.name = service.name || service.host;
          service.host = service.service || service.host;
          service.wasDiscovered = Boolean(service.service);

          // Might be updated when instance IDs are provided by the remote service info
          service.instanceIds = [];

          if (discoveredRemoteServices.get(serviceType).has(service.host)) {
            service.inConfig = true;
            service.instanceIds = discoveredRemoteServices.get(serviceType).get(service.host).instanceIds;
          }

          discoveredRemoteServices.get(serviceType).set(service.host, service);
        }
      });
    });
  }

  async function discoverRemoteHyperionServices(type, params) {
    const result = await requestServiceDiscovery(type, params);

    const discoveryResult = result && !result.error ? result.info : { services: [] };

    if (["jsonapi", "flatbuffer"].includes(type)) {
      updateRemoteServiceCache(discoveryResult.services);
      updateForwarderSelectList(type);
    }
  }

});

function createSection(id, titleKey, schemaProps, helpPanelId = null) {
  const containerId = `conf_cont_${id}`;
  $('#conf_cont').append(createRow(containerId));
  $(`#${containerId}`)
    .append(createOptPanel('fa-sitemap', $.i18n(titleKey), `editor_container_${id}`, `btn_submit_${id}`, 'panel-system'))
    .append(createHelpTable(schemaProps, $.i18n(titleKey), helpPanelId));
}

function appendPanel(id, titleKey) {
  const containerId = `conf_cont_${id}`;

  // Create the container element
  const $newContainer = $('<div></div>', { id: containerId });

  // Append the newly created container to #conf_cont
  $('#conf_cont').append($newContainer);

  // Append the option panel inside the newly created container
  $newContainer.append(createOptPanel('fa-sitemap', $.i18n(titleKey), `editor_container_${id}`, `btn_submit_${id}`, 'panel-system'));
}

  function createEditor(container, schemaKey, changeHandler) {
    editors[container] = createJsonEditor(
      `editor_container_${container}`,
      { [schemaKey]: globalThis.schema[schemaKey] },
      true,
      true
    );

    editors[container].on('change', function () {
      const isValid = editors[container].validate().length === 0 && !globalThis.readOnlyMode;
      $(`#btn_submit_${container}`).prop('disabled', !isValid);
    });

    $(`#btn_submit_${container}`).off().on('click', function () {
      requestWriteConfig(editors[container].getValue());
    });

    if (changeHandler) changeHandler(editors[container]);
  }

    function updateConfiguredInstancesList() {
    const enumVals = [];
    const enumTitelVals = [];
    let enumDefaultVal = "";
    let addSelect = false;

    const configuredInstances = globalThis.serverInfo.instance;

    if (!configuredInstances || Object.keys(configuredInstances).length === 0) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_forwarder_no_instance_configured_title'));
    } else {
      Object.values(configuredInstances).forEach(({ friendly_name, instance }) => {
        enumTitelVals.push(friendly_name);
        enumVals.push(instance.toString());
      });

      const configuredInstance = globalThis.serverConfig.forwarder.instance.toString();

      if (enumVals.includes(configuredInstance)) {
        enumDefaultVal = configuredInstance;
      } else {
        addSelect = true;
      }
    }

    if (enumVals.length > 0) {
      updateJsonEditorSelection(editors["forwarder"], 'root.forwarder',
        'instanceList', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  }

  function toggleHelpPanel(editor, key, panelId) {
    const enable = editor.getEditor(`root.${key}.enable`).getValue();
    $(`#${panelId}`).toggle(enable);
  }