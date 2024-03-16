//clear priority and other tasks if people reload the page or lost connection while a wizard was active
$(window.hyperion).one("ready", function (event) {
  if (getStorage("wizardactive") === 'true') {
    requestPriorityClear();
    setStorage("wizardactive", false);
  }
});

$("#btn_wizard_colorcalibration").click(function() {
      const script = 'js/wizards/wizard_kodi_cc.js';
      loadScript(script, function () {
        startWizardCC();
      });
});

$("#btn_wizard_byteorder").click(function() {
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

