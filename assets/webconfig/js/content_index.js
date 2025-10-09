$(document).ready(() => {
  initializeApp();
});

$(window.hyperion).on("ready", function (event) {

  loadContent(undefined, true);

  // Hide capture menu entries if no grabbers are available
  if ((window.serverInfo.grabbers.screen.available.length === 0) &&
    (window.serverInfo.grabbers.video.available.length === 0) &&
    (window.serverInfo.grabbers.audio.available.length === 0)) {
    $("#MenuItemGrabber").hide();
    if ((jQuery.inArray("boblight", window.serverInfo.services) === -1)) {
      $("#MenuItemInstCapture").hide();
    }
  }

  // Hide effects config menu entry if effect engine is not available
  if (jQuery.inArray("effectengine", window.serverInfo.services) === -1) {
    $("#MenuItemEffectsConfig").hide();
  }

  $("#main-nav, #top-navbar").show();
  if (!window.defaultPasswordIsSet) {
    $("#btn_lock_ui").show();
  }
});

$(window.hyperion).on("error", function (event) {
  const error = event.reason;

  //An error "No Authorization" is handled by waitForSuccessfulAuthorization
  if (error?.message !== "No Authorization") {

    const errorDetails = [];

    if (error?.cmd) {
      errorDetails.push(`Command: "${error.cmd}"`);
    }

    errorDetails.push(error?.details || "No additional details.");

    showInfoDialog("error", "", error?.message || "Unknown error", errorDetails);
  }
});

$(window.hyperion).on("cmd-authorize-tokenRequest cmd-authorize-getPendingTokenRequests", function (event) {

  if (event.response && event.response.info !== undefined) {
    const val = event.response.info;

    if (Array.isArray(event.response.info)) {
      if (event.response.info.length == 0) {
        return
      }
      const info = event.response.info[0]
      if (info.comment == '')
        $('#modal_dialog').modal('hide');
    }

    showInfoDialog("grantToken", $.i18n('conf_network_tok_grantT'), $.i18n('conf_network_tok_grantMsg') + '<br><span style="font-weight:bold">App: ' + val.comment + '</span><br><span style="font-weight:bold">Code: ' + val.id + '</span>')

    $("#tok_grant_acc").off().on('click', function () {
      addToTokenList(val);

      // forward event, in case we need to rebuild the list now
      $(window.hyperion).trigger({ type: "build-token-list" });
      requestHandleTokenRequest(val.id, true)
    });
    $("#tok_deny_acc").off().on('click', function () {
      requestHandleTokenRequest(val.id, false)
    });
  }
});

$(window.hyperion).one("cmd-authorize-getTokenList", function (event) {
  setTokenList(event.response.info);
});

$(window.hyperion).on("cmd-authorize-newPassword", async function (event) {

  if (event.response.success === true) {
    showInfoDialog("success", $.i18n('InfoDialog_changePassword_success'));
 
    try {
      // Force login with new passwort
      await waitForSuccessfulAuthorization();
      $(window.hyperion).trigger("ready");
    } catch (err) {
      console.error("Authorization failed for new password:", err);
    }
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

  const isInstanceRunningBeforeUpdate = isCurrentInstanceRunning();

  window.serverInfo.instance = event.response.data;

  // notify the update
  $(window.hyperion).trigger("instance-updated");

  if (!isCurrentInstanceRunning()) {
    let newInstance = getFirstRunningInstance();
    if (newInstance === null) {

      const lastSelectedInstance = getStorage('lastSelectedInstance');

      if (doesInstanceExist(lastSelectedInstance)) {

        newInstance = lastSelectedInstance;
      } else {
        newInstance = getFirstConfiguredInstance();
        //Delete Storage information about the last used but now stopped instance
        removeStorage('lastSelectedInstance');
      }
    }

    if (newInstance !== null) {
      instanceSwitch(newInstance);
    } else {
      debugMessage("No instance is configured.");
      window.currentHyperionInstance = null;
      updateUiOnInstance(window.currentHyperionInstance);
    }
  } else if (!isInstanceRunningBeforeUpdate) {
    if (window.currentHyperionInstance !== null) {
      requestServerInfo(window.currentHyperionInstance);
    }
  }

  updateHyperionInstanceListing();
});

$(window.hyperion).on("cmd-instance-switchTo", function (event) {
  const newInstance = window.currentHyperionInstance;

  if (typeof newInstance === "number" && !isNaN(newInstance)) {
    getServerInformation(newInstance);
  } else {
    console.warn("Invalid instance ID during switch");
  }
});

$(window.hyperion).on("cmd-effects-update", function (event) {
  window.serverInfo.effects = event.response.data.effects
});

function waitForSuccessfulAuthorization() {
  return new Promise((resolve, reject) => {
    let loginResolved = false;

    const onLogin = (event) => {
      if (!loginResolved) {
        loginResolved = true;
        $(window.hyperion).off("cmd-authorize-login", onLogin);
        resolve(event);
      }
    };

    const onPwRequired = async (event) => {
      try {
        await handlePasswordRequirement(event);
      } catch (err) {
        reject(err);
      }
    };

    const onError = (event) => {
      const error = event.reason;
      if (error?.message === "No Authorization" && getStorage("loginToken")) {
        removeStorage("loginToken");
        requestRequiresDefaultPasswortChange(); // Retry trigger
      }
    };

    $(window.hyperion).on("cmd-authorize-login", onLogin);
    $(window.hyperion).on("cmd-authorize-newPasswordRequired", onPwRequired);
    $(window.hyperion).on("error", onError);

    // Kick off the process (non-async call)
    requestRequiresDefaultPasswortChange();
  });
}

async function initializeApp() {
  try {
    setupDarkMode();
    setMediaStreamingSupport();
    await loadEssentialContent();

    // Ensure WebSocket is initialized and open
    initWebSocket();
    await waitForEvent("open"); // Wait for the WebSocket "open" event to confirm the connection

    // Run authorization flow and wait for login success
    await waitForSuccessfulAuthorization();

    // Run following calls in parallel
    await Promise.all([
      requestTokenInfo(),
      requestGetPendingTokenRequests(),
      requestServerConfigSchema(),
      requestSysInfo()
    ]);

    //Get configuration schemas
    const schemaEvent = await waitForEvent("cmd-config-getschema");
    await handleSchema(schemaEvent);

    // Once getschema is complete, we can proceed with getServerInformation
    const instance = getStoredInstance();
    await getServerInformation(instance);

    await waitForEvent("cmd-serverinfo-getInfo");
    // cmd-serverinfo-getInfo will trigger hyperion - ready

    bindUiHandlers();
  }
  catch (err) {
    console.error("App initialization failed:", err);
    showInfoDialog("error", "UI startup failed", err.message || err);
  }
}

function setupDarkMode() {
  const darkModeOverwrite = getStorage("darkModeOverwrite");

  if (darkModeOverwrite == "false" || darkModeOverwrite == null) {
    if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
      handleDarkMode();
    }
    if (window.matchMedia('(prefers-color-scheme: light)').matches) {
      setStorage("darkMode", "off");
    }
  }
  if (getStorage("darkMode") == "on") {
    handleDarkMode();
  }
}

function setMediaStreamingSupport() {
  const isMediaStreamingSupported = !!(navigator.mediaDevices?.getDisplayMedia);
  setStorage('mediaStreamingSupported', isMediaStreamingSupported);
}

async function loadEssentialContent() {
  loadContentTo("#container_connection_lost", "connection_lost");
  loadContentTo("#container_restart", "restart");
}

async function handlePasswordRequirement(event) {
  const token = getStorage("loginToken");

  if (event.response.info.newPasswordRequired) {
    window.defaultPasswordIsSet = true;

    if (token) {
      requestTokenAuthorization(token);
    } else {
      requestAuthorization('hyperion');
    }

    const loginEvent = await waitForEvent("cmd-authorize-login");
    handleLogin(loginEvent, token !== null);
  } else {
    $("#main-nav, #top-navbar").hide();

    if (token) {
      requestTokenAuthorization(token);

      const loginEvent = await waitForEvent("cmd-authorize-login");
      handleLogin(loginEvent, true);
    } else {
      loadContentTo("#page-content", "login");
    }
  }
}

function handleLogin(event, isLoggedIn = false) {
  if (isLoggedIn && window.defaultPasswordIsSet && getStorage("suppressDefaultPwWarning") !== "true") {
    const msg = `
      <div class="text-right">
        ${$.i18n('dashboard_message_do_not_show_again')}
        <input id="chk_suppressDefaultPw" type="checkbox" onChange="suppressDefaultPwWarning()">
      </div>`;

    showNotification(
      'warning',
      $.i18n('dashboard_message_default_password'),
      $.i18n('dashboard_message_default_password_t'),
      `<a style="cursor:pointer" onClick="changePassword()">
        ${$.i18n('InfoDialog_changePassword_title')}
      </a>` + msg
    );
  } else {
    $("#btn_lock_ui").hide();
  }

  if (event.response?.info?.token) {
    setStorage("loginToken", event.response.info.token);
  }
}


async function handleSchema(event) {
  window.serverSchema = event.response.info;
  window.schema = window.serverSchema.properties;
}

function getStoredInstance() {
  const stored = getStorage('lastSelectedInstance');
  return (stored !== null && !isNaN(Number(stored))) ? Number(stored) : null;
}

async function getServerInformation(instance) {

  try {
    // Step 1: Start waiting for the config-getconfig event before sending the request
    const configEventPromise = new Promise((resolve) => {
      const handler = function (event) {
        $(window.hyperion).off("cmd-config-getconfig", handler);
        resolve(event);
      };
      $(window.hyperion).on("cmd-config-getconfig", handler);
    });

    // Step 2: Send config request
    const configResponsePromise = requestServerConfig.async([], [instance], []);

    // Step 3: Wait for both response and event
    const [, configEvent] = await Promise.all([
      configResponsePromise,
      configEventPromise
    ]);

    const config = configEvent.response.info;
    let instanceId = instance;
    const { instanceIds, instances } = config;

    // Step 4: Validate instance
    if (!instanceIds.includes(instance)) {
      // Choose a fallback: enabled instance or first
      const fallback = instances.find((inst) => inst.enabled)?.id || instanceIds[0];
      if (fallback !== undefined) {
        instanceId = fallback;
        window.currentHyperionInstance = fallback;
        setStorage('lastSelectedInstance', fallback);
      } else {
        console.warn("No valid instance found");
        window.currentHyperionInstance = null;
        instanceId = [];
      }
    } else {
      window.currentHyperionInstance = instance;
    }

    // Step 5: Update server config for the (corrected) instance
    const legacyConfig = reverseTransformConfig(config, instanceId);
    window.serverConfig = legacyConfig;
    window.showOptHelp = window.serverConfig.general?.showOptHelp;
    $(window.hyperion).trigger("serverConfig_updated");

    // Step 6: Load additional info
    requestServerInfo(instanceId);

  } catch (error) {
    console.error("Error in getServerInformation:", error);
  }
}

$(window.hyperion).on("cmd-serverinfo-getInfo", function (event) {

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
  const running = window.serverInfo.instance.filter(entry => entry.running);
  if (running.length > 1) {
    $('#btn_hypinstanceswitch').toggle(true)
  } else {
    $('#btn_hypinstanceswitch').toggle(false)
  }
}); // end cmd-serverinfo

$(window.hyperion).on("cmd-sysinfo", function (event) {
  window.sysInfo = event.response.info;

  window.currentVersion = window.sysInfo.hyperion.version;
  window.currentChannel = window.sysInfo.hyperion.channel;
  window.readOnlyMode = window.sysInfo.hyperion.readOnlyMode;
});

$(window.hyperion).on("cmd-config-setconfig", function (event) {
  if (event.response.success === true) {
    showNotification('success', $.i18n('dashboard_alert_message_confsave_success'), $.i18n('dashboard_alert_message_confsave_success_t'))
  }
});

function bindUiHandlers() {

  // Side menu link click activation
  $('#side-menu li a, #side-menu li ul li a').on("click", function () {
    $('#side-menu').find('.active').removeClass('active');
    $(this).addClass('active');
  });

  // Dark mode toggle button
  $("#btn_darkmode").off().on("click", function () {
    const darkOn = getStorage("darkMode") === "on";
    setStorage("darkModeOverwrite", true);
    if (darkOn) {
      setStorage("darkMode", "off");
      location.reload();
    } else {
      handleDarkMode();
    }
  });

  // Main navigation click handling
  document.querySelectorAll(".mnava").forEach(el => {
    el.addEventListener("click", function (e) {
      loadContent(e);
      window.scrollTo(0, 0);
    });
  });

  // Scroll-based logo visibility
  window.addEventListener("scroll", function () {
    const logo = document.getElementById("navbar_brand_logo");
    if (!logo) return;

    logo.style.display = window.scrollY > 65 ? "none" : "";
  }, { passive: true });

  // Language selector
  $(".langSelect").off().on("click", function () {
    const newLang = $(this).attr("id").replace("lang_", "");
    setStorage("lang", newLang);
    location.reload();
  });

  // Toggle top menu for mobile
  $(".navbar-toggle").off().on("click", function () {
    const target = $(this).data("target");
    $(target).toggleClass("collapse");
  });

  // Scroll to top button
  $("#btn_top").off().on("click", function () {
    $("html, body").animate({ scrollTop: 0 }, "slow");
  });

  // Prevent form submissions with Enter in specific cases (optional UX improvement)
  $("form input").on("keypress", function (e) {
    if (e.which === 13 && $(this).attr("type") !== "textarea") {
      e.preventDefault();
    }
  });
}

function suppressDefaultPwWarning() {
  const checked = document.getElementById('chk_suppressDefaultPw').checked;
  setStorage("suppressDefaultPwWarning", String(checked));
}

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
