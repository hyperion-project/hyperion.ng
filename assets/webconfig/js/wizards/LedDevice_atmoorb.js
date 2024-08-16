//****************************
// Wizard AtmoOrb
//****************************

import { ledDeviceWizardUtils as utils } from './LedDevice_utils.js';

const atmoorbWizard = (() => {

  const lights = [];
  let configuredLights = [];

  function getIdInLights(id) {
    return lights.filter(
      function (lights) {
        return lights.id === id
      }
    );
  }

  function begin() {

    const configruedOrbIds = conf_editor.getEditor("root.specificOptions.orbIds").getValue().trim();
    if (configruedOrbIds.length !== 0) {
      configuredLights = configruedOrbIds.split(",").map(Number);
    }

    const multiCastGroup = conf_editor.getEditor("root.specificOptions.host").getValue();
    const multiCastPort = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());

    discover(multiCastGroup, multiCastPort);

    $('#btn_wiz_save').off().on("click", function () {
      let ledConfig = [];
      let finalLights = [];

      //create atmoorb led config
      for (let key in lights) {
        if ($('#orb_' + key).val() !== "disabled") {
          // Set Name to layout-position, if empty
          if (lights[key].name === "") {
            lights[key].name = $.i18n('conf_leds_layout_cl_' + $('#orb_' + key).val());
          }

          finalLights.push(lights[key].id);

          let name = lights[key].id;
          if (lights[key].host !== "")
            name += ':' + lights[key].host;

          const idx_content = utils.assignLightPos($('#orb_' + key).val(), name);
          ledConfig.push(JSON.parse(JSON.stringify(idx_content)));
        }
      }

      //LED layout
      window.serverConfig.leds = ledConfig;

      //LED device config
      //Start with a clean configuration
      let d = {};

      d.type = 'atmoorb';
      d.hardwareLedCount = finalLights.length;
      d.colorOrder = conf_editor.getEditor("root.generalOptions.colorOrder").getValue();

      d.orbIds = finalLights.toString();
      d.useOrbSmoothing = utils.eV("useOrbSmoothing");

      d.host = conf_editor.getEditor("root.specificOptions.host").getValue();
      d.port = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());
      d.latchTime = parseInt(conf_editor.getEditor("root.specificOptions.latchTime").getValue());;

      window.serverConfig.device = d;

      requestWriteConfig(window.serverConfig, true);
      resetWizard();
    });

    $('#btn_wiz_abort').off().on('click', resetWizard);
  }

  async function discover(multiCastGroup, multiCastPort) {
    let params = {};
    if (multiCastGroup !== "") {
      params.multiCastGroup = multiCastGroup;
    }

    if (multiCastPort !== 0) {
      params.multiCastPort = multiCastPort;
    }

    // Get discovered lights
    const res = await requestLedDeviceDiscovery('atmoorb', params);
    if (res && !res.error) {
      const r = res.info;

      // Process devices returned by discovery
      processDiscoveredDevices(r.devices);

      // Add additional items from configuration
      for (const configuredLight of configuredLights) {
        processConfiguredLight(configuredLight);
      }

      sortLightsById();
      assign_lights();
    }
  }

  function processDiscoveredDevices(devices) {
    for (const device of devices) {
      if (device.id !== "" && getIdInLights(device.id).length === 0) {
        const light = {
          id: device.id,
          ip: device.ip,
          host: device.hostname
        };
        lights.push(light);
      }
    }
  }

  function processConfiguredLight(configuredLight) {
    if (configuredLight !== "" && !isNaN(configuredLight)) {
      if (getIdInLights(configuredLight).length === 0) {
        const light = {
          id: configuredLight,
          ip: "",
          host: ""
        };
        lights.push(light);
      }
    }
  }

  function attachIdentifyButtonEvent() {
    // Use event delegation to handle clicks on buttons with class "btn-identify"
    $('#wizp2_body').on('click', '.btn-identify', function () {
      const orbId = $(this).data('orb-id');
      identify(orbId);
    });
  }

  function sortLightsById() {
    lights.sort((a, b) => (a.id > b.id) ? 1 : -1);
  }

  function assign_lights() {
    // If records are left for configuration
    if (Object.keys(lights).length > 0) {
      $('#wh_topcontainer').toggle(false);
      $('#orb_ids_t, #btn_wiz_save').toggle(true);

      const lightOptions = utils.getLayoutPositions();
      lightOptions.unshift("disabled");

      $('.lidsb').html("");
      let pos = "";

      for (const lightid in lights) {
        const orbId = lights[lightid].id;
        const orbIp = lights[lightid].ip;
        let orbHostname = lights[lightid].host;

        if (orbHostname === "")
          orbHostname = $.i18n('edt_dev_spec_lights_itemtitle');

        let options = "";
        for (const opt in lightOptions) {
          const val = lightOptions[opt];
          options += '<option value="' + val + '"';
          if (pos === val) options += ' selected="selected"';
          options += '>' + $.i18n('conf_leds_layout_cl_' + val) + '</option>';
        }

        let enabled = 'enabled';
        if (orbId < 1 || orbId > 255) {
          enabled = 'disabled';
          options = '<option value=disabled>' + $.i18n('wiz_atmoorb_unsupported') + '</option>';
        }

        let lightAnnotation = "";
        if (orbIp !== "") {
          lightAnnotation = ': ' + orbIp + '<br>(' + orbHostname + ')';
        }

        $('.lidsb').append(createTableRow([orbId + lightAnnotation, '<select id="orb_' + lightid + '" ' + enabled + ' class="orb_sel_watch form-control">'
          + options
          + '</select>', '<button class="btn btn-sm btn-primary btn-identify" ' + enabled + ' data-orb-id="' + orbId + '")>'
          + $.i18n('wiz_identify_light', orbId) + '</button>']));
      }
      attachIdentifyButtonEvent();

      $('.orb_sel_watch').on("change", function () {
        let cC = 0;
        for (const key in lights) {
          if ($('#orb_' + key).val() !== "disabled") {
            cC++;
          }
        }
        if (cC === 0 || window.readOnlyMode)
          $('#btn_wiz_save').prop("disabled", true);
        else
          $('#btn_wiz_save').prop("disabled", false);
      });
      $('.orb_sel_watch').trigger('change');
    }
    else {
      const noLightsTxt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_noLights', 'AtmoOrbs') + '</p>';
      $('#wizp2_body').append(noLightsTxt);
    }


  }

  async function identify(orbId) {
    const disabled = $('#btn_wiz_save').is(':disabled');

    // Take care that new record cannot be save during background process
    $('#btn_wiz_save').prop('disabled', true);

    const params = { id: orbId };
    await requestLedDeviceIdentification("atmoorb", params);

    if (!window.readOnlyMode) {
      $('#btn_wiz_save').prop('disabled', disabled);
    }
  }

  return {
    start: function (e) {

      //create html
      const atmoorb_title = 'wiz_atmoorb_title';
      const atmoorb_intro1 = 'wiz_atmoorb_intro1';

      $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(atmoorb_title));
      $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(atmoorb_title) + '</h4><p>' + $.i18n(atmoorb_intro1) + '</p>');

      $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'
        + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'
        + $.i18n('general_btn_cancel') + '</button>');

      $('#wizp2_body').html('<div id="wh_topcontainer"></div>');

      $('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

      $('#wizp2_body').append('<div id="orb_ids_t" style="display:none"><p style="font-weight:bold" id="orb_id_headline">' + $.i18n('wiz_atmoorb_desc2') + '</p></div>');

      createTable("lidsh", "lidsb", "orb_ids_t");
      $('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lights_title'), $.i18n('wiz_pos'), $.i18n('wiz_identify')], true));
      $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'
        + $.i18n('general_btn_save') + '</button><buttowindow.serverConfig.device = d;n type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'
        + $.i18n('general_btn_cancel') + '</button>');

      if (getStorage("darkMode") == "on")
        $('#wizard_logo').attr("src", 'img/hyperion/logo_negativ.png');

      //open modal
      $("#wizard_modal").modal({ backdrop: "static", keyboard: false, show: true });

      //listen for continue
      $('#btn_wiz_cont').off().on('click', function () {
        begin();
        $('#wizp1').toggle(false);
        $('#wizp2').toggle(true);
      });
    }
  };
})();

export { atmoorbWizard };
