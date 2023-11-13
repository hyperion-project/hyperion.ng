$(document).ready(function () {
  performTranslation();

  const CEC_ENABLED = (jQuery.inArray("cec", window.serverInfo.services) !== -1);

  let conf_editor_osEvents = null;
  let conf_editor_cecEvents = null;

  if (window.showOptHelp) {
    //Operating System Events
    $('#conf_cont').append(createRow('conf_cont_os_events'));
    $('#conf_cont_os_events').append(createOptPanel('fa-laptop', $.i18n("conf_os_events_heading_title"), 'editor_container_os_events', 'btn_submit_os_events', 'panel-system'));
    $('#conf_cont_os_events').append(createHelpTable(window.schema.osEvents.properties, $.i18n("conf_os_events_heading_title")));

    //CEC Events
    if (CEC_ENABLED) {
      $('#conf_cont').append(createRow('conf_cont_event_cec'));
      $('#conf_cont_event_cec').append(createOptPanel('fa-tv', $.i18n("conf_cec_events_heading_title"), 'editor_container_cec_events', 'btn_submit_cec_events', 'panel-system'));
      $('#conf_cont_event_cec').append(createHelpTable(window.schema.cecEvents.properties, $.i18n("conf_cec_events_heading_title"), "cecEventsHelpPanelId"));
    }
  }
  else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-laptop', $.i18n("conf_os_events_heading_title"), 'editor_container_os_events', 'btn_submit_os_events'));
    if (CEC_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-tv', $.i18n("conf_cec_events_heading_title"), 'editor_container_cec_events', 'btn_submit_cec_events'));
    }
  }

  function findDuplicateCecEventsIndices(data) {
    const cecEventIndices = {};
    data.forEach((item, index) => {
      const cecEvent = item.cec_event;
      if (!cecEventIndices[cecEvent]) {
        cecEventIndices[cecEvent] = [index];
      } else {
        cecEventIndices[cecEvent].push(index);
      }
    });

    return Object.values(cecEventIndices).filter(indices => indices.length > 1);
  }

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    let errors = [];
    if (schema.type === 'array' && Array.isArray(value)) {
      const duplicateCecEventIndices = findDuplicateCecEventsIndices(value);

      if (duplicateCecEventIndices.length > 0) {

        let recs;
        duplicateCecEventIndices.forEach(indices => {
          const displayIndices = indices.map(index => index + 1);
          recs = displayIndices.join(', ');
        });

        errors.push({
          path: path,
          message: $.i18n('edt_conf_cec_action_record_validation_error', recs)
        });
      }
    }
    return errors;
  });

  //Operating System Events
  conf_editor_osEvents = createJsonEditor('editor_container_os_events', {
    osEvents: window.schema.osEvents
  }, true, true);

  conf_editor_osEvents.on('change', function () {
    conf_editor_osEvents.validate().length || window.readOnlyMode ? $('#btn_submit_os_events').prop('disabled', true) : $('#btn_submit_os_events').prop('disabled', false);
  });

  $('#btn_submit_os_events').off().on('click', function () {
    requestWriteConfig(conf_editor_osEvents.getValue());
  });

  //CEC Events
  if (CEC_ENABLED) {
    conf_editor_cecEvents = createJsonEditor('editor_container_cec_events', {
      cecEvents: window.schema.cecEvents
    }, true, true);

    conf_editor_cecEvents.on('change', function () {

      const cecEventsEnable = conf_editor_cecEvents.getEditor("root.cecEvents.enable").getValue();

      if (cecEventsEnable) {
        showInputOptionsForKey(conf_editor_cecEvents, "cecEvents", "enable", true);
        $('#cecEventsHelpPanelId').show();
      } else {
        showInputOptionsForKey(conf_editor_cecEvents, "cecEvents", "enable", false);
        $('#cecEventsHelpPanelId').hide();
      }

      conf_editor_cecEvents.validate().length || window.readOnlyMode ? $('#btn_submit_cec_events').prop('disabled', true) : $('#btn_submit_cec_events').prop('disabled', false);
    });

    $('#btn_submit_cec_events').off().on('click', function () {

      const saveOptions = conf_editor_cecEvents.getValue();
      // Workaround, as otherwise actions array is empty	 
      saveOptions.cecEvents.actions = conf_editor_cecEvents.getEditor("root.cecEvents.actions").getValue();
      requestWriteConfig(saveOptions);
    });
  }

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_os_events_intro'), "editor_container_os_events");
    if (CEC_ENABLED) {
      createHint("intro", $.i18n('conf_cec_events_intro'), "editor_container_cec_events");
    }
  }

  removeOverlay();
});

