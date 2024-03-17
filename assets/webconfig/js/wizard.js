//clear priority and other tasks if people reload the page or lost connection while a wizard was active
$(window.hyperion).one("ready", function (event) {
  if (getStorage("wizardactive") === 'true') {
    requestPriorityClear();
    setStorage("wizardactive", false);
  }
});

$("#btn_wizard_colorcalibration").click(function () {
  const script = 'js/wizards/wizard_kodi_cc.js';
  loadScript(script, function () {
    startWizardCC();
  });
});

$("#btn_wizard_byteorder").click(function () {
  const script = 'js/wizards/wizard_byteorder.js';
  loadScript(script, function () {
    startWizardRGB();
  });
});

function resetWizard(reload) {
  $("#wizard_modal").modal('hide');
  requestPriorityClear();
  setStorage("wizardactive", false);
  $('#wizp1').toggle(true);
  $('#wizp2').toggle(false);
  $('#wizp3').toggle(false);
  if (!reload) {
    location.reload();
  }
}

function createLedDeviceWizards(ledType) {
  $('#btn_wiz_holder').html("");
  $('#btn_led_device_wiz').off();
  if (ledType == "philipshue") {
    const data = { ledType: ledType };
    const hue_title = 'wiz_hue_title';
    $('#btn_wiz_holder').show();
    changeLedDeviceWizard(data, hue_title, "startWizardPhilipsHue");
  }
  else if (ledType == "nanoleaf") {
    $('#btn_wiz_holder').hide();
    const data = { ledType: ledType };
    const nanoleaf_user_auth_title = 'wiz_nanoleaf_user_auth_title';
    changeLedDeviceWizard(data, nanoleaf_user_auth_title, "startWizardNanoleafUserAuth");
  }
  else if (ledType == "atmoorb") {
    $('#btn_wiz_holder').show();
    const data = { ledType: ledType };
    const atmoorb_title = 'wiz_atmoorb_title';
    changeLedDeviceWizard(data, atmoorb_title, "startWizardAtmoOrb");
  }
  else if (ledType == "yeelight") {
    $('#btn_wiz_holder').show();
    const data = { ledType: ledType };
    const yeelight_title = 'wiz_yeelight_title';
    changeLedDeviceWizard(data, yeelight_title, "startWizardYeelight");
  }
}

function changeLedDeviceWizard(data, hint, fnName) {
  $('#btn_wiz_holder').html("")
  createHint("wizard", $.i18n(hint), "btn_wiz_holder", "btn_led_device_wiz");
  $('#btn_led_device_wiz').off();
  $('#btn_led_device_wiz').on('click', data, function (event) {
    // Load LED Device wizard utils
    loadScript('js/wizards/LedDevice_utils.js');
    // Load the respective LED Device wizard code
    const script = 'js/wizards/LedDevice_' + data.ledType + '.js';
    loadScript(script, function () {
      if (typeof window[fnName] === 'function') {
        // Pass the event parameter to the function specified by fnName
        window[fnName](event);
      } else {
        console.error("Function", fnName, "is not available.");
      }
    });
  });
}

