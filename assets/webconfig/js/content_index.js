$(document).ready(function () {
  var darkModeOverwrite = getStorage("darkModeOverwrite");

  if (darkModeOverwrite == "false" || darkModeOverwrite == null) {
    if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
      handleDarkMode();
    }

    if (window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches) {
      setStorage("darkMode", "off");
    }
  }

  if (getStorage("darkMode") == "on") {
    handleDarkMode();
  }

  loadContentTo("#container_connection_lost", "connection_lost");
  loadContentTo("#container_restart", "restart");
  initWebSocket();

  $(window.hyperion).on("cmd-serverinfo", function (event) {
    window.serverInfo = event.response.info;

    // comps
    window.comps = event.response.info.components

    $(window.hyperion).trigger("ready");

    window.comps.forEach(function (obj) {
      if (obj.name == "ALL") {
        if (obj.enabled)
          $("#hyperion_disabled_notify").fadeOut("fast");
        else
          $("#hyperion_disabled_notify").fadeIn("fast");
      }
    });

    // determine button visibility
    var running = window.serverInfo.instance.filter(entry => entry.running);
    if (running.length > 1)
      $('#btn_hypinstanceswitch').toggle(true)
    else
      $('#btn_hypinstanceswitch').toggle(false)
  }); // end cmd-serverinfo

  // Update language selection
  $("#language-select").on('changed.bs.select', function (e, clickedIndex, isSelected, previousValue) {
    var newLang = availLang[clickedIndex];
    if (newLang !== storedLang) {
      setStorage("langcode", newLang);
      reload();
    }
  });

  $("#language-select").selectpicker(
    {
      container: 'body'
    });

  $(".bootstrap-select").on("click", function () {
    $(this).addClass("open");
  });

  $(document).on("click", function () {
    $(".bootstrap-select").removeClass("open");
  });

  $(".bootstrap-select").on("click", function (e) {
    e.stopPropagation();
  });

  //End language selection

  $(window.hyperion).on("cmd-authorize-tokenRequest cmd-authorize-getPendingTokenRequests", function (event) {
    var val = event.response.info;
    if (Array.isArray(event.response.info)) {
      if (event.response.info.length == 0) {
        return
      }
      val = event.response.info[0]
      if (val.comment == '')
        $('#modal_dialog').modal('hide');
    }

    showInfoDialog("grantToken", $.i18n('conf_network_tok_grantT'), $.i18n('conf_network_tok_grantMsg') + '<br><span style="font-weight:bold">App: ' + val.comment + '</span><br><span style="font-weight:bold">Code: ' + val.id + '</span>')
    $("#tok_grant_acc").off().on('click', function () {
      tokenList.push(val)
      // forward event, in case we need to rebuild the list now
      $(window.hyperion).trigger({ type: "build-token-list" });
      requestHandleTokenRequest(val.id, true)
    });
    $("#tok_deny_acc").off().on('click', function () {
      requestHandleTokenRequest(val.id, false)
    });
  });

  $(window.hyperion).one("cmd-authorize-getTokenList", function (event) {
    tokenList = event.response.info;
  });

  $(window.hyperion).on("cmd-sysinfo", function (event) {
    window.sysInfo = event.response.info;

    window.currentVersion = window.sysInfo.hyperion.version;
    window.currentChannel = window.sysInfo.hyperion.channel;
    window.readOnlyMode = window.sysInfo.hyperion.readOnlyMode;
  });

  $(window.hyperion).one("cmd-config-getschema", function (event) {
    window.serverSchema = event.response.info;
    window.schema = window.serverSchema.properties;

    requestTokenInfo();
    requestGetPendingTokenRequests();

    //Switch to last selected instance and load related config
    var lastSelectedInstance = getStorage('lastSelectedInstance');
    if (lastSelectedInstance == null || window.serverInfo.instance && !window.serverInfo.instance[lastSelectedInstance]) {
      lastSelectedInstance = 0;
    }
    instanceSwitch(lastSelectedInstance);

    requestSysInfo();
  });

  $(window.hyperion).on("cmd-config-getconfig", function (event) {
    window.serverConfig = event.response.info;

    window.showOptHelp = window.serverConfig.general.showOptHelp;
  });

  $(window.hyperion).on("cmd-config-setconfig", function (event) {
    if (event.response.success === true) {
      showNotification('success', $.i18n('dashboard_alert_message_confsave_success'), $.i18n('dashboard_alert_message_confsave_success_t'))
    }
  });

  $(window.hyperion).on("cmd-authorize-login", function (event) {
    $("#main-nav").removeAttr('style')
    $("#top-navbar").removeAttr('style')

    if (window.defaultPasswordIsSet === true && getStorage("suppressDefaultPwWarning") !== "true") {
      var supprPwWarnCheckbox = '<div class="text-right">' + $.i18n('dashboard_message_do_not_show_again')
        + ' <input id="chk_suppressDefaultPw" type="checkbox" onChange="suppressDefaultPwWarning()"> </div>'
      showNotification('warning', $.i18n('dashboard_message_default_password'), $.i18n('dashboard_message_default_password_t'), '<a style="cursor:pointer" onClick="changePassword()">'
        + $.i18n('InfoDialog_changePassword_title') + '</a>' + supprPwWarnCheckbox)
    }
    else
      //if logged on and pw != default show option to lock ui
      $("#btn_lock_ui").removeAttr('style')

    if (event.response.hasOwnProperty('info'))
      setStorage("loginToken", event.response.info.token);

    requestServerConfigSchema();
  });

  $(window.hyperion).on("cmd-authorize-newPassword", function (event) {
    if (event.response.success === true) {
      showInfoDialog("success", $.i18n('InfoDialog_changePassword_success'));
      // not necessarily true, but better than nothing
      window.defaultPasswordIsSet = false;
    }
  });

  $(window.hyperion).on("cmd-authorize-newPasswordRequired", function (event) {
    var loginToken = getStorage("loginToken")

    if (event.response.info.newPasswordRequired == true) {
      window.defaultPasswordIsSet = true;

      if (loginToken)
        requestTokenAuthorization(loginToken)
      else
        requestAuthorization('hyperion');
    }
    else {
      $("#main-nav").attr('style', 'display:none')
      $("#top-navbar").attr('style', 'display:none')

      if (loginToken)
        requestTokenAuthorization(loginToken)
      else
        loadContentTo("#page-content", "login")
    }
  });

  $(window.hyperion).on("cmd-authorize-adminRequired", function (event) {
    //Check if a admin login is required.
    //If yes: check if default pw is set. If no: go ahead to get server config and render page
    if (event.response.info.adminRequired === true)
      requestRequiresDefaultPasswortChange();
    else
      requestServerConfigSchema();
  });

  $(window.hyperion).on("error", function (event) {
    //If we are getting an error "No Authorization" back with a set loginToken we will forward to new Login (Token is expired.
    //e.g.: hyperiond was started new in the meantime)
    if (event.reason == "No Authorization" && getStorage("loginToken")) {
      removeStorage("loginToken");
      requestRequiresAdminAuth();
    }
    else if (event.reason == "Selected Hyperion instance isn't running") {
      //Switch to default instance
      instanceSwitch(0);
    } else {
      showInfoDialog("error", "Error", event.reason);
    }
  });

  $(window.hyperion).on("open", function (event) {
    requestRequiresAdminAuth();
  });

  $(window.hyperion).on("ready", function (event) {
    loadContent(undefined,true);

    //Hide capture menu entries, if no grabbers are available
    if ((window.serverInfo.grabbers.screen.available.length === 0) &&
        (window.serverInfo.grabbers.video.available.length === 0) &&
        (window.serverInfo.grabbers.audio.available.length === 0)) {
      $("#MenuItemGrabber").attr('style', 'display:none')
      if ((jQuery.inArray("boblight", window.serverInfo.services) === -1)) {
        $("#MenuItemInstCapture").attr('style', 'display:none')
      }
    }

    //Hide effectsconfigurator menu entry, if effectengine is not available
    if (jQuery.inArray("effectengine", window.serverInfo.services) === -1) {
      $("#MenuItemEffectsConfig").attr('style', 'display:none')
    }
  });

  $(window.hyperion).on("cmd-adjustment-update", function (event) {
    window.serverInfo.adjustment = event.response.data
  });

  $(window.hyperion).on("cmd-videomode-update", function (event) {
    window.serverInfo.videomode = event.response.data.videomode
  });

  $(window.hyperion).on("cmd-components-update", function (event) {
    let obj = event.response.data

    // notfication in index
    if (obj.name == "ALL") {
      if (obj.enabled)
        $("#hyperion_disabled_notify").fadeOut("fast");
      else
        $("#hyperion_disabled_notify").fadeIn("fast");
    }

    window.comps.forEach((entry, index) => {
      if (entry.name === obj.name) {
        window.comps[index] = obj;
      }
    });
    // notify the update
    $(window.hyperion).trigger("components-updated", event.response.data);
  });

  $(window.hyperion).on("cmd-instance-update", function (event) {
    window.serverInfo.instance = event.response.data
    var avail = event.response.data;
    // notify the update
    $(window.hyperion).trigger("instance-updated");

    // if our current instance is no longer available we are at instance 0 again.
    var isInData = false;
    for (var key in avail) {
      if (avail[key].instance == currentHyperionInstance && avail[key].running) {
        isInData = true;
      }
    }

    if (!isInData) {
      //Delete Storage information about the last used but now stopped instance
      if (getStorage('lastSelectedInstance'))
        removeStorage('lastSelectedInstance')

      window.currentHyperionInstance = 0;
      window.currentHyperionInstanceName = getInstanceNameByIndex(0);

      requestServerConfig();
      setTimeout(requestServerInfo, 100)
      setTimeout(requestTokenInfo, 200)
    }

    // determine button visibility
    var running = serverInfo.instance.filter(entry => entry.running);
    if (running.length > 1)
      $('#btn_hypinstanceswitch').toggle(true)
    else
      $('#btn_hypinstanceswitch').toggle(false)

    // update listing for button
    updateUiOnInstance(window.currentHyperionInstance);
    updateHyperionInstanceListing();
  });

  $(window.hyperion).on("cmd-instance-switchTo", function (event) {
    requestServerConfig();
    setTimeout(requestServerInfo, 200)
    setTimeout(requestTokenInfo, 400)
  });

  $(window.hyperion).on("cmd-effects-update", function (event) {
    window.serverInfo.effects = event.response.data.effects
  });

  $(".mnava").on('click.menu', function (e) {
    loadContent(e);
    window.scrollTo(0, 0);
  });

  $(window).on("scroll", function () {
    if ($(window).scrollTop() > 65)
      $("#navbar_brand_logo").css("display", "none");
    else
      $("#navbar_brand_logo").css("display", "");
  });

  $('#side-menu li a, #side-menu li ul li a').on("click", function () {
    $('#side-menu').find('.active').toggleClass('inactive'); // find all active classes and set inactive;
    $(this).addClass('active');
  });
});

function suppressDefaultPwWarning() {
  if (document.getElementById('chk_suppressDefaultPw').checked)
    setStorage("suppressDefaultPwWarning", "true");
  else
    setStorage("suppressDefaultPwWarning", "false");
}

$(function () {
  var sidebar = $('#side-menu');  // cache sidebar to a variable for performance
  sidebar.on("click", 'a.inactive', function () {
    sidebar.find('.active').toggleClass('active inactive');
    $(this).toggleClass('active inactive');
  });
});

// hotfix body padding when bs modals overlap
$(document.body).on('hide.bs.modal,hidden.bs.modal', function () {
  $('body').css('padding-right', '0');
});

//Dark Mode
$("#btn_darkmode").off().on("click", function (e) {
  if (getStorage("darkMode") != "on") {
    handleDarkMode();
    setStorage("darkModeOverwrite", true);
  }
  else {
    setStorage("darkMode", "off",);
    setStorage("darkModeOverwrite", true);
    location.reload();
  }
});

// Menuitem toggle;
function SwitchToMenuItem(target, item) {
  document.getElementById(target).click(); // Get <a href menu item;
  let sidebar = $('#side-menu');  // Get sidebar menu;
  sidebar.find('.active').toggleClass('inactive'); // find all active classes and set inactive;
  sidebar.find('.in').removeClass("in"); // Find all collapsed menu items and close it by remove "in" class;
  $('#' + target).removeClass('inactive'); // Remove inactive state by classname;
  $('#' + target).addClass('active'); // Add active state by classname;
  let cl_object = $('#' + target).closest('ul'); // Find closest ul sidemenu header;
  cl_object.addClass('in'); // Add class "in" to expand header in sidebar menu;
  if (item) { // Jump to div "item" if available. Time limit 3 seconds
    function scrollTo(counter) {
      if (counter < 30) {
        setTimeout(function () {
          counter++;
          if ($('#' + item).length)
            $('#' + item)[0].scrollIntoView();
          else
            scrollTo(counter);
        }, 100);
      }
    }

    scrollTo(0);
  }
};

