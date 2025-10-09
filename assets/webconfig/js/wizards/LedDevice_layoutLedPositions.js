//****************************
// Wizard LED Layout
//****************************

import { ledDeviceWizardUtils as utils } from './LedDevice_utils.js';

const layoutLedPositionsWizard = (() => {

  let wiz_editor;

  function createEditor() {
    wiz_editor = createJsonEditor('editor_container_wiz', {
      layoutPosition: {
        "type": "string",
        "title": "wiz_layout_led_position_title",
        "enum": utils.getLayoutPositions(),
        "options": {
          "enum_titles": utils.getLayoutPositionsTitles()
        }
      }
    }, true, true);
  }

  function stopWizardLedLayout(reload) {
    resetWizard(reload);
  }

  function beginWizardLayoutLedPositions() {
    createEditor();
    setStorage("wizardactive", true);

    $('#btn_wiz_abort').off().on('click', function () {
      stopWizardLedLayout(true);
    });

    $('#btn_wiz_ok').off().on('click', function () {
      const layoutPosition = wiz_editor.getEditor("root.layoutPosition").getValue();
      const layoutObject = utils.assignLightPos(layoutPosition);

      var layoutObjects = [];
      layoutObjects.push(JSON.parse(JSON.stringify(layoutObject)));
      aceEdt.set(layoutObjects);

      stopWizardLedLayout(true);
    });
  }

  return {
    start: function (e, data) {
      $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n('wiz_layout_led_positions_title'));
      $('#wizp1_body').html('<div <p style="font-weight:bold">' + $.i18n('wiz_layout_led_positions_expl', data.ledType) + '</p></div>' +
        '<div id="editor_container_wiz"></div>'
      );
      $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_ok"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_ok') + 
      '</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>'
      );

      if (getStorage("darkMode") == "on")
        $('#wizard_logo').attr("src", 'img/hyperion/logo_negativ.png');

      //open modal
      $("#wizard_modal").modal({
        backdrop: "static",
        keyboard: false,
        show: true
      });

      beginWizardLayoutLedPositions();
    }
  };
})();

export { layoutLedPositionsWizard };

