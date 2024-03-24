//clear priority and other tasks if people reload the page or lost connection while a wizard was active
$(window.hyperion).one("ready", function (event) {
  if (getStorage("wizardactive") === 'true') {
    requestPriorityClear();
    setStorage("wizardactive", false);
  }
});

$("#btn_wizard_colorcalibration").click(async function () {
  const { colorCalibrationKodiWizard } = await import('./wizards/colorCalibrationKodiWizard.js');
  colorCalibrationKodiWizard.start();
});

$('#btn_wizard_byteorder').on('click', async () => {
  const { rgbByteOrderWizard } = await import('./wizards/rgbByteOrderWizard.js');
  rgbByteOrderWizard.start();
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

  let data = {};
  let title;

  $('#btn_wiz_holder').html("");
  $('#btn_led_device_wiz').off();
  if (ledType == "philipshue") {
    $('#btn_wiz_holder').show();
    data = { ledType };
    title = 'wiz_hue_title';
  }
  else if (ledType == "nanoleaf") {
    $('#btn_wiz_holder').hide();
    data = { ledType };
    title = 'wiz_nanoleaf_user_auth_title';
  }
  else if (ledType == "atmoorb") {
    $('#btn_wiz_holder').show();
    data = { ledType };
    title = 'wiz_atmoorb_title';
  }
  else if (ledType == "yeelight") {
    $('#btn_wiz_holder').show();
    data = { ledType };
    title = 'wiz_yeelight_title';
  }

  if (Object.keys(data).length !== 0) {
    startLedDeviceWizard(data, title, ledType + "Wizard");
  }
}

function startLedDeviceWizard(data, hint, wizardName) {
  $('#btn_wiz_holder').html("")
  createHint("wizard", $.i18n(hint), "btn_wiz_holder", "btn_led_device_wiz");
  $('#btn_led_device_wiz').off();
  $('#btn_led_device_wiz').on('click', async (e) => {
    const { [wizardName]: winzardObject } = await import('./wizards/LedDevice_' + data.ledType + '.js');
    winzardObject.start(e);
  });
}

