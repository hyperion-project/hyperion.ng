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

// Layout positions
var lightPosTop = { hmin: 0.15, hmax: 0.85, vmin: 0, vmax: 0.2 };
var lightPosTopLeft = { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.15 };
var lightPosTopRight = { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.15 };
var lightPosBottom = { hmin: 0.15, hmax: 0.85, vmin: 0.8, vmax: 1.0 };
var lightPosBottomLeft = { hmin: 0, hmax: 0.15, vmin: 0.85, vmax: 1.0 };
var lightPosBottomRight = { hmin: 0.85, hmax: 1.0, vmin: 0.85, vmax: 1.0 };
var lightPosLeft = { hmin: 0, hmax: 0.15, vmin: 0.15, vmax: 0.85 };
var lightPosLeftTop = { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.5 };
var lightPosLeftMiddle = { hmin: 0, hmax: 0.15, vmin: 0.25, vmax: 0.75 };
var lightPosLeftBottom = { hmin: 0, hmax: 0.15, vmin: 0.5, vmax: 1.0 };
var lightPosRight = { hmin: 0.85, hmax: 1.0, vmin: 0.15, vmax: 0.85 };
var lightPosRightTop = { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.5 };
var lightPosRightMiddle = { hmin: 0.85, hmax: 1.0, vmin: 0.25, vmax: 0.75 };
var lightPosRightBottom = { hmin: 0.85, hmax: 1.0, vmin: 0.5, vmax: 1.0 };
var lightPosEntire = { hmin: 0.0, hmax: 1.0, vmin: 0.0, vmax: 1.0 };

var lightPosBottomLeft14 = { hmin: 0, hmax: 0.25, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft12 = { hmin: 0.25, hmax: 0.5, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft34 = { hmin: 0.5, hmax: 0.75, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft11 = { hmin: 0.75, hmax: 1, vmin: 0.85, vmax: 1.0 };

var lightPosBottomLeft112 = { hmin: 0, hmax: 0.5, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft121 = { hmin: 0.5, hmax: 1, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeftNewMid = { hmin: 0.25, hmax: 0.75, vmin: 0.85, vmax: 1.0 };

var lightPosTopLeft112 = { hmin: 0, hmax: 0.5, vmin: 0, vmax: 0.15 };
var lightPosTopLeft121 = { hmin: 0.5, hmax: 1, vmin: 0, vmax: 0.15 };
var lightPosTopLeftNewMid = { hmin: 0.25, hmax: 0.75, vmin: 0, vmax: 0.15 };

function assignLightPos(pos, name) {
  var i = null;

  if (pos === "top")
    i = lightPosTop;
  else if (pos === "topleft")
    i = lightPosTopLeft;
  else if (pos === "topright")
    i = lightPosTopRight;
  else if (pos === "bottom")
    i = lightPosBottom;
  else if (pos === "bottomleft")
    i = lightPosBottomLeft;
  else if (pos === "bottomright")
    i = lightPosBottomRight;
  else if (pos === "left")
    i = lightPosLeft;
  else if (pos === "lefttop")
    i = lightPosLeftTop;
  else if (pos === "leftmiddle")
    i = lightPosLeftMiddle;
  else if (pos === "leftbottom")
    i = lightPosLeftBottom;
  else if (pos === "right")
    i = lightPosRight;
  else if (pos === "righttop")
    i = lightPosRightTop;
  else if (pos === "rightmiddle")
    i = lightPosRightMiddle;
  else if (pos === "rightbottom")
    i = lightPosRightBottom;
  else if (pos === "lightPosBottomLeft14")
    i = lightPosBottomLeft14;
  else if (pos === "lightPosBottomLeft12")
    i = lightPosBottomLeft12;
  else if (pos === "lightPosBottomLeft34")
    i = lightPosBottomLeft34;
  else if (pos === "lightPosBottomLeft11")
    i = lightPosBottomLeft11;
  else if (pos === "lightPosBottomLeft112")
    i = lightPosBottomLeft112;
  else if (pos === "lightPosBottomLeft121")
    i = lightPosBottomLeft121;
  else if (pos === "lightPosBottomLeftNewMid")
    i = lightPosBottomLeftNewMid;
  else if (pos === "lightPosTopLeft112")
    i = lightPosTopLeft112;
  else if (pos === "lightPosTopLeft121")
    i = lightPosTopLeft121;
  else if (pos === "lightPosTopLeftNewMid")
    i = lightPosTopLeftNewMid;
  else
    i = lightPosEntire;

  i.name = name;
  return i;
}

function getHostInLights(hostname) {
  return lights.filter(
    function (lights) {
      return lights.host === hostname
    }
  );
}

function getIpInLights(ip) {
  return lights.filter(
    function (lights) {
      return lights.ip === ip
    }
  );
}

function getIdInLights(id) {
  return lights.filter(
    function (lights) {
      return lights.id === id
    }
  );
}

// External properties properties, 2-dimensional arry of [ledType][key]
devicesProperties = {};

//****************************
// Wizard Philips Hue
//****************************

var hueIPs = [];
var hueIPsinc = 0;
var hueLights = [];
var hueEntertainmentConfigs = [];
var hueEntertainmentServices = [];
var lightLocation = [];
var groupLights = [];
var groupChannels = [];
var groupLightsLocations = [];
var isAPIv2Ready = true;
var isEntertainmentReady = true;

function startWizardPhilipsHue(e) {
  //create html

  var hue_title = 'wiz_hue_title';
  var hue_intro1 = 'wiz_hue_e_intro1';
  var hue_desc1 = 'wiz_hue_desc1';
  var hue_create_user = 'wiz_hue_create_user';

  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(hue_title));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(hue_title) + '</h4><p>' + $.i18n(hue_intro1) + '</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  $('#wizp2_body').html('<div id="wh_topcontainer"></div>');

  var topContainer_html = '<p class="text-left" style="font-weight:bold">' + $.i18n(hue_desc1) + '</p>' +
    '<div class="row">' +
    '<div class="col-md-2">' +
    '  <p class="text-left">' + $.i18n('wiz_hue_ip') + '</p></div>' +
    '  <div class="col-md-7"><div class="input-group">' +
    '    <span class="input-group-addon" id="retry_bridge" style="cursor:pointer"><i class="fa fa-refresh"></i></span>' +
    '    <select id="hue_bridge_select" class="hue_bridge_sel_watch form-control">' + '</select>' + '</div></div>' +
    '  <div class="col-md-7"><div class="input-group">' +
    '    <span class="input-group-addon"><i class="fa fa-arrow-right"></i></span>' +
    '    <input type="text" class="input-group form-control" id="host" placeholder="' + $.i18n('wiz_hue_ip') + '"></div></div>';

  if (storedAccess === 'expert') {
    topContainer_html += '<div class="col-md-3"><div class="input-group">' +
      '<span class="input-group-addon">:</span>' +
      '<input type="text" class="input-group form-control" id="port" placeholder="' + $.i18n('edt_conf_general_port_title') + '"></div></div>';
  }

  topContainer_html += '</div><p><span style="font-weight:bold;color:red" id="wiz_hue_ipstate"></span><span style="font-weight:bold;" id="wiz_hue_discovered"></span></p>';
  topContainer_html += '<div class="form-group" id="usrcont" style="display:none"></div>';

  $('#wh_topcontainer').append(topContainer_html);

  $('#usrcont').append('<div class="row"><div class="col-md-2"><p class="text-left">' + $.i18n('wiz_hue_username') + '</p ></div>' +
    '<div class="col-md-7">' +
    '<div class="input-group">' +
    '  <span class="input-group-addon" id="retry_usr" style="cursor:pointer"><i class="fa fa-refresh"></i></span>' +
    '  <input type="text" class="input-group form-control" id="user">' +
    '</div></div></div><br>' +
    '</div><input type="hidden" id="groupId">'
  );

  $('#usrcont').append('<div id="hue_client_key_r" class="row"><div class="col-md-2"><p class="text-left">' + $.i18n('wiz_hue_clientkey') +
    '</p></div><div class="col-md-7"><input class="form-control" id="clientkey" type="text"></div></div><br>');

  $('#usrcont').append('<p><span style="font-weight:bold;color:red" id="wiz_hue_usrstate"></span><\p>' +
    '<button type="button" class="btn btn-primary" style="display:none" id="wiz_hue_create_user"> <i class="fa fa-fw fa-plus"></i>' + $.i18n(hue_create_user) + '</button>');

  $('#wizp2_body').append('<div id="hue_grp_ids_t" style="display:none"><p class="text-left" style="font-weight:bold">' + $.i18n('wiz_hue_e_desc2') + '</p></div>');
  createTable("gidsh", "gidsb", "hue_grp_ids_t");
  $('.gidsh').append(createTableRow([$.i18n('edt_dev_spec_groupId_title'), ""], true));

  $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p class="text-left" style="font-weight:bold" id="hue_id_headline">' + $.i18n('wiz_hue_e_desc3') + '</p></div>');

  createTable("lidsh", "lidsb", "hue_ids_t");
  $('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lightid_title'), $.i18n('wiz_pos'), $.i18n('wiz_identify')], true));
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_save') + '</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  $('#wizp3_body').html('<span>' + $.i18n('wiz_hue_press_link') + '</span> <br /><br /><center><span id="connectionTime"></span><br /><i class="fa fa-cog fa-spin" style="font-size:100px"></i></center>');

  if (getStorage("darkMode") == "on")
    $('#wizard_logo').attr("src", 'img/hyperion/logo_negativ.png');

  //open modal
  $("#wizard_modal").modal({
    backdrop: "static",
    keyboard: false,
    show: true
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click', function () {
    beginWizardHue();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function checkHueBridge(cb, hueUser) {
  var usr = (typeof hueUser != "undefined") ? hueUser : 'config';
  if (usr === 'config') {
    $('#wiz_hue_discovered').html("");
  }

  if (hueIPs[hueIPsinc]) {
    var host = hueIPs[hueIPsinc].host;
    var port = hueIPs[hueIPsinc].port;

    if (usr != '')
    {
      getProperties_hue_bridge(cb, decodeURIComponent(host), port, usr);
    }
    else
    {
      cb(false, usr);
    }

    if (isAPIv2Ready) {
      $('#port').val(443);
    }
  }
}

function checkBridgeResult(reply, usr) {
  if (reply) {
    //abort checking, first reachable result is used
    $('#wiz_hue_ipstate').html("");
    $('#host').val(hueIPs[hueIPsinc].host)
    $('#port').val(hueIPs[hueIPsinc].port)

    $('#usrcont').toggle(true);

    checkHueBridge(checkUserResult, $('#user').val());
  }
  else {
    $('#usrcont').toggle(false);
    $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
  }
};

function checkUserResult(reply, username) {
  $('#usrcont').toggle(true);

  var hue_create_user = 'wiz_hue_e_create_user';
  if (!isEntertainmentReady) {
    hue_create_user = 'wiz_hue_create_user';
    $('#hue_client_key_r').toggle(false);
  } else {
    $('#hue_client_key_r').toggle(true);
  }

  $('#wiz_hue_create_user').text($.i18n(hue_create_user));
  $('#wiz_hue_create_user').toggle(true);

  if (reply) {
    $('#user').val(username);

    if (isEntertainmentReady && $('#clientkey').val() == "") {
      $('#wiz_hue_usrstate').html($.i18n('wiz_hue_e_clientkey_needed'));
      $('#wiz_hue_create_user').toggle(true);
    } else {
      $('#wiz_hue_usrstate').html("");
      $('#wiz_hue_create_user').toggle(false);

      if (isEntertainmentReady) {
        $('#hue_id_headline').text($.i18n('wiz_hue_e_desc3'));
        $('#hue_grp_ids_t').toggle(true);

        get_hue_groups(username);

      } else {
        $('#hue_id_headline').text($.i18n('wiz_hue_desc2'));
        $('#hue_grp_ids_t').toggle(false);

        get_hue_lights(username);

      }
    }
  }
  else {
    //abort checking, first reachable result is used
    $('#wiz_hue_usrstate').html($.i18n('wiz_hue_failure_user'));
    $('#wiz_hue_create_user').toggle(true);
  }
};

function useGroupId(id, username) {
  $('#groupId').val(hueEntertainmentConfigs[id].id);
  if (isAPIv2Ready) {
    var group = hueEntertainmentConfigs[id];

    groupLights = [];
    for (const light of group.light_services) {
      groupLights.push(light.rid);
    }

    groupChannels = [];
    for (const channel of group.channels) {
      groupChannels.push(channel);
    }

    groupLightsLocations = [];
    for (const location of group.locations.service_locations) {
      groupLightsLocations.push(location);
    }
  } else {
    //Ensure ligthIDs are strings
    groupLights = hueEntertainmentConfigs[id].lights.map(num => {
      return String(num);
    });

    var lightLocations = hueEntertainmentConfigs[id].locations;
    for (var locationID in lightLocations) {
      var lightLocation = {};

      let position = {
        x: lightLocations[locationID][0],
        y: lightLocations[locationID][1],
        z: lightLocations[locationID][2]
      };
      lightLocation.position = position;

      groupLightsLocations.push(lightLocation);
    }
  }

  get_hue_lights(username);
}

function updateBridgeDetails(properties) {
  var ledDeviceProperties = properties.config;

  if (!jQuery.isEmptyObject(ledDeviceProperties)) {
    isEntertainmentReady = properties.isEntertainmentReady;
    isAPIv2Ready = properties.isAPIv2Ready;

    if (ledDeviceProperties.name && ledDeviceProperties.bridgeid && ledDeviceProperties.modelid) {
      $('#wiz_hue_discovered').html(
        "Bridge: " + ledDeviceProperties.name +
        ", Modelid: " + ledDeviceProperties.modelid +
        ", Firmware: " + ledDeviceProperties.swversion + "<br/>" +
        "API-Version: " + ledDeviceProperties.apiversion +
        ", Entertainment: " + (isEntertainmentReady ? "&#10003;" : "-") +
        ", APIv2: " + (isAPIv2Ready ? "&#10003;" : "-")
      );
    }
  }
}

async function discover_hue_bridges() {
  $('#wiz_hue_ipstate').html($.i18n('edt_dev_spec_devices_discovery_inprogress'));

  //  $('#wiz_hue_discovered').html("")
  const res = await requestLedDeviceDiscovery('philipshue');
  if (res && !res.error) {
    const r = res.info;

    // Process devices returned by discovery
    if (r.devices.length == 0) {
      $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
      $('#wiz_hue_discovered').html("")
    }
    else {
      hueIPs = [];
      hueIPsinc = 0;

      var discoveryMethod = "ssdp";
      if (res.info.discoveryMethod) {
        discoveryMethod = res.info.discoveryMethod;
      }

      for (const device of r.devices) {
        if (device) {
          var host;
          var port;
          if (discoveryMethod === "ssdp") {
            if (device.hostname && device.domain) {
              host = device.hostname + "." + device.domain;
              port = device.port;
            } else {
              host = device.ip;
              port = device.port;
            }
          } else {
            host = device.service;
            port = device.port;
          }
          if (host) {

            if (!hueIPs.some(item => item.host === host)) {
              hueIPs.push({ host: host, port: port });
            }
          }
        }
      }

      $('#wiz_hue_ipstate').html("");
      $('#host').val(hueIPs[hueIPsinc].host)
      $('#port').val(hueIPs[hueIPsinc].port)

      $('#hue_bridge_select').html("");

      for (var key in hueIPs) {
        $('#hue_bridge_select').append(createSelOpt(key, hueIPs[key].host));
      }

      $('.hue_bridge_sel_watch').on("click", function () {
        hueIPsinc = $(this).val();

        var name = $("#hue_bridge_select option:selected").text();
        $('#host').val(name);
        $('#port').val(hueIPs[hueIPsinc].port)

        var usr = $('#user').val();
        if (usr != "") {
          checkHueBridge(checkUserResult, usr);
        } else {
          checkHueBridge(checkBridgeResult);
        }
      });

      $('.hue_bridge_sel_watch').click();
    }
  }
}

async function getProperties_hue_bridge(cb, hostAddress, port, username, resourceFilter) {
  let params = { host: hostAddress, username: username, filter: resourceFilter };
  if (port !== 'undefined') {
    params.port = parseInt(port);
  }

  var ledType = 'philipshue';
  var key = hostAddress;

  //Create ledType cache entry
  if (!devicesProperties[ledType]) {
    devicesProperties[ledType] = {};
  }

  // Use device's properties, if properties in chache
  if (devicesProperties[ledType][key] && devicesProperties[ledType][key][username]) {
    updateBridgeDetails(devicesProperties[ledType][key]);
    cb(true, username);
  } else {
    const res = await requestLedDeviceProperties(ledType, params);
    if (res && !res.error) {
      var ledDeviceProperties = res.info.properties;
      if (!jQuery.isEmptyObject(ledDeviceProperties)) {

        devicesProperties[ledType][key] = {};
        devicesProperties[ledType][key][username] = ledDeviceProperties;

        isAPIv2Ready = res.info.isAPIv2Ready;
        devicesProperties[ledType][key].isAPIv2Ready = isAPIv2Ready;
        isEntertainmentReady = res.info.isEntertainmentReady;
        devicesProperties[ledType][key].isEntertainmentReady = isEntertainmentReady;

        updateBridgeDetails(devicesProperties[ledType][key]);
        if (username === "config") {
          cb(true);
        } else {
          cb(true, username);
        }
      } else {
        cb(false, username);
      }
    } else {
      cb(false, username);
    }
  }
}

async function identify_hue_device(hostAddress, port, username, name, id, id_v1) {
  var disabled = $('#btn_wiz_save').is(':disabled');
  // Take care that new record cannot be save during background process
  $('#btn_wiz_save').prop('disabled', true);

  let params = { host: decodeURIComponent(hostAddress), username: username, lightName: decodeURIComponent(name), lightId: id, lightId_v1: id_v1 };

  if (port !== 'undefined') {
    params.port = parseInt(port);
  }

  await requestLedDeviceIdentification('philipshue', params);

  if (!window.readOnlyMode) {
    $('#btn_wiz_save').prop('disabled', disabled);
  }
}

//return editor Value
function eV(vn, defaultVal = "") {
  var editor = (vn) ? conf_editor.getEditor("root.specificOptions." + vn) : null;
  return (editor == null) ? defaultVal : ((defaultVal != "" && !isNaN(defaultVal) && isNaN(editor.getValue())) ? defaultVal : editor.getValue());
}

function beginWizardHue() {
  var usr = eV("username");
  if (usr != "") {
    $('#user').val(usr);
  }

  var clkey = eV("clientkey");
  if (clkey != "") {
    $('#clientkey').val(clkey);
  }

  //check if host is empty/reachable/search for bridge
  if (eV("host") == "") {
    hueIPs = [];
    hueIPsinc = 0;

    discover_hue_bridges();
  }
  else {
    var host = eV("host");
    $('#host').val(host);

    var port = eV("port");
    if (port > 0) {
      $('#port').val(port);
    }
    else {
      $('#port').val('');
    }
    hueIPs.push({ host: host, port: port });

    if (usr != "") {
      checkHueBridge(checkUserResult, usr);
    } else {
      checkHueBridge(checkBridgeResult);
    }
  }

  $('#retry_bridge').off().on('click', function () {
    var host = $('#host').val();
    var port = parseInt($('#port').val());

    if (host != "") {

      var idx = hueIPs.findIndex(item => item.host === host && item.port === port);
      if (idx === -1) {
        hueIPs.push({ host: host, port: port });
        hueIPsinc = hueIPs.length - 1;
      } else {
        hueIPsinc = idx;
      }
    }
    else {
      discover_hue_bridges();
    }

    var usr = $('#user').val();
    if (usr != "") {
      checkHueBridge(checkUserResult, usr);
    } else {
      checkHueBridge(checkBridgeResult);
    }
  });

  $('#retry_usr').off().on('click', function () {
    checkHueBridge(checkUserResult, $('#user').val());
  });

  $('#wiz_hue_create_user').off().on('click', function () {
    createHueUser();
  });

  function assignLightEntertainmentPos(isFocusCenter, position, name, id) {

    var x = position.x;
    var z = position.z;

    if (isFocusCenter) {
      // Map lights as in centered range -0.5 to 0.5
      if (x < -0.5) {
        x = -0.5;
      } else if (x > 0.5) {
        x = 0.5;
      }
      if (z < -0.5) {
        z = -0.5;
      } else if (z > 0.5) {
        z = 0.5;
      }
    } else {
      // Map lights as in full range -1 to 1
      x /= 2;
      z /= 2;
    }

    var h = x + 0.5;
    var v = -z + 0.5;

    var hmin = h - 0.05;
    var hmax = h + 0.05;
    var vmin = v - 0.05;
    var vmax = v + 0.05;

    let layoutObject = {
      hmin: hmin < 0 ? 0 : hmin,
      hmax: hmax > 1 ? 1 : hmax,
      vmin: vmin < 0 ? 0 : vmin,
      vmax: vmax > 1 ? 1 : vmax,
      name: name
    };

    if (id) {
      layoutObject.name += "_" + id;
    }
    return layoutObject;
  }

  function assignSegmentedLightPos(segment, position, name) {
    var layoutObjects = [];

    var segTotalLength = 0;
    for (var key in segment) {

      segTotalLength += segment[key].length;
    }

    var min;
    var max;
    var horizontal = true;

    var layoutObject = assignLightPos(position, name);
    if (position === "left" || position === "right") {
      // vertical distribution
      min = layoutObject.vmin;
      max = layoutObject.vmax;
      horizontal = false;

    } else {
      // horizontal distribution
      min = layoutObject.hmin;
      max = layoutObject.hmax;
    }

    var step = (max - min) / segTotalLength;
    var start = min;

    for (var key in segment) {
      min = start;
      max = round(start + segment[key].length * step);

      if (horizontal) {
        layoutObject.hmin = min;
        layoutObject.hmax = max;
      } else {
        layoutObject.vmin = min;
        layoutObject.vmax = max;
      }
      layoutObject.name = name + "_" + key;
      layoutObjects.push(JSON.parse(JSON.stringify(layoutObject)));

      start = max;
    }

    return layoutObjects;
  }

  $('#btn_wiz_save').off().on("click", function () {
    var hueLedConfig = [];
    var finalLightIds = [];
    var channelNumber = 0;

    //create hue led config
    for (var key in groupLights) {
      var lightId = groupLights[key];

      if ($('#hue_' + lightId).val() != "disabled") {
        finalLightIds.push(lightId);

        var lightName;
        if (isAPIv2Ready) {
          var light = hueLights.find(light => light.id === lightId);
          lightName = light.metadata.name;
        } else {
          lightName = hueLights[lightId].name;
        }

        var position = $('#hue_' + lightId).val();
        var lightIdx = groupLights.indexOf(lightId);
        var lightLocation = groupLightsLocations[lightIdx];

        var serviceID;
        if (isAPIv2Ready) {
          serviceID = lightLocation.service.rid;
        }

        if (position.startsWith("entertainment")) {

          // Layout per entertainment area definition at bridge
          var isFocusCenter = false;
          if (position === "entertainment_center") {
            isFocusCenter = true;
          }

          if (isAPIv2Ready) {

            groupChannels.forEach((channel) => {
              if (channel.members[0].service.rid === serviceID) {
                var layoutObject = assignLightEntertainmentPos(isFocusCenter, channel.position, lightName, channel.channel_id);
                hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
                ++channelNumber;
              }
            });
          } else {
            var layoutObject = assignLightEntertainmentPos(isFocusCenter, lightLocation.position, lightName);
            hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
          }
        }
        else {
          // Layout per manual settings
          var maxSegments = 1;

          if (isAPIv2Ready) {
            var service = hueEntertainmentServices.find(service => service.id === serviceID);
            maxSegments = service.segments.max_segments;
          }

          if (maxSegments > 1) {
            var segment = service.segments.segments;
            var layoutObjects = assignSegmentedLightPos(segment, position, lightName);
            hueLedConfig.push(...layoutObjects);
          } else {
            var layoutObject = assignLightPos(position, lightName);
            hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
          }
          channelNumber += maxSegments;
        }
      }
    }

    var sc = window.serverConfig;
    sc.leds = hueLedConfig;

    //Adjust gamma, brightness and compensation
    var c = sc.color.channelAdjustment[0];
    c.gammaBlue = 1.0;
    c.gammaRed = 1.0;
    c.gammaGreen = 1.0;
    c.brightness = 100;
    c.brightnessCompensation = 0;

    //device config

    //Start with a clean configuration
    var d = {};
    d.host = $('#host').val();
    d.port = parseInt($('#port').val());
    d.username = $('#user').val();
    d.type = 'philipshue';
    d.colorOrder = 'rgb';
    d.lightIds = finalLightIds;
    d.transitiontime = parseInt(eV("transitiontime", 1));
    d.restoreOriginalState = (eV("restoreOriginalState", false) == true);
    d.switchOffOnBlack = (eV("switchOffOnBlack", false) == true);

    d.blackLevel = parseFloat(eV("blackLevel", 0.009));
    d.onBlackTimeToPowerOff = parseInt(eV("onBlackTimeToPowerOff", 600));
    d.onBlackTimeToPowerOn = parseInt(eV("onBlackTimeToPowerOn", 300));
    d.brightnessFactor = parseFloat(eV("brightnessFactor", 1));

    d.clientkey = $('#clientkey').val();
    d.groupId = $('#groupId').val();
    d.blackLightsTimeout = parseInt(eV("blackLightsTimeout", 5000));
    d.brightnessMin = parseFloat(eV("brightnessMin", 0));
    d.brightnessMax = parseFloat(eV("brightnessMax", 1));
    d.brightnessThreshold = parseFloat(eV("brightnessThreshold", 0.0001));
    d.handshakeTimeoutMin = parseInt(eV("handshakeTimeoutMin", 300));
    d.handshakeTimeoutMax = parseInt(eV("handshakeTimeoutMax", 1000));
    d.verbose = (eV("verbose") == true);

    d.autoStart = conf_editor.getEditor("root.generalOptions.autoStart").getValue();
    d.enableAttempts = parseInt(conf_editor.getEditor("root.generalOptions.enableAttempts").getValue());
    d.enableAttemptsInterval = parseInt(conf_editor.getEditor("root.generalOptions.enableAttemptsInterval").getValue());

    d.useEntertainmentAPI = isEntertainmentReady;
    d.useAPIv2 = isAPIv2Ready;

    if (isEntertainmentReady) {
      d.hardwareLedCount = channelNumber;
      if (window.serverConfig.device.type !== d.type) {
        //smoothing on, if new device
        sc.smoothing = { enable: true };
      }
    } else {
      d.hardwareLedCount = finalLightIds.length;
      d.verbose = false;
      if (window.serverConfig.device.type !== d.type) {
        //smoothing off, if new device
        sc.smoothing = { enable: false };
      }
    }

    window.serverConfig.device = d;

    requestWriteConfig(sc, true);
    resetWizard();
  });

  $('#btn_wiz_abort').off().on('click', resetWizard);
}

function createHueUser() {
  var host = hueIPs[hueIPsinc].host;
  var port = hueIPs[hueIPsinc].port;

  let params = { host: host };
  if (port !== 'undefined') {
    params.port = parseInt(port);
  }

  var retryTime = 30;
  var retryInterval = 2;

  var UserInterval = setInterval(function () {

    $('#wizp1').toggle(false);
    $('#wizp2').toggle(false);
    $('#wizp3').toggle(true);

    (async () => {

      retryTime -= retryInterval;
      $("#connectionTime").html(retryTime);
      if (retryTime <= 0) {
        abortConnection(UserInterval);
        clearInterval(UserInterval);
      }
      else {
        const res = await requestLedDeviceAddAuthorization('philipshue', params);
        if (res && !res.error) {
          var response = res.info;

          if (jQuery.isEmptyObject(response)) {
            debugMessage(retryTime + ": link button not pressed or device not reachable");
          } else {
            $('#wizp1').toggle(false);
            $('#wizp2').toggle(true);
            $('#wizp3').toggle(false);

            var username = response.username;
            if (username != 'undefined') {
              $('#user').val(username);
              conf_editor.getEditor("root.specificOptions.username").setValue(username);
              conf_editor.getEditor("root.specificOptions.host").setValue(host);
              conf_editor.getEditor("root.specificOptions.port").setValue(port);
            }

            if (isEntertainmentReady) {
              var clientkey = response.clientkey;
              if (clientkey != 'undefined') {
                $('#clientkey').val(clientkey);
                conf_editor.getEditor("root.specificOptions.clientkey").setValue(clientkey);
              }
            }
            checkHueBridge(checkUserResult, username);
            clearInterval(UserInterval);
          }
        } else {
          $('#wizp1').toggle(false);
          $('#wizp2').toggle(true);
          $('#wizp3').toggle(false);
          clearInterval(UserInterval);
        }
      }
    })();

  }, retryInterval * 1000);
}

function get_hue_groups(username) {
  var host = hueIPs[hueIPsinc].host;

  if (devicesProperties['philipshue'][host] && devicesProperties['philipshue'][host][username]) {
    var ledProperties = devicesProperties['philipshue'][host][username];

    if (isAPIv2Ready) {
      if (!jQuery.isEmptyObject(ledProperties.data)) {
        if (Object.keys(ledProperties.data).length > 0) {
          hueEntertainmentConfigs = ledProperties.data.filter(config => {
            return config.type === "entertainment_configuration";
          });
          hueEntertainmentServices = ledProperties.data.filter(config => {
            return (config.type === "entertainment" && config.renderer === true);
          });
        }
      }
    } else {
      if (!jQuery.isEmptyObject(ledProperties.groups)) {
        hueEntertainmentConfigs = [];
        var hueGroups = ledProperties.groups;
        for (var groupid in hueGroups) {
          if (hueGroups[groupid].type == 'Entertainment') {
            hueGroups[groupid].id = groupid;
            hueEntertainmentConfigs.push(hueGroups[groupid]);
          }
        }
      }
    }

    if (Object.keys(hueEntertainmentConfigs).length > 0) {

      $('.lidsb').html("");
      $('#wh_topcontainer').toggle(false);
      $('#hue_grp_ids_t').toggle(true);

      for (var groupid in hueEntertainmentConfigs) {
        $('.gidsb').append(createTableRow([groupid + ' (' + hueEntertainmentConfigs[groupid].name + ')', '<button class="btn btn-sm btn-primary" onClick=useGroupId("' + groupid + '","' + username + '")>' + $.i18n('wiz_hue_e_use_group') + '</button>']));
      }
    } else {
      noAPISupport('wiz_hue_e_noegrpids', username);
    }
  }
}

function noAPISupport(txt, username) {
  showNotification('danger', $.i18n('wiz_hue_e_title'), $.i18n('wiz_hue_e_noapisupport_hint'));
  conf_editor.getEditor("root.specificOptions.useEntertainmentAPI").setValue(false);
  $("#root_specificOptions_useEntertainmentAPI").trigger("change");
  $('#btn_wiz_holder').append('<div class="bs-callout bs-callout-danger" style="margin-top:0px">' + $.i18n('wiz_hue_e_noapisupport_hint') + '</div>');
  $('#hue_grp_ids_t').toggle(false);
  var txt = (txt) ? $.i18n(txt) : $.i18n('wiz_hue_e_nogrpids');
  $('<p style="font-weight:bold;color:red;">' + txt + '<br />' + $.i18n('wiz_hue_e_noapisupport') + '</p>').insertBefore('#wizp2_body #hue_ids_t');
  $('#hue_id_headline').html($.i18n('wiz_hue_desc2'));

  get_hue_lights(username);
}

function get_hue_lights(username) {
  var host = hueIPs[hueIPsinc].host;

  if (devicesProperties['philipshue'][host] && devicesProperties['philipshue'][host][username]) {
    var ledProperties = devicesProperties['philipshue'][host][username];

    if (isAPIv2Ready) {
      if (!jQuery.isEmptyObject(ledProperties.data)) {
        if (Object.keys(ledProperties.data).length > 0) {
          hueLights = ledProperties.data.filter(config => {
            return config.type === "light";
          });
        }
      }
    } else {
      if (!jQuery.isEmptyObject(ledProperties.lights)) {
        hueLights = ledProperties.lights;
      }
    }

    if (Object.keys(hueLights).length > 0) {
      if (!isEntertainmentReady) {
        $('#wh_topcontainer').toggle(false);
      }
      $('#hue_ids_t, #btn_wiz_save').toggle(true);

      var lightOptions = [
        "top", "topleft", "topright",
        "bottom", "bottomleft", "bottomright",
        "left", "lefttop", "leftmiddle", "leftbottom",
        "right", "righttop", "rightmiddle", "rightbottom",
        "entire",
        "lightPosTopLeft112", "lightPosTopLeftNewMid", "lightPosTopLeft121",
        "lightPosBottomLeft14", "lightPosBottomLeft12", "lightPosBottomLeft34", "lightPosBottomLeft11",
        "lightPosBottomLeft112", "lightPosBottomLeftNewMid", "lightPosBottomLeft121"
      ];

      if (isEntertainmentReady) {
        lightOptions.unshift("entertainment_center");
        lightOptions.unshift("entertainment");
      } else {
        lightOptions.unshift("disabled");
        groupLights = Object.keys(hueLights);
      }

      $('.lidsb').html("");

      var pos = "";
      for (var id in groupLights) {
        var lightId = groupLights[id];
        var lightId_v1 = "/lights/" + lightId;

        var lightName;
        if (isAPIv2Ready) {
          var light = hueLights.find(light => light.id === lightId);
          lightName = light.metadata.name;
          lightId_v1 = light.id_v1;
        } else {
          lightName = hueLights[lightId].name;
        }

        if (isEntertainmentReady) {
          var lightLocation = {};
          lightLocation = groupLightsLocations[id];
          if (lightLocation) {
            if (isAPIv2Ready) {
              pos = 0;
            } else {
              var x = lightLocation.position.x;
              var y = lightLocation.position.y;
              var z = lightLocation.position.z;

              var xval = (x < 0) ? "left" : "right";
              if (z != 1 && x >= -0.25 && x <= 0.25) xval = "";
              switch (z) {
                case 1: // top / Ceiling height
                  pos = "top" + xval;
                  break;
                case 0: // middle / TV height
                  pos = (xval == "" && y >= 0.75) ? "bottom" : xval + "middle";
                  break;
                case -1: // bottom / Ground height
                  pos = xval + "bottom";
                  break;
              }
            }
          }
        }

        var options = "";
        for (var opt in lightOptions) {
          var val = lightOptions[opt];
          var txt = (val != 'entire' && val != 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
          options += '<option value="' + val + '"';
          if (pos == val) options += ' selected="selected"';
          options += '>' + $.i18n(txt + val) + '</option>';
        }

        $('.lidsb').append(createTableRow([id + ' (' + lightName + ')', '<select id="hue_' + lightId + '" class="hue_sel_watch form-control">'
          + options
          + '</select>', '<button class="btn btn-sm btn-primary" onClick=identify_hue_device("' + encodeURIComponent($("#host").val()) + '","' + $('#port').val() + '","' + $("#user").val() + '","' + encodeURIComponent(lightName) + '","' + lightId + '","' + lightId_v1 + '")>' + $.i18n('wiz_hue_blinkblue', id) + '</button>']));
      }

      if (!isEntertainmentReady) {
        $('.hue_sel_watch').on("change", function () {
          var cC = 0;
          for (var key in hueLights) {
            if ($('#hue_' + key).val() != "disabled") {
              cC++;
            }
          }

          (cC == 0 || window.readOnlyMode) ? $('#btn_wiz_save').prop("disabled", true) : $('#btn_wiz_save').prop("disabled", false);
        });
      }
      $('.hue_sel_watch').trigger('change');
    }
    else {
      var txt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_hue_noids') + '</p>';
      $('#wizp2_body').append(txt);
    }
  }
}

function abortConnection(UserInterval) {
  clearInterval(UserInterval);
  $('#wizp1').toggle(false);
  $('#wizp2').toggle(true);
  $('#wizp3').toggle(false);
  $("#wiz_hue_usrstate").html($.i18n('wiz_hue_failure_connection'));
}

//****************************
// Wizard Yeelight
//****************************
var lights = null;
function startWizardYeelight(e) {
  //create html

  var yeelight_title = 'wiz_yeelight_title';
  var yeelight_intro1 = 'wiz_yeelight_intro1';

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
    beginWizardYeelight();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function beginWizardYeelight() {
  lights = [];
  configuredLights = conf_editor.getEditor("root.specificOptions.lights").getValue();

  discover_yeelight_lights();

  $('#btn_wiz_save').off().on("click", function () {
    var yeelightLedConfig = [];
    var finalLights = [];

    //create yeelight led config
    for (var key in lights) {
      if ($('#yee_' + key).val() !== "disabled") {

        var name = lights[key].name;
        // Set Name to layout-position, if empty
        if (name === "") {
          name = lights[key].host;
        }

        finalLights.push(lights[key]);

        var idx_content = assignLightPos($('#yee_' + key).val(), name);
        yeelightLedConfig.push(JSON.parse(JSON.stringify(idx_content)));
      }
    }

    //LED layout
    window.serverConfig.leds = yeelightLedConfig;

    //LED device config
    var currentDeviceType = window.serverConfig.device.type;

    //Start with a clean configuration
    var d = {};

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

async function discover_yeelight_lights() {
  var light = {};
  // Get discovered lights
  const res = await requestLedDeviceDiscovery('yeelight');

  // TODO: error case unhandled
  // res can be: false (timeout) or res.error (not found)
  if (res && !res.error) {
    const r = res.info;

    var discoveryMethod = "ssdp";
    if (res.info.discoveryMethod) {
      discoveryMethod = res.info.discoveryMethod;
    }

    // Process devices returned by discovery
    for (const device of r.devices) {
      if (device.hostname !== "") {
        if (getHostInLights(device.hostname).length === 0) {
          var light = {};



          if (discoveryMethod === "ssdp") {
            //Create a valid hostname
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
            //Yeelight does not provide correct API port with mDNS response, use default one
            light.port = 55443;
          }
          else {
            light.name = device.other.name;
            light.model = device.other.model;
          }
          lights.push(light);
        }
      }
    }

    // Add additional items from configuration
    for (var keyConfig in configuredLights) {
      var host = configuredLights[keyConfig].host;

      //In case port has been explicitly provided, overwrite port given as part of hostname
      if (configuredLights[keyConfig].port !== 0)
        port = configuredLights[keyConfig].port;

      if (host !== "")
        if (getHostInLights(host).length === 0) {
          var light = {};
          light.host = host;
          light.port = port;
          light.name = configuredLights[keyConfig].name;
          light.model = "color4";
          lights.push(light);
        }
    }

    assign_yeelight_lights();
  }
}

function assign_yeelight_lights() {
  // Model mappings, see https://www.home-assistant.io/integrations/yeelight/
  var models = ['color', 'color1', 'YLDP02YL', 'YLDP02YL', 'color2', 'YLDP06YL', 'color4', 'YLDP13YL', 'color6', 'YLDP13AYL', 'colorb', "YLDP005", 'colorc', "YLDP004-A", 'stripe', 'YLDD04YL', 'strip1', 'YLDD01YL', 'YLDD02YL', 'strip4', 'YLDD05YL', 'strip6', 'YLDD05YL'];

  // If records are left for configuration
  if (Object.keys(lights).length > 0) {
    $('#wh_topcontainer').toggle(false);
    $('#yee_ids_t, #btn_wiz_save').toggle(true);

    var lightOptions = [
      "top", "topleft", "topright",
      "bottom", "bottomleft", "bottomright",
      "left", "lefttop", "leftmiddle", "leftbottom",
      "right", "righttop", "rightmiddle", "rightbottom",
      "entire",
      "lightPosTopLeft112", "lightPosTopLeftNewMid", "lightPosTopLeft121",
      "lightPosBottomLeft14", "lightPosBottomLeft12", "lightPosBottomLeft34", "lightPosBottomLeft11",
      "lightPosBottomLeft112", "lightPosBottomLeftNewMid", "lightPosBottomLeft121"
    ];

    lightOptions.unshift("disabled");

    $('.lidsb').html("");
    var pos = "";

    for (var lightid in lights) {
      var lightHostname = lights[lightid].host;
      var lightPort = lights[lightid].port;
      var lightName = lights[lightid].name;

      if (lightName === "")
        lightName = $.i18n('edt_dev_spec_lights_itemtitle') + '(' + lightHostname + ')';

      var options = "";
      for (var opt in lightOptions) {
        var val = lightOptions[opt];
        var txt = (val !== 'entire' && val !== 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
        options += '<option value="' + val + '"';
        if (pos === val) options += ' selected="selected"';
        options += '>' + $.i18n(txt + val) + '</option>';
      }

      var enabled = 'enabled';
      if (!models.includes(lights[lightid].model)) {
        var enabled = 'disabled';
        options = '<option value=disabled>' + $.i18n('wiz_yeelight_unsupported') + '</option>';
      }

      $('.lidsb').append(createTableRow([(parseInt(lightid, 10) + 1) + '. ' + lightName, '<select id="yee_' + lightid + '" ' + enabled + ' class="yee_sel_watch form-control">'
        + options
        + '</select>', '<button class="btn btn-sm btn-primary" onClick=identify_yeelight_device("' + lightHostname + '",' + lightPort + ')>'
        + $.i18n('wiz_identify') + '</button>']));
    }

    $('.yee_sel_watch').on("change", function () {
      var cC = 0;
      for (var key in lights) {
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
    var noLightsTxt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_noLights', 'Yeelights') + '</p>';
    $('#wizp2_body').append(noLightsTxt);
  }
}

async function getProperties_yeelight(host, port) {
  let params = { host: host, port: port };

  const res = await requestLedDeviceProperties('yeelight', params);

  // TODO: error case unhandled
  // res can be: false (timeout) or res.error (not found)
  if (res && !res.error) {
    const r = res.info
    console.log("Yeelight properties: ", r);
  }
}

async function identify_yeelight_device(host, port) {

  var disabled = $('#btn_wiz_save').is(':disabled');

  // Take care that new record cannot be save during background process
  $('#btn_wiz_save').prop('disabled', true);

  let params = { host: host, port: port };
  await requestLedDeviceIdentification("yeelight", params);

  if (!window.readOnlyMode) {
    $('#btn_wiz_save').prop('disabled', disabled);
  }
}

//****************************
// Wizard AtmoOrb
//****************************
var lights = null;
function startWizardAtmoOrb(e) {
  //create html

  var atmoorb_title = 'wiz_atmoorb_title';
  var atmoorb_intro1 = 'wiz_atmoorb_intro1';

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
    beginWizardAtmoOrb();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function beginWizardAtmoOrb() {
  lights = [];
  configuredLights = [];

  var configruedOrbIds = conf_editor.getEditor("root.specificOptions.orbIds").getValue().trim();
  if (configruedOrbIds.length !== 0) {
    configuredLights = configruedOrbIds.split(",").map(Number);
  }

  var multiCastGroup = conf_editor.getEditor("root.specificOptions.host").getValue();
  var multiCastPort = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());

  discover_atmoorb_lights(multiCastGroup, multiCastPort);

  $('#btn_wiz_save').off().on("click", function () {
    var atmoorbLedConfig = [];
    var finalLights = [];

    //create atmoorb led config
    for (var key in lights) {
      if ($('#orb_' + key).val() !== "disabled") {
        // Set Name to layout-position, if empty
        if (lights[key].name === "") {
          lights[key].name = $.i18n('conf_leds_layout_cl_' + $('#orb_' + key).val());
        }

        finalLights.push(lights[key].id);

        var name = lights[key].id;
        if (lights[key].host !== "")
          name += ':' + lights[key].host;

        var idx_content = assignLightPos($('#orb_' + key).val(), name);
        atmoorbLedConfig.push(JSON.parse(JSON.stringify(idx_content)));
      }
    }

    //LED layout
    window.serverConfig.leds = atmoorbLedConfig;

    //LED device config
    //Start with a clean configuration
    var d = {};

    d.type = 'atmoorb';
    d.hardwareLedCount = finalLights.length;
    d.colorOrder = conf_editor.getEditor("root.generalOptions.colorOrder").getValue();

    d.orbIds = finalLights.toString();
    d.useOrbSmoothing = (eV("useOrbSmoothing") == true);

    d.host = conf_editor.getEditor("root.specificOptions.host").getValue();
    d.port = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());
    d.latchTime = parseInt(conf_editor.getEditor("root.specificOptions.latchTime").getValue());;

    window.serverConfig.device = d;

    requestWriteConfig(window.serverConfig, true);
    resetWizard();
  });

  $('#btn_wiz_abort').off().on('click', resetWizard);
}

async function discover_atmoorb_lights(multiCastGroup, multiCastPort) {
  var light = {};

  var params = {};
  if (multiCastGroup !== "") {
    params.multiCastGroup = multiCastGroup;
  }

  if (multiCastPort !== 0) {
    params.multiCastPort = multiCastPort;
  }

  // Get discovered lights
  const res = await requestLedDeviceDiscovery('atmoorb', params);

  // TODO: error case unhandled
  // res can be: false (timeout) or res.error (not found)
  if (res && !res.error) {
    const r = res.info;

    // Process devices returned by discovery
    for (const device of r.devices) {
      if (device.id !== "") {
        if (getIdInLights(device.id).length === 0) {
          var light = {};
          light.id = device.id;
          light.ip = device.ip;
          light.host = device.hostname;
          lights.push(light);
        }
      }
    }

    // Add additional items from configuration
    for (const keyConfig in configuredLights) {
      if (configuredLights[keyConfig] !== "" && !isNaN(configuredLights[keyConfig])) {
        if (getIdInLights(configuredLights[keyConfig]).length === 0) {
          var light = {};
          light.id = configuredLights[keyConfig];
          light.ip = "";
          light.host = "";
          lights.push(light);
        }
      }
    }

    lights.sort((a, b) => (a.id > b.id) ? 1 : -1);

    assign_atmoorb_lights();
  }
}

function assign_atmoorb_lights() {
  // If records are left for configuration
  if (Object.keys(lights).length > 0) {
    $('#wh_topcontainer').toggle(false);
    $('#orb_ids_t, #btn_wiz_save').toggle(true);

    var lightOptions = [
      "top", "topleft", "topright",
      "bottom", "bottomleft", "bottomright",
      "left", "lefttop", "leftmiddle", "leftbottom",
      "right", "righttop", "rightmiddle", "rightbottom",
      "entire",
      "lightPosTopLeft112", "lightPosTopLeftNewMid", "lightPosTopLeft121",
      "lightPosBottomLeft14", "lightPosBottomLeft12", "lightPosBottomLeft34", "lightPosBottomLeft11",
      "lightPosBottomLeft112", "lightPosBottomLeftNewMid", "lightPosBottomLeft121"
    ];

    lightOptions.unshift("disabled");

    $('.lidsb').html("");
    var pos = "";

    for (var lightid in lights) {
      var orbId = lights[lightid].id;
      var orbIp = lights[lightid].ip;
      var orbHostname = lights[lightid].host;

      if (orbHostname === "")
        orbHostname = $.i18n('edt_dev_spec_lights_itemtitle');

      var options = "";
      for (var opt in lightOptions) {
        var val = lightOptions[opt];
        var txt = (val !== 'entire' && val !== 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
        options += '<option value="' + val + '"';
        if (pos === val) options += ' selected="selected"';
        options += '>' + $.i18n(txt + val) + '</option>';
      }

      var enabled = 'enabled';
      if (orbId < 1 || orbId > 255) {
        enabled = 'disabled';
        options = '<option value=disabled>' + $.i18n('wiz_atmoorb_unsupported') + '</option>';
      }

      var lightAnnotation = "";
      if (orbIp !== "") {
        lightAnnotation = ': ' + orbIp + '<br>(' + orbHostname + ')';
      }

      $('.lidsb').append(createTableRow([orbId + lightAnnotation, '<select id="orb_' + lightid + '" ' + enabled + ' class="orb_sel_watch form-control">'
        + options
        + '</select>', '<button class="btn btn-sm btn-primary" ' + enabled + ' onClick=identify_atmoorb_device("' + orbId + '")>'
        + $.i18n('wiz_identify_light', orbId) + '</button>']));
    }

    $('.orb_sel_watch').on("change", function () {
      var cC = 0;
      for (var key in lights) {
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
    var noLightsTxt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_noLights', 'AtmoOrbs') + '</p>';
    $('#wizp2_body').append(noLightsTxt);
  }
}

async function identify_atmoorb_device(orbId) {
  var disabled = $('#btn_wiz_save').is(':disabled');

  // Take care that new record cannot be save during background process
  $('#btn_wiz_save').prop('disabled', true);

  let params = { id: orbId };
  await requestLedDeviceIdentification("atmoorb", params);

  if (!window.readOnlyMode) {
    $('#btn_wiz_save').prop('disabled', disabled);
  }
}

//****************************
// Nanoleaf Token Wizard
//****************************
var lights = null;
function startWizardNanoleafUserAuth(e) {
  //create html
  var nanoleaf_user_auth_title = 'wiz_nanoleaf_user_auth_title';
  var nanoleaf_user_auth_intro = 'wiz_nanoleaf_user_auth_intro';

  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(nanoleaf_user_auth_title));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(nanoleaf_user_auth_title) + '</h4><p>' + $.i18n(nanoleaf_user_auth_intro) + '</p>');

  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'
    + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'
    + $.i18n('general_btn_cancel') + '</button>');

  $('#wizp3_body').html('<span>' + $.i18n('wiz_nanoleaf_press_onoff_button') + '</span> <br /><br /><center><span id="connectionTime"></span><br /><i class="fa fa-cog fa-spin" style="font-size:100px"></i></center>');

  if (getStorage("darkMode") == "on")
    $('#wizard_logo').attr("src", 'img/hyperion/logo_negativ.png');

  //open modal
  $("#wizard_modal").modal({ backdrop: "static", keyboard: false, show: true });

  //listen for continue
  $('#btn_wiz_cont').off().on('click', function () {
    createNanoleafUserAuthorization();
    $('#wizp1').toggle(false);
    $('#wizp3').toggle(true);
  });
}

function createNanoleafUserAuthorization() {
  var host = conf_editor.getEditor("root.specificOptions.host").getValue();

  let params = { host: host };

  var retryTime = 30;
  var retryInterval = 2;

  var UserInterval = setInterval(function () {

    $('#wizp1').toggle(false);
    $('#wizp3').toggle(true);

    (async () => {

      retryTime -= retryInterval;
      $("#connectionTime").html(retryTime);
      if (retryTime <= 0) {
        abortConnection(UserInterval);
        clearInterval(UserInterval);

        showNotification('warning', $.i18n('wiz_nanoleaf_failure_auth_token'), $.i18n('wiz_nanoleaf_failure_auth_token_t'));

        resetWizard(true);
      }
      else {
        const res = await requestLedDeviceAddAuthorization('nanoleaf', params);
        if (res && !res.error) {
          var response = res.info;

          if (jQuery.isEmptyObject(response)) {
            debugMessage(retryTime + ": Power On/Off button not pressed or device not reachable");
          } else {
            $('#wizp1').toggle(false);
            $('#wizp3').toggle(false);

            var token = response.auth_token;
            if (token != 'undefined') {
              conf_editor.getEditor("root.specificOptions.token").setValue(token);
            }
            clearInterval(UserInterval);
            resetWizard(true);
          }
        } else {
          $('#wizp1').toggle(false);
          $('#wizp3').toggle(false);
          clearInterval(UserInterval);
          resetWizard(true);
        }
      }
    })();

  }, retryInterval * 1000);
}

