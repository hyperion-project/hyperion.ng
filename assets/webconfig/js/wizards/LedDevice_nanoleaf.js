//****************************
// Wizard Nanoleaf
//****************************

const nanoleafWizard = (() => {

  const retryInterval = 2;

  async function createNanoleafUserAuthorization() {
    const host = conf_editor.getEditor("root.specificOptions.host").getValue();
    const params = { host };
    let retryTime = 30;

    const UserInterval = setInterval(async function () {
      retryTime -= retryInterval;
      $("#connectionTime").html(retryTime);

      if (retryTime <= 0) {
        handleTimeout();
      } else {
        const res = await requestLedDeviceAddAuthorization('nanoleaf', params);
        handleResponse(res);
      }
    }, retryInterval * 1000);

    function handleTimeout() {
      clearInterval(UserInterval);
      showNotification(
        'warning',
        $.i18n('wiz_nanoleaf_failure_auth_token'),
        $.i18n('wiz_nanoleaf_failure_auth_token_t')
      );
      resetWizard(true);
    }

    function handleResponse(res) {
      if (res && !res.error) {
        const response = res.info;
        if (jQuery.isEmptyObject(response)) {
          debugMessage(`${retryTime}: Power On/Off button not pressed or device not reachable`);
        } else {
          const token = response.auth_token;
          if (token !== 'undefined') {
            conf_editor.getEditor("root.specificOptions.token").setValue(token);
          }
          clearInterval(UserInterval);
          resetWizard(true);
        }
      } else {
        clearInterval(UserInterval);
        resetWizard(true);
      }
    }
  }

  return {
    start: function () {
      const nanoleaf_user_auth_title = 'wiz_nanoleaf_user_auth_title';
      const nanoleaf_user_auth_intro = 'wiz_nanoleaf_user_auth_intro';

      $('#wiz_header').html(
        `<i class="fa fa-magic fa-fw"></i>${$.i18n(nanoleaf_user_auth_title)}`
      );
      $('#wizp1_body').html(
        `<h4 style="font-weight:bold;text-transform:uppercase;">${$.i18n(nanoleaf_user_auth_title)}</h4><p>${$.i18n(nanoleaf_user_auth_intro)}</p>`
      );
      $('#wizp1_footer').html(
        `<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>${$.i18n('general_btn_continue')}</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>${$.i18n('general_btn_cancel')}</button>`
      );
      $('#wizp3_body').html(
        `<span>${$.i18n('wiz_nanoleaf_press_onoff_button')}</span> <br /><br /><center><span id="connectionTime"></span><br /><i class="fa fa-cog fa-spin" style="font-size:100px"></i></center>`
      );

      if (getStorage("darkMode") == "on") {
        $('#wizard_logo').attr("src", 'img/hyperion/logo_negativ.png');
      }

      $("#wizard_modal").modal({
        backdrop: "static",
        keyboard: false,
        show: true
      });

      $('#btn_wiz_cont').off().on('click', function () {
        createNanoleafUserAuthorization();
        $('#wizp1').toggle(false);
        $('#wizp3').toggle(true);
      });
    }
  };
})();

export { nanoleafWizard };

