$(document).ready(function () {
  performTranslation();

  const CEC_ENABLED = (jQuery.inArray("cec", window.serverInfo.services) !== -1);

  let conf_editor_osEvents = null;
  let conf_editor_cecEvents = null;
  let conf_editor_schedEvents = null;

  if (window.showOptHelp) {
    //Operating System Events
    $('#conf_cont').append(createRow('conf_cont_os_events'));
    $('#conf_cont_os_events').append(createOptPanel('fa-laptop', $.i18n("conf_os_events_heading_title"), 'editor_container_os_events', 'btn_submit_os_events', 'panel-system'));
    $('#conf_cont_os_events').append(createHelpTable(window.schema.osEvents.properties, $.i18n("conf_os_events_heading_title")));

    //Scheduled Events
    $('#conf_cont').append(createRow('conf_cont_sched_events'));
    $('#conf_cont_sched_events').append(createOptPanel('fa-laptop', $.i18n("conf_sched_events_heading_title"), 'editor_container_sched_events', 'btn_submit_sched_events', 'panel-system'));
    $('#conf_cont_sched_events').append(createHelpTable(window.schema.schedEvents.properties, $.i18n("conf_sched_events_heading_title")));


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
    $('#conf_cont').append(createOptPanel('fa-laptop', $.i18n("conf_sched_events_heading_title"), 'editor_container_sched_events', 'btn_submit_sched_events'));
    if (CEC_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-tv', $.i18n("conf_cec_events_heading_title"), 'editor_container_cec_events', 'btn_submit_cec_events'));
    }
  }

  function findDuplicateEventsIndices(data) {
    const eventIndices = {};
    data.forEach((item, index) => {
      const event = item.event;
      if (!eventIndices[event]) {
        eventIndices[event] = [index];
      } else {
        eventIndices[event].push(index);
      }
    });

    return Object.values(eventIndices).filter(indices => indices.length > 1);
  }

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    let errors = [];
    if (schema.type === 'array' && Array.isArray(value)) {
      const duplicateEventIndices = findDuplicateEventsIndices(value);

      if (duplicateEventIndices.length > 0) {

        let recs;
        duplicateEventIndices.forEach(indices => {
          const displayIndices = indices.map(index => index + 1);
          recs = displayIndices.join(', ');
        });

        errors.push({
          path: path,
          message: $.i18n('edt_conf_action_record_validation_error', recs)
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

  //Scheduled Events
  conf_editor_schedEvents = createJsonEditor('editor_container_sched_events', {
    schedEvents: window.schema.schedEvents
  }, true, true);

  conf_editor_schedEvents.on('change', function () {

    const schedEventsEnable = conf_editor_schedEvents.getEditor("root.schedEvents.enable").getValue();

    if (schedEventsEnable) {
      showInputOptionsForKey(conf_editor_schedEvents, "schedEvents", "enable", true);
      $('#schedEventsHelpPanelId').show();
    } else {
      showInputOptionsForKey(conf_editor_schedEvents, "schedEvents", "enable", false);
      $('#schedEventsHelpPanelId').hide();
    }

    conf_editor_schedEvents.validate().length || window.readOnlyMode ? $('#btn_submit_sched_events').prop('disabled', true) : $('#btn_submit_sched_events').prop('disabled', false);
  });

  $('#btn_submit_sched_events').off().on('click', function () {

    const saveOptions = conf_editor_schedEvents.getValue();
    // Workaround, as otherwise values are not reflected correctly
    saveOptions.schedEvents.enable = conf_editor_schedEvents.getEditor("root.schedEvents.enable").getValue();
    saveOptions.schedEvents.actions = conf_editor_schedEvents.getEditor("root.schedEvents.actions").getValue();
    requestWriteConfig(saveOptions);
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
      // Workaround, as otherwise values are not reflected correctly	 
      saveOptions.cecEvents.enable = conf_editor_cecEvents.getEditor("root.cecEvents.enable").getValue();
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

