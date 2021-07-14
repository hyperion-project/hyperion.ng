$(document).ready(function () {
  performTranslation();

  // update instance listing
  updateHyperionInstanceListing();

  var conf_editor_instCapt = null;

  // Instance Capture
  $('#conf_cont').append(createRow('conf_cont_instCapt'));
  $('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt', ''));
  if (window.showOptHelp) {
    $('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));
  }

  // Instance Capture
  conf_editor_instCapt = createJsonEditor('editor_container_instCapt', {
    instCapture: window.schema.instCapture
  }, true, true);

  var grabber_config_info_html = '<div class="bs-callout bs-callout-info" style="margin-top:0px"><h4>' + $.i18n('dashboard_infobox_label_title') + '</h4 >';
  grabber_config_info_html += '<span>' + $.i18n('conf_grabber_inst_grabber_config_info') + '</span>';
  grabber_config_info_html += '<a class="fa fa-cog fa-fw" onclick="SwitchToMenuItem(\'MenuItemGrabber\')" style="text-decoration:none;cursor:pointer"></a>';
  grabber_config_info_html += '</div>';
  $('#editor_container_instCapt').append(grabber_config_info_html);

  conf_editor_instCapt.on('ready', function () {

    if (!window.serverConfig.framegrabber.enable) {
      conf_editor_instCapt.getEditor("root.instCapture.systemEnable").setValue(false);
      conf_editor_instCapt.getEditor("root.instCapture.systemEnable").disable();
    }
    else {
      conf_editor_instCapt.getEditor("root.instCapture.systemEnable").setValue(window.serverConfig.instCapture.systemEnable);
    }

    if (!window.serverConfig.grabberV4L2.enable) {
      conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").setValue(false);
      conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").disable();
    }
    else {
      conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").setValue(window.serverConfig.instCapture.v4lEnable);
    }

  });

  conf_editor_instCapt.on('change', function () {

    if (!conf_editor_instCapt.validate().length) {
      if (!window.serverConfig.framegrabber.enable && !window.serverConfig.grabberV4L2.enable) {
        $('#btn_submit_instCapt').attr('disabled', true);
      } else {
        window.readOnlyMode ? $('#btn_submit_instCapt').attr('disabled', true) : $('#btn_submit_instCapt').attr('disabled', false);
      }
    }
    else {
      $('#btn_submit_instCapt').attr('disabled', true);
    }
  });

  conf_editor_instCapt.watch('root.instCapture.systemEnable', () => {

    var screenEnable = conf_editor_instCapt.getEditor("root.instCapture.systemEnable").getValue();
    if (screenEnable) {
      conf_editor_instCapt.getEditor("root.instCapture.systemGrabberDevice").setValue(window.serverConfig.framegrabber.available_devices);
      conf_editor_instCapt.getEditor("root.instCapture.systemGrabberDevice").disable();
      showInputOptions("instCapture", ["systemGrabberDevice"], true);
      showInputOptions("instCapture", ["systemPriority"], true);

    } else {
      showInputOptions("instCapture", ["systemGrabberDevice"], false);
      showInputOptions("instCapture", ["systemPriority"], false);
    }

  });

  conf_editor_instCapt.watch('root.instCapture.v4lEnable', () => {
    var videoEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
    if (videoEnable) {
      conf_editor_instCapt.getEditor("root.instCapture.v4lGrabberDevice").setValue(window.serverConfig.grabberV4L2.available_devices);
      conf_editor_instCapt.getEditor("root.instCapture.v4lGrabberDevice").disable();
      showInputOptions("instCapture", ["v4lGrabberDevice"], true);
      showInputOptions("instCapture", ["v4lPriority"], true);
    }
    else {
      if (!window.serverConfig.grabberV4L2.enable) {
        conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").disable();
      }
      showInputOptions("instCapture", ["v4lGrabberDevice"], false);
      showInputOptions("instCapture", ["v4lPriority"], false);
    }
  });

  $('#btn_submit_instCapt').off().on('click', function () {
    requestWriteConfig(conf_editor_instCapt.getValue());
  });

  removeOverlay();
});
