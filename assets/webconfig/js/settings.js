var availAccess = ['default', 'advanced', 'expert'];
var storedAccess;

//Change Password
function changePassword() {
  showInfoDialog('changePassword', $.i18n('InfoDialog_changePassword_title'));

  // fill default pw if default is set
  if (window.defaultPasswordIsSet)
    $('#current-password').val('hyperion')

  $('#id_btn_ok').off().on('click', function () {
    var oldPw = $('#current-password').val();
    var newPw = $('#new-password').val();

    requestChangePassword(oldPw, newPw);
    history.pushState({}, "New password");
  });

  $('#new-password, #current-password').off().on('input', function (e) {
    ($('#current-password').val().length >= 8 && $('#new-password').val().length >= 8) && !window.readOnlyMode ? $('#id_btn_ok').prop('disabled', false) : $('#id_btn_ok').prop('disabled', true);
  });
}

$(document).ready(function () {

  if (!storageComp()) {
    showInfoDialog('warning', "Can't store settings", "Your browser doesn't support localStorage. You can't save a specific language setting (fallback to 'auto detection') and access level (fallback to 'default'). Some wizards may be hidden. You could still use the webinterface without further issues");
    $('#language-select').prop("disabled", true);
    $('#btn_setaccess').prop("disabled", true);
  }

  initLanguageSelection();

  //access
  storedAccess = getStorage("accesslevel");
  if (storedAccess == null) {
    storedAccess = "default";
    setStorage("accesslevel", storedAccess);
  }

  if (!storageComp()) {
    showInfoDialog('warning', $.i18n('InfoDialog_nostorage_title'), $.i18n('InfoDialog_nostorage_text'));
    $('#btn_setlang').prop("disabled", true);
  }

  $('#btn_setaccess').off().on('click', function () {
    var newAccess;
    showInfoDialog('select', $.i18n('InfoDialog_access_title'), $.i18n('InfoDialog_access_text'));

    for (var lcx = 0; lcx < availAccess.length; lcx++) {
      $('#id_select').append(createSelOpt(availAccess[lcx], $.i18n('general_access_' + availAccess[lcx])));
    }

    $('#id_select').val(storedAccess);

    $('#id_select').off().on('change', function () {
      newAccess = $('#id_select').val();
      if (newAccess == storedAccess)
        $('#id_btn_saveset').prop('disabled', true);
      else
        $('#id_btn_saveset').prop('disabled', false);
    });

    $('#id_btn_saveset').off().on('click', function () {
      setStorage("accesslevel", newAccess);
      reload();
    });

    $('#id_select').trigger('change');
  });

  // change pw btn
  $('#btn_changePassword').off().on('click', function () {
    changePassword();
  });

  //Lock Ui
  $('#btn_lock_ui').off().on('click', function () {
    removeStorage('loginToken');
    location.replace('/');
  });

  //hide menu elements
  if (storedAccess != 'expert')
    $('#load_webconfig').toggle(false);

});

