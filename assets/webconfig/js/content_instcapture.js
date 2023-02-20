$(document).ready(function () {
  performTranslation();

  var screenGrabberAvailable = (window.serverInfo.grabbers.screen.available.length !== 0);
  var videoGrabberAvailable = (window.serverInfo.grabbers.video.available.length !== 0);
  const audioGrabberAvailable = (window.serverInfo.grabbers.audio.available.length !== 0);

  var BOBLIGHT_ENABLED = (jQuery.inArray("boblight", window.serverInfo.services) !== -1);

  // update instance listing
  updateHyperionInstanceListing();

  var conf_editor_instCapt = null;
  var conf_editor_bobl = null;

  // Instance Capture

  if (window.showOptHelp) {
    if (screenGrabberAvailable || videoGrabberAvailable || audioGrabberAvailable) {
      $('#conf_cont').append(createRow('conf_cont_instCapt'));
      $('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt', ''));
      $('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));
    }
    //boblight
    if (BOBLIGHT_ENABLED) {
      $('#conf_cont').append(createRow('conf_cont_bobl'));
      $('#conf_cont_bobl').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver', ''));
      $('#conf_cont_bobl').append(createHelpTable(window.schema.boblightServer.properties, $.i18n("edt_conf_bobls_heading_title"), "boblightServerHelpPanelId"));
    }
  }
  else {
    $('#conf_cont').addClass('row');
    if (screenGrabberAvailable || videoGrabberAvailable || audioGrabberAvailable) {
      $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt', ''));
    }
    if (BOBLIGHT_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-sitemap', $.i18n("edt_conf_bobls_heading_title"), 'editor_container_boblightserver', 'btn_submit_boblightserver', ''));
    }
  }

  if (screenGrabberAvailable || videoGrabberAvailable || audioGrabberAvailable) {

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

      if (screenGrabberAvailable) {
        if (!window.serverConfig.framegrabber.enable) {
          conf_editor_instCapt.getEditor("root.instCapture.systemEnable").setValue(false);
          conf_editor_instCapt.getEditor("root.instCapture.systemEnable").disable();
        }
        else {
          conf_editor_instCapt.getEditor("root.instCapture.systemEnable").setValue(window.serverConfig.instCapture.systemEnable);
        }
      } else {
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "systemEnable", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "systemGrabberDevice", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "systemPriority", false);
      }

      if (videoGrabberAvailable) {
        if (!window.serverConfig.grabberV4L2.enable) {
          conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").setValue(false);
          conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").disable();
        }
        else {
          conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").setValue(window.serverConfig.instCapture.v4lEnable);

        }
      } else {
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "v4lGrabberDevice", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "v4lEnable", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "v4lPriority", false);
      }

      if (audioGrabberAvailable) {
        if (!window.serverConfig.grabberAudio.enable) {
          conf_editor_instCapt.getEditor("root.instCapture.audioEnable").setValue(false);
          conf_editor_instCapt.getEditor("root.instCapture.audioEnable").disable();
        }
        else {
          conf_editor_instCapt.getEditor("root.instCapture.audioEnable").setValue(window.serverConfig.instCapture.audioEnable);

        }
      } else {
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "audioGrabberDevice", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "audioEnable", false);
        showInputOptionForItem(conf_editor_instCapt, "instCapture", "audioPriority", false);
      }

    });

    conf_editor_instCapt.on('change', function () {

      if (!conf_editor_instCapt.validate().length) {
        if (!window.serverConfig.framegrabber.enable &&
          !window.serverConfig.grabberV4L2.enable &&
          !window.serverConfig.grabberAudio.enable) {
          $('#btn_submit_instCapt').prop('disabled', true);
        } else {
          window.readOnlyMode ? $('#btn_submit_instCapt').prop('disabled', true) : $('#btn_submit_instCapt').prop('disabled', false);
        }
      }
      else {
        $('#btn_submit_instCapt').prop('disabled', true);
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

    conf_editor_instCapt.watch('root.instCapture.audioEnable', () => {
      const audioEnable = conf_editor_instCapt.getEditor("root.instCapture.audioEnable").getValue();
      if (audioEnable) {
        conf_editor_instCapt.getEditor("root.instCapture.audioGrabberDevice").setValue(window.serverConfig.grabberAudio.available_devices);
        conf_editor_instCapt.getEditor("root.instCapture.audioGrabberDevice").disable();
        showInputOptions("instCapture", ["audioGrabberDevice"], true);
        showInputOptions("instCapture", ["audioPriority"], true);
      }
      else {
        if (!window.serverConfig.grabberAudio.enable) {
          conf_editor_instCapt.getEditor("root.instCapture.audioEnable").disable();
        }
        showInputOptions("instCapture", ["audioGrabberDevice"], false);
        showInputOptions("instCapture", ["audioPriority"], false);
      }
    });

    $('#btn_submit_instCapt').off().on('click', function () {
      requestWriteConfig(conf_editor_instCapt.getValue());
    });
  }

  //boblight
  if (BOBLIGHT_ENABLED) {
    conf_editor_bobl = createJsonEditor('editor_container_boblightserver', {
      boblightServer: window.schema.boblightServer
    }, true, true);

    conf_editor_bobl.on('ready', function () {
      var boblightServerEnable = conf_editor_bobl.getEditor("root.boblightServer.enable").getValue();
      if (!boblightServerEnable) {
        showInputOptionsForKey(conf_editor_bobl, "boblightServer", "enable", false);
        $('#boblightServerHelpPanelId').hide();
      }
    });

    conf_editor_bobl.on('change', function () {
      conf_editor_bobl.validate().length || window.readOnlyMode ? $('#btn_submit_boblightserver').prop('disabled', true) : $('#btn_submit_boblightserver').prop('disabled', false);
    });

    conf_editor_bobl.watch('root.boblightServer.enable', () => {
      var boblightServerEnable = conf_editor_bobl.getEditor("root.boblightServer.enable").getValue();
      if (boblightServerEnable) {
        //Make port instance specific, if port is still the default one (avoids overlap of used ports)
        var port = conf_editor_bobl.getEditor("root.boblightServer.port").getValue();
        if (port === conf_editor_bobl.schema.properties.boblightServer.properties.port.default) {
          port += parseInt(window.currentHyperionInstance);
        }
        conf_editor_bobl.getEditor("root.boblightServer.port").setValue(port);

        showInputOptionsForKey(conf_editor_bobl, "boblightServer", "enable", true);
        $('#boblightServerHelpPanelId').show();
      } else {
        showInputOptionsForKey(conf_editor_bobl, "boblightServer", "enable", false);
        $('#boblightServerHelpPanelId').hide();
      }
    });

    $('#btn_submit_boblightserver').off().on('click', function () {
      requestWriteConfig(conf_editor_bobl.getValue());
    });
  }

  //create introduction
  if (window.showOptHelp) {
    if (BOBLIGHT_ENABLED) {
      createHint("intro", $.i18n('conf_network_bobl_intro'), "editor_container_boblightserver");
    }
  }

  removeOverlay();
});
