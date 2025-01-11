$(document).ready(function () {
  performTranslation();

  function updateInstanceComponents() {

    let instanceRunningStatus = isInstanceRunning(window.currentHyperionInstance);
    let isInstanceEnabled = false;
    const components = window.comps;

    if (instanceRunningStatus) {
      isInstanceEnabled = components.some(obj => obj.name === "ALL" && obj.enabled);
    }

    // Generate instance status button
    let instBtn = `
  <span style="display:block; margin:3px">
    <input id="instanceButton" 
      type="checkbox" 
      data-toggle="toggle" 
      data-size="small" 
      data-onstyle="success" 
      data-on="${$.i18n('general_btn_on')}" 
      data-off="${$.i18n('general_btn_off')}" 
      ${isInstanceEnabled ? "checked" : ""}
      ${instanceRunningStatus ? "" : "disabled"}>
  </span>
`;

    // Remove existing instance elements
    $("div[class*='currentInstance']").remove();

    // Start constructing the HTML for instances
    let instances_html = `
  <div class="col-md-6 col-xxl-4 currentInstance-">
    <div class="panel panel-default">
      <div class="panel-heading panel-instance">
        <div class="dropdown">
          <a id="active_instance_dropdown" 
             class="dropdown-toggle" 
             data-toggle="dropdown" 
             href="#" 
             style="text-decoration:none; display:flex; align-items:center;">
            <div id="active_instance_friendly_name"></div>
            <div id="btn_hypinstanceswitch" style="white-space:nowrap;">
              <span class="mdi mdi-lightbulb-group mdi-24px" style="margin-right:0; margin-left:5px;"></span>
              <span class="mdi mdi-menu-down mdi-24px"></span>
            </div>
          </a>
          <ul id="hyp_inst_listing" 
              class="dropdown-menu dropdown-alerts" 
              style="cursor:pointer;">
          </ul>
        </div>
      </div>
      <div class="panel-body">
        <table class="table borderless">
          <thead>
            <tr>
              <th style="vertical-align:middle">
                <i class="mdi mdi-lightbulb-on fa-fw"></i>
                <span>${$.i18n('dashboard_componentbox_label_status')}</span>
              </th>
              <th style="width:1px; text-align:right">
                ${instBtn}
              </th>
              </tr>
          </thead>
      </table>              
`;

    instances_html += `
  <table class="table borderless">
    <thead>
      <tr>
        <th colspan="3">
          <i class="fa fa-info-circle fa-fw"></i>
          <span>${$.i18n('dashboard_infobox_label_title')}</span>
        </th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td></td>
        <td>${$.i18n('conf_leds_contr_label_contrtype')}</td>
        <td style="text-align:right; padding-right:0">
          <span>${window.serverConfig.device.type}</span>
          <a class="fa fa-cog fa-fw" 
             onclick="SwitchToMenuItem('MenuItemInstLeds')" 
             style="text-decoration:none; cursor:pointer">
          </a>
        </td>
      </tr>
    </tbody>
  </table>
`;

    // If the current instance is running, add components table
    if (instanceRunningStatus) {
      instances_html += `
    <table class="table first_cell_borderless">
      <thead>
        <tr>
          <th colspan="3">
            <i class="fa fa-eye fa-fw"></i>
            <span>${$.i18n('dashboard_componentbox_label_title')}</span>
          </th>
        </tr>
      </thead>
  `;

      // Initialize components table body
      let instance_components = "";

      for (const element of components) {
        const componentName = element.name;

        // Skip unwanted components
        if (componentName === "ALL" ||
          (componentName === "FORWARDER" && window.currentHyperionInstance != 0) ||
          (componentName === "GRABBER" && !window.serverConfig.framegrabber.enable) ||
          (componentName === "V4L" && !window.serverConfig.grabberV4L2.enable) ||
          (componentName === "AUDIO" && !window.serverConfig.grabberAudio.enable)) {
          continue;
        }

        // Determine if the component is enabled
        const comp_enabled = element.enabled ? "checked" : "";
        const general_comp = `general_comp_${componentName}`;

        // Create the toggle button for the component
        const componentBtn = `
      <input 
        id="${general_comp}" 
        type="checkbox" 
        data-toggle="toggle" 
        data-size="mini" 
        data-onstyle="success" 
        data-on="${$.i18n('general_btn_on')}" 
        data-off="${$.i18n('general_btn_off')}" 
        ${comp_enabled}>
    `;

        // Add row for the component
        instance_components += `
      <tr>
        <td></td>
        <td>${$.i18n('general_comp_' + componentName)}</td>
        <td style="text-align:right">${componentBtn}</td>
      </tr>
    `;
      }

      // Close components table
      instances_html += `
      <tbody>${instance_components}</tbody>
    </table>
  `;
    }

    // Close the container divs
    instances_html += `
  </div>
</div>
</div>
`;

    // Prepend the instances HTML to the DOM
    $('.instances').prepend(instances_html);

    //Replace buttons by Bootstrap ones
    $('#instanceButton').bootstrapToggle();
    if (instanceRunningStatus) {

      $('#instanceButton').change(function () {
        requestSetComponentState('ALL', $(this).prop('checked'))
      });

      for (const element of components) {
        const componentName = element.name;
        if (componentName !== "ALL") {
          $("#general_comp_" + componentName).bootstrapToggle();
          $("#general_comp_" + componentName).bootstrapToggle(isInstanceEnabled ? "enable" : "disable");

          $("#general_comp_" + componentName).change(function () {
            const componentName = componentName;
            const isChecked = $(this).prop('checked');
            requestSetComponentState(componentName, isChecked);
          });
        }
      }
    }
    updateUiOnInstance(window.currentHyperionInstance);
    updateHyperionInstanceListing();

  }

  function updateGlobalComponents() {

    // add more info
    const screenGrabberAvailable = (window.serverInfo.grabbers.screen.available.length !== 0);
    const videoGrabberAvailable = (window.serverInfo.grabbers.video.available.length !== 0);
    const audioGrabberAvailable = (window.serverInfo.grabbers.audio.available.length !== 0);

    if (screenGrabberAvailable || videoGrabberAvailable || audioGrabberAvailable) {

      if (screenGrabberAvailable) {
        const screenGrabber = window.serverConfig.framegrabber.enable ? $.i18n('general_enabled') : $.i18n('general_disabled');
        $('#dash_screen_grabber').html(screenGrabber);
      } else {
        $("#dash_screen_grabber_row").hide();
      }

      if (videoGrabberAvailable) {
        const videoGrabber = window.serverConfig.grabberV4L2.enable ? $.i18n('general_enabled') : $.i18n('general_disabled');
        $('#dash_video_grabber').html(videoGrabber);
      } else {
        $("#dash_video_grabber_row").hide();
      }

      if (audioGrabberAvailable) {
        const audioGrabber = window.serverConfig.grabberAudio.enable ? $.i18n('general_enabled') : $.i18n('general_disabled');
        $('#dash_audio_grabber').html(audioGrabber);
      } else {
        $("#dash_audio_grabber_row").hide();
      }
    } else {
      $("#dash_capture_hw").hide();
    }

    if (jQuery.inArray("flatbuffer", window.serverInfo.services) !== -1) {
      const fbPort = window.serverConfig.flatbufServer.enable ? window.serverConfig.flatbufServer.port : $.i18n('general_disabled');
      $('#dash_fbPort').html(fbPort);
    } else {
      $("#dash_ports_flat_row").hide();
    }

    if (jQuery.inArray("protobuffer", window.serverInfo.services) !== -1) {
      const pbPort = window.serverConfig.protoServer.enable ? window.serverConfig.protoServer.port : $.i18n('general_disabled');
      $('#dash_pbPort').html(pbPort);
    } else {
      $("#dash_ports_proto_row").hide();
    }

    if (jQuery.inArray("boblight", window.serverInfo.services) !== -1) {
      const boblightPort = window.serverConfig.boblightServer.enable ? window.serverConfig.boblightServer.port : $.i18n('general_disabled');
      $('#dash_boblightPort').html(boblightPort);
    } else {
      $("#dash_ports_boblight_row").hide();
    }

    const jsonPort = window.serverConfig.jsonServer.port;
    $('#dash_jsonPort').html(jsonPort);
    const wsPorts = window.serverConfig.webConfig.port + ' | ' + window.serverConfig.webConfig.sslPort;
    $('#dash_wsPorts').html(wsPorts);


    $('#dash_currv').html(window.currentVersion);
    $('#dash_watchedversionbranch').html(window.serverConfig.general.watchedVersionBranch);

    getReleases(function (callback) {
      if (callback) {
        $('#dash_latev').html(window.latestVersion.tag_name);

        if (semverLite.gt(window.latestVersion.tag_name, window.currentVersion)) {
          $('#versioninforesult').html(
            `<div class="bs-callout bs-callout-warning" style="margin:0px">
           <a target="_blank" href="${window.latestVersion.html_url}">
             ${$.i18n('dashboard_infobox_message_updatewarning', window.latestVersion.tag_name)}
           </a>
         </div>`
          );
        } else {
          $('#versioninforesult').html(
            `<div class="bs-callout bs-callout-info" style="margin:0px">
           ${$.i18n('dashboard_infobox_message_updatesuccess')}
         </div>`
          );
        }
      }
    });
  }
  function updateDashboard() {

    //Only show an instance, if minimum one configured
    if (window.serverInfo.instance.length !== 0) {
      updateInstanceComponents();
    }
    updateGlobalComponents();
  }

  updateDashboard();

  $(window.hyperion).on("components-updated", updateDashboard);

  if (window.showOptHelp) {
    createHintH("intro", $.i18n('dashboard_label_intro'), "dash_intro");
  }

  removeOverlay();
});

