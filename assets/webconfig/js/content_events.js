$(document).ready(function () {
  performTranslation();

  var conf_editor_systemEvents = null;

  if (window.showOptHelp) {
    //System Events
    $('#conf_cont').append(createRow('conf_cont_system_events'));
    $('#conf_cont_system_events').append(createOptPanel('fa-laptop', $.i18n("conf_system_events_heading_title"), 'editor_container_system_events', 'btn_submit_system_events', 'panel-system'));
    $('#conf_cont_system_events').append(createHelpTable(window.schema.systemEvents.properties, $.i18n("conf_system_events_heading_title")));
  }
  else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-laptop', $.i18n("conf_system_events_heading_title"), 'editor_container_system_events', 'btn_submit_system_events'));
  }

  //System Events
  conf_editor_systemEvents = createJsonEditor('editor_container_system_events', {
    systemEvents: window.schema.systemEvents
  }, true, true);

  conf_editor_systemEvents.on('change', function () {
    conf_editor_systemEvents.validate().length || window.readOnlyMode ? $('#btn_submit_system_events').prop('disabled', true) : $('#btn_submit_system_events').prop('disabled', false);
  });

  $('#btn_submit_system_events').off().on('click', function () {
    requestWriteConfig(conf_editor_systemEvents.getValue());
  });

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_system_events_intro'), "editor_container_system_events");
  }
  
  removeOverlay();
});

