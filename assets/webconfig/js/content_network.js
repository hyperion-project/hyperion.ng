$(document).ready(function () {
  performTranslation();

  const services = window.serverInfo.services;
  const isServiceEnabled = (service) => services.includes(service);
  const isForwarderEnabled = isServiceEnabled("forwarder");
  const isFlatbufEnabled = isServiceEnabled("flatbuffer");
  const isProtoBufEnabled = isServiceEnabled("protobuffer");

  let editors = {}; // Store JSON editors in a structured way

  // Service properties , 2-dimensional array of [servicetype][id]
  let discoveredRemoteServices = {};

  addJsonEditorHostValidation();
  initializeUI();
  setupEditors();
  setupTokenManagement();

  removeOverlay();

  function initializeUI() {
    if (window.showOptHelp) {
      createSection("network", "edt_conf_net_heading_title", window.schema.network.properties);
      createSection("jsonserver", "edt_conf_js_heading_title", window.schema.jsonServer.properties);
      if (isFlatbufEnabled) createSection("flatbufserver", "edt_conf_fbs_heading_title", window.schema.flatbufServer.properties, "flatbufServerHelpPanelId");
      if (isProtoBufEnabled) createSection("protoserver", "edt_conf_pbs_heading_title", window.schema.protoServer.properties, "protoServerHelpPanelId");
      if (isForwarderEnabled && storedAccess !== 'default') createSection("forwarder", "edt_conf_fw_heading_title", window.schema.forwarder.properties, "forwarderHelpPanelId");
    } else {
      $('#conf_cont').addClass('row')
      appendPanel("network", "edt_conf_net_heading_title");
      appendPanel("jsonserver", "edt_conf_js_heading_title");
      if (isFlatbufEnabled) appendPanel("flatbufserver", "edt_conf_fbs_heading_title");
      if (isProtoBufEnabled) appendPanel("protoserver", "edt_conf_pbs_heading_title");
      if (isForwarderEnabled) appendPanel("forwarder", "edt_conf_fv_heading_title");
      $("#conf_cont_tok").removeClass('row');
    }

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
  }

  function setupEditors() {
    createEditor("network", "network");
    createEditor("jsonserver", "jsonServer");
    if (isFlatbufEnabled) createEditor("flatbufserver", "flatbufServer", handleFlatbufChange);
    if (isProtoBufEnabled) createEditor("protoserver", "protoServer", handleProtoBufChange);
    if (isForwarderEnabled && storedAccess !== 'default') createEditor("forwarder", "forwarder", handleForwarderChange);

    function createEditor(container, schemaKey, changeHandler) {
      editors[container] = createJsonEditor(`editor_container_${container}`, { [schemaKey]: window.schema[schemaKey] }, true, true);

      editors[container].on('change', function () {
        const isValid = editors[container].validate().length === 0 && !window.readOnlyMode;
        $(`#btn_submit_${container}`).prop('disabled', !isValid);
      });

      $(`#btn_submit_${container}`).off().on('click', function () {
        requestWriteConfig(editors[container].getValue());
      });

      if (changeHandler) changeHandler(editors[container]);
    }

    function handleFlatbufChange(editor) {
      editor.on('change', () => toggleHelpPanel(editor, "flatbufServer", "flatbufServerHelpPanelId"));
    }

    function handleProtoBufChange(editor) {
      editor.on('change', () => toggleHelpPanel(editor, "protoServer", "protoServerHelpPanelId"));
    }

    function handleForwarderChange(editor) {
      editor.on('ready', () => {
        updateServiceCacheForwarderConfiguredItems("jsonapi");
        updateServiceCacheForwarderConfiguredItems("flatbuffer");
        if (editor.getEditor("root.forwarder.enable").getValue()) {
          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        }
      });

      editor.on('change', () => toggleHelpPanel(editor, "forwarder", "forwarderHelpPanelId"));

      ["jsonapi", "flatbuffer"].forEach((type) => {
        editor.watch(`root.forwarder.${type}select`, () => updateForwarderServiceSections(type));
        editor.watch(`root.forwarder.${type}`, () => onChangeForwarderServiceSections(type));
      });

      editor.watch('root.forwarder.enable', () => {
        if (editor.getEditor("root.forwarder.enable").getValue()) {
          discoverRemoteHyperionServices("jsonapi");
          discoverRemoteHyperionServices("flatbuffer");
        }
      });
    }

    function toggleHelpPanel(editor, key, panelId) {
      const enable = editor.getEditor(`root.${key}.enable`).getValue();
      $(`#${panelId}`).toggle(enable);
      showInputOptionsForKey(editor, key, "enable", enable);
    }
  }

  function setupTokenManagement() {
    createTable('tkthead', 'tktbody', 'tktable');
    $('.tkthead').html(createTableRow([$.i18n('conf_network_tok_idhead'), $.i18n('conf_network_tok_cidhead'), $.i18n('conf_network_tok_lastuse'), $.i18n('general_btn_delete')], true, true));

    buildTokenList();

    // Reorder hardcoded token div after the general token setting div
    $("#conf_cont_tok").insertAfter("#conf_cont_network");

    // Initial state check based on server config
    checkApiTokenState(window.serverConfig.network.localApiAuth);

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

    $(window.hyperion).off("cmd-authorize-createToken").on("cmd-authorize-createToken", function (event) {
      const val = event.response.info;
      showInfoDialog("newToken", $.i18n('conf_network_tok_diaTitle'), $.i18n('conf_network_tok_diaMsg') + `<br><div style="font-weight:bold">${val.token}</div>`);
      tokenList.push(val);
      buildTokenList();
    });

    function buildTokenList() {
      $('.tktbody').empty();
      tokenList.forEach(token => {
        const lastUse = token.last_use || "-";
        const btn = `<button id="tok${token.id}" type="button" class="btn btn-danger">${$.i18n('general_btn_delete')}</button>`;
        $('.tktbody').append(createTableRow([token.id, token.comment, lastUse, btn], false, true));
        $(`#tok${token.id}`).off().on('click', () => handleDeleteToken(token.id));
      });
    }

    function handleDeleteToken(id) {
      requestTokenDelete(id);
      tokenList = tokenList.filter(token => token.id !== id);
      buildTokenList();
    }

    function checkApiTokenState(state) {
      if (!state) {
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
      const service = discoveredRemoteServices[type]?.[serviceConfig.host];

      if (service?.wasDiscovered) {
        itemEditor?.disable();
        showInputOptions(`root.forwarder.${type}.${i}`, ["name"], true);
      } else {
        itemEditor?.enable();
        showInputOptions(`root.forwarder.${type}.${i}`, ["name"], false);

        if (!service) {
          const hostEditor = editors["forwarder"].getEditor(`root.forwarder.${type}.${i}.host`);
          if (hostEditor?.getValue()) {
            updateServiceCacheForwarderConfiguredItems(type);
            updateForwarderSelectList(type);
          }
        }
      }
    });
  }

  function updateForwarderServiceSections(type) {
    const editorPath = `root.forwarder.${type}`;
    const selectedServices = editors["forwarder"].getEditor(`${editorPath}select`).getValue();

    if (jQuery.isEmptyObject(selectedServices) || selectedServices[0] === "NONE") {
      return;
    }

    const newServices = selectedServices.map((serviceKey) => {
      const service = discoveredRemoteServices[type][serviceKey];
      return {
        name: service.name,
        host: service.host,
        port: service.port,
        wasDiscovered: service.wasDiscovered
      };
    });

    editors["forwarder"].getEditor(editorPath).setValue(newServices);
    showInputOptionForItem(editors["forwarder"], "forwarder", type, true);
  }

  function updateForwarderSelectList(type) {
    const selectionElement = type + "select";

    let enumVals = [];
    let enumTitleVals = [];
    let enumDefaultVals = [];

    for (let key in discoveredRemoteServices[type]) {
      const service = discoveredRemoteServices[type][key];
      enumVals.push(service.host);
      enumTitleVals.push(service.name);

      if (service.inConfig === true) {
        enumDefaultVals.push(service.host);
      }
    }

    let addSchemaElements = { "uniqueItems": true };

    if (jQuery.isEmptyObject(enumVals)) {
      enumVals.push("NONE");
      enumTitleVals.push($.i18n('edt_conf_fw_remote_service_discovered_none'));
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
      if (!discoveredRemoteServices[serviceType]) {
        discoveredRemoteServices[serviceType] = {};
      }

      const configuredServices = JSON.parse(JSON.stringify(editor.getValue('items')));

      configuredServices.forEach((service) => {
        service.inConfig = true;
        let existingService = discoveredRemoteServices[serviceType][service.host] || {};
        discoveredRemoteServices[serviceType][service.host] = { ...existingService, ...service };
      });
    }
  }


  function updateRemoteServiceCache(discoveryInfo) {
    Object.entries(discoveryInfo).forEach(([serviceType, discoveredServices]) => {
      discoveredRemoteServices[serviceType] = discoveredRemoteServices[serviceType] || {};

      discoveredServices.forEach((service) => {
        if (!service.sameHost) {
          service.name = service.name || service.host;
          service.host = service.service || service.host;
          service.wasDiscovered = Boolean(service.service);

          if (discoveredRemoteServices[serviceType][service.host]) {
            service.inConfig = true;
          }

          discoveredRemoteServices[serviceType][service.host] = service;
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


