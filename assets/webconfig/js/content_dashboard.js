$(document).ready(function () {
  performTranslation();

  function updateComponents() {
    $("div[class*='currentInstance']").remove();

    var instances_html = '<div class="col-md-6 col-xxl-4 currentInstance-"><div class="panel panel-default">';
    instances_html += '<div class="panel-heading panel-instance">';
    instances_html += '<div class="dropdown">';
    instances_html += '<a id="active_instance_dropdown" class="dropdown-toggle" data-toggle="dropdown" href="#" style="text-decoration:none;display:flex;align-items:center;">';
    instances_html += '<div id="active_instance_friendly_name"></div>';
    instances_html += '<div id="btn_hypinstanceswitch" style="white-space:nowrap;"><span class="mdi mdi-lightbulb-group mdi-24px" style="margin-right:0;margin-left:5px;"></span><span class="mdi mdi-menu-down mdi-24px"></span></div>';
    instances_html += '</a><ul id="hyp_inst_listing" class="dropdown-menu dropdown-alerts" style="cursor:pointer;"></ul>';
    instances_html += '</div></div>';

    instances_html += '<div class="panel-body">';
    instances_html += '<table class="table borderless">';
    instances_html += '<thead><tr><th style="vertical-align:middle"><i class="mdi mdi-lightbulb-on fa-fw"></i>';
    instances_html += '<span>' + $.i18n('dashboard_componentbox_label_status') + '</span></th>';

    var components = window.comps;
    var hyperion_enabled = true;
    components.forEach(function (obj) {
      if (obj.name == "ALL") {
        hyperion_enabled = obj.enabled;
      }
    });

    var instBtn = '<span style="display:block; margin:3px"><input id="instanceButton"'
      + (hyperion_enabled ? "checked" : "") + ' type="checkbox" data-toggle="toggle" data-size="small" data-onstyle="success" data-on="'
      + $.i18n('general_btn_on') + '" data-off="'
      + $.i18n('general_btn_off') + '"></span>';

    instances_html += '<th style="width:1px;text-align:right">' + instBtn + '</th></tr></thead></table>';

    instances_html += '<table class="table borderless">';
    instances_html += '<thead><tr><th colspan="3">';
    instances_html += '<i class="fa fa-info-circle fa-fw"></i>';
    instances_html += '<span>' + $.i18n('dashboard_infobox_label_title') + '</span>';
    instances_html += '</th></tr></thead>';
    instances_html += '<tbody>';
    instances_html += '<tr><td></td><td>' + $.i18n('conf_leds_contr_label_contrtype') + '</td>';
    instances_html += '<td style="text-align:right; padding-right:0">';
    instances_html += '<span>' + window.serverConfig.device.type + '</span>';
    instances_html += '<a class="fa fa-cog fa-fw" onclick="SwitchToMenuItem(\'MenuItemLeds\')" style="text-decoration:none;cursor:pointer"></a>';
    instances_html += '</td></tr>';
    instances_html += '</tbody></table>';

    instances_html += '<table class="table first_cell_borderless">';
    instances_html += '<thead><tr><th colspan="3">';
    instances_html += '<i class="fa fa-eye fa-fw"></i>';
    instances_html += '<span>' + $.i18n('dashboard_componentbox_label_title') + '</span>';
    instances_html += '</th></tr></thead>';

    var componentBtn = "";
    var instance_components = "";
    for (var idx = 0; idx < components.length; idx++) {
      if (components[idx].name != "ALL") {
        if ((components[idx].name === "FORWARDER" && window.currentHyperionInstance != 0) ||
          (components[idx].name === "GRABBER" && !window.serverConfig.framegrabber.enable) ||
          (components[idx].name === "V4L" && !window.serverConfig.grabberV4L2.enable))
          continue;

        var comp_enabled = components[idx].enabled ? "checked" : "";
        const general_comp = "general_comp_" + components[idx].name;
        componentBtn = '<input ' +
          'id="' + general_comp + '" ' + comp_enabled +
          ' type="checkbox" ' +
          'data-toggle="toggle" ' +
          'data-size="mini" ' +
          'data-onstyle="success" ' +
          'data-on="' + $.i18n('general_btn_on') + '" ' +
          'data-off="' + $.i18n('general_btn_off') + '">';

        instance_components += '<tr><td></td><td>' + $.i18n('general_comp_' + components[idx].name) + '</td><td style="text-align:right">' + componentBtn + '</td></tr>';
      }
    }

    instances_html += '<tbody>' + instance_components + '</tbody></table>';
    instances_html += '</div></div></div>';

    $('.instances').prepend(instances_html);

    updateUiOnInstance(window.currentHyperionInstance);
    updateHyperionInstanceListing();

    $('#instanceButton').bootstrapToggle();
    $('#instanceButton').on("change", e => {
      requestSetComponentState('ALL', e.currentTarget.checked);
    });

    for (var idx = 0; idx < components.length; idx++) {
      if (components[idx].name != "ALL") {
        $("#general_comp_" + components[idx].name).bootstrapToggle();
        $("#general_comp_" + components[idx].name).bootstrapToggle(hyperion_enabled ? "enable" : "disable");
        $("#general_comp_" + components[idx].name).on("change", e => {
          requestSetComponentState(e.currentTarget.id.split('_')[2], e.currentTarget.checked);
        });
      }
    }
  }

  // add more info

  var screenGrabberAvailable = (window.serverInfo.grabbers.screen.available.length !== 0);
  var videoGrabberAvailable = (window.serverInfo.grabbers.video.available.length !== 0);

  if (screenGrabberAvailable || videoGrabberAvailable) {

    if (screenGrabberAvailable) {
      var screenGrabber = window.serverConfig.framegrabber.enable ? $.i18n('general_enabled') : $.i18n('general_disabled');
      $('#dash_screen_grabber').html(screenGrabber);
    } else {
      $("#dash_screen_grabber_row").hide();
    }

    if (videoGrabberAvailable) {
      var videoGrabber = window.serverConfig.grabberV4L2.enable ? $.i18n('general_enabled') : $.i18n('general_disabled');
      $('#dash_video_grabber').html(videoGrabber);
    } else {
      $("#dash_video_grabber_row").hide();
    }
  } else {
    $("#dash_capture_hw").hide();
  }

  if (jQuery.inArray("flatbuffer", window.serverInfo.services) !== -1) {
    var fbPort = window.serverConfig.flatbufServer.enable ? window.serverConfig.flatbufServer.port : $.i18n('general_disabled');
    $('#dash_fbPort').html(fbPort);
  } else {
    $("#dash_ports_flat_row").hide();
  }

  if (jQuery.inArray("protobuffer", window.serverInfo.services) !== -1) {
    var pbPort = window.serverConfig.protoServer.enable ? window.serverConfig.protoServer.port : $.i18n('general_disabled');
    $('#dash_pbPort').html(pbPort);
  } else {
    $("#dash_ports_proto_row").hide();
  }

  if (jQuery.inArray("boblight", window.serverInfo.services) !== -1) {
    var boblightPort = window.serverConfig.boblightServer.enable ? window.serverConfig.boblightServer.port : $.i18n('general_disabled');
    $('#dash_boblightPort').html(boblightPort);
  } else {
    $("#dash_ports_boblight_row").hide();
  }

  var jsonPort = window.serverConfig.jsonServer.port;
  $('#dash_jsonPort').html(jsonPort);
  var wsPorts = window.serverConfig.webConfig.port + ' | ' + window.serverConfig.webConfig.sslPort;
  $('#dash_wsPorts').html(wsPorts);

  $('#dash_currv').html(window.currentVersion);
  $('#dash_watchedversionbranch').html(window.serverConfig.general.watchedVersionBranch);

  getReleases(function (callback) {
    if (callback) {
      $('#dash_latev').html(window.latestVersion.tag_name);

      if (semverLite.gt(window.latestVersion.tag_name, window.currentVersion))
        $('#versioninforesult').html('<div class="bs-callout bs-callout-warning" style="margin:0px"><a target="_blank" href="' + window.latestVersion.html_url + '">' + $.i18n('dashboard_infobox_message_updatewarning', window.latestVersion.tag_name) + '</a></div>');
      else
        $('#versioninforesult').html('<div class="bs-callout bs-callout-info" style="margin:0px">' + $.i18n('dashboard_infobox_message_updatesuccess') + '</div>');
    }
  });

  //interval update
  updateComponents();
  $(window.hyperion).on("components-updated", updateComponents);

  if (window.showOptHelp)
    createHintH("intro", $.i18n('dashboard_label_intro'), "dash_intro");

  removeOverlay();
});
