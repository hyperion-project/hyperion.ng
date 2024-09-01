//****************************
// Wizard Yeelight
//****************************

import { ledDeviceWizardUtils as utils } from './LedDevice_utils.js';

const yeelightWizard = (() => {

  const lights = [];
  let configuredLights = conf_editor.getEditor("root.specificOptions.lights").getValue();

  function getHostInLights(hostname) {
    return lights.filter(
      function (lights) {
        return lights.host === hostname
      }
    );
  }

  function begin() {
    discover();

    $('#btn_wiz_save').off().on("click", function () {
      let ledConfig = [];
      let finalLights = [];

      //create yeelight led config
      for (const key in lights) {
        if ($('#yee_' + key).val() !== "disabled") {

          let name = lights[key].name;
          // Set Name to layout-position, if empty
          if (name === "") {
            name = lights[key].host;
          }

          finalLights.push(lights[key]);

          const idx_content = utils.assignLightPos($('#yee_' + key).val(), name);
          ledConfig.push(JSON.parse(JSON.stringify(idx_content)));
        }
      }

      //LED layout
      window.serverConfig.leds = ledConfig;

      //LED device config
      const currentDeviceType = window.serverConfig.device.type;

      //Start with a clean configuration
      let d = {};

      d.type = 'yeelight';
      d.hardwareLedCount = finalLights.length;
      d.colorOrder = conf_editor.getEditor("root.generalOptions.colorOrder").getValue();
      d.colorModel = parseInt(conf_editor.getEditor("root.specificOptions.colorModel").getValue());

      d.transEffect = parseInt(conf_editor.getEditor("root.specificOptions.transEffect").getValue());
      d.transTime = parseInt(conf_editor.getEditor("root.specificOptions.transTime").getValue());
      d.extraTimeDarkness = parseInt(conf_editor.getEditor("root.specificOptions.extraTimeDarkness").getValue());

      d.brightnessMin = parseInt(conf_editor.getEditor("root.specificOptions.brightnessMin").getValue());
      d.brightnessSwitchOffOnMinimum = JSON.parse(conf_editor.getEditor("root.specificOptions.brightnessSwitchOffOnMinimum").getValue());
      d.brightnessMax = parseInt(conf_editor.getEditor("root.specificOptions.brightnessMax").getValue());
      d.brightnessFactor = parseFloat(conf_editor.getEditor("root.specificOptions.brightnessFactor").getValue());

      d.latchTime = parseInt(conf_editor.getEditor("root.specificOptions.latchTime").getValue());;
      d.debugLevel = parseInt(conf_editor.getEditor("root.specificOptions.debugLevel").getValue());

      d.lights = finalLights;

      window.serverConfig.device = d;

      if (currentDeviceType !== d.type) {
        //smoothing off, if new device
        window.serverConfig.smoothing = { enable: false };
      }

      requestWriteConfig(window.serverConfig, true);
      resetWizard();
    });

    $('#btn_wiz_abort').off().on('click', resetWizard);
  }

  async function discover() {
    // Get discovered lights
    const res = await requestLedDeviceDiscovery('yeelight');
    if (res && !res.error) {
      const r = res.info;

      let discoveryMethod = "ssdp";
      if (res.info.discoveryMethod) {
        discoveryMethod = res.info.discoveryMethod;
      }

      // Process devices returned by discovery
      for (const device of r.devices) {
        if (device.hostname !== "") {
          processDiscoverdDevice(device, discoveryMethod);
        }
      }

      // Add additional items from configuration
      for (const configuredLight of configuredLights) {
        processConfiguredLight(configuredLight);
      }

      assign_lights();
    }
  }

  function processDiscoverdDevice(device, discoveryMethod) {
    if (getHostInLights(device.hostname).length > 0) {
      return;
    }

    const light = {
      host: device.hostname
    };

    if (discoveryMethod === "ssdp") {
      if (device.domain) {
        light.host += '.' + device.domain;
      }
    } else {
      light.host = device.service;
      light.name = device.name;
    }

    light.port = device.port;

    if (device.txt) {
      light.model = device.txt.md;
      light.port = 55443; // Yeelight default port
    } else {
      light.name = device.other.name;
      light.model = device.other.model;
    }

    lights.push(light);
  }
  function processConfiguredLight(configuredLight) {
    const host = configuredLight.host;
    let port = configuredLight.port || 0;

    if (host !== "" && getHostInLights(host).length === 0) {
      const light = {
        host: host,
        port: port,
        name: configuredLight.name,
        model: "color4"
      };

      lights.push(light);
    }
  }

  function attachIdentifyButtonEvent() {
    $('#wizp2_body').on('click', '.btn-identify', function () {
      const hostname = $(this).data('hostname');
      const port = $(this).data('port');
      identify(hostname, port);
    });
  }

  function assign_lights() {
    // Model mappings, see https://www.home-assistant.io/integrations/yeelight/
    const models = ['color', 'color1', 'YLDP02YL', 'YLDP02YL', 'color2', 'YLDP06YL', 'color4', 'YLDP13YL', 'color6', 'YLDP13AYL', 'colorb', "YLDP005", 'colorc', "YLDP004-A", 'stripe', 'YLDD04YL', 'strip1', 'YLDD01YL', 'YLDD02YL', 'strip4', 'YLDD05YL', 'strip6', 'YLDD05YL'];

    // If records are left for configuration
    if (Object.keys(lights).length > 0) {
      $('#wh_topcontainer').toggle(false);
      $('#yee_ids_t, #btn_wiz_save').toggle(true);

      const lightOptions = utils.getLayoutPositions();
      lightOptions.unshift("disabled");

      $('.lidsb').html("");
      let pos = "";

      for (const lightid in lights) {
        const lightHostname = lights[lightid].host;
        const lightPort = lights[lightid].port;
        let lightName = lights[lightid].name;

        if (lightName === "")
          lightName = $.i18n('edt_dev_spec_lights_itemtitle') + '(' + lightHostname + ')';

        let options = "";
        for (const opt in lightOptions) {
          const val = lightOptions[opt];
          options += '<option value="' + val + '"';
          if (pos === val) options += ' selected="selected"';
          options += '>' + $.i18n('conf_leds_layout_cl_' + val) + '</option>';
        }

        let enabled = 'enabled';
        if (!models.includes(lights[lightid].model)) {
          enabled = 'disabled';
          options = '<option value=disabled>' + $.i18n('wiz_yeelight_unsupported') + '</option>';
        }

        $('.lidsb').append(createTableRow([(parseInt(lightid, 10) + 1) + '. ' + lightName, '<select id="yee_' + lightid + '" ' + enabled + ' class="yee_sel_watch form-control">'
          + options
          + '</select>', '<button class="btn btn-sm btn-primary btn-identify" data-hostname="' + lightHostname + '" data-port="' + lightPort + '")>'
          + $.i18n('wiz_identify') + '</button>']));
      }
      attachIdentifyButtonEvent();

      $('.yee_sel_watch').on("change", function () {
        let cC = 0;
        for (const key in lights) {
          if ($('#yee_' + key).val() !== "disabled") {
            cC++;
          }
        }

        if (cC === 0 || window.readOnlyMode)
          $('#btn_wiz_save').prop("disabled", true);
        else
          $('#btn_wiz_save').prop("disabled", false);
      });
      $('.yee_sel_watch').trigger('change');
    }
    else {
      const noLightsTxt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_noLights', 'lights') + '</p>';
      $('#wizp2_body').append(noLightsTxt);
    }
  }

  async function identify(host, port) {

    const disabled = $('#btn_wiz_save').is(':disabled');

    // Take care that new record cannot be save during background process
    $('#btn_wiz_save').prop('disabled', true);

    const params = { host: host, port: port };
    await requestLedDeviceIdentification("yeelight", params);

    if (!window.readOnlyMode) {
      $('#btn_wiz_save').prop('disabled', disabled);
    }
  }

  return {
    start: function (e) {
    //create html
    const yeelight_title = 'wiz_yeelight_title';
    const yeelight_intro1 = 'wiz_yeelight_intro1';

    $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(yeelight_title));
    $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(yeelight_title) + '</h4><p>' + $.i18n(yeelight_intro1) + '</p>');

    $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'
      + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'
      + $.i18n('general_btn_cancel') + '</button>');

    $('#wizp2_body').html('<div id="wh_topcontainer"></div>');

    $('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

    $('#wizp2_body').append('<div id="yee_ids_t" style="display:none"><p style="font-weight:bold" id="yee_id_headline">' + $.i18n('wiz_yeelight_desc2') + '</p></div>');

    createTable("lidsh", "lidsb", "yee_ids_t");
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
}) ();

export { yeelightWizard };

