//****************************
// Wizard Philips Hue
//****************************

import { ledDeviceWizardUtils as utils } from './LedDevice_utils.js';

const philipshueWizard = (() => {

  // External properties, 2-dimensional map of [ledType][key]
  const devicesProperties = new Map();

  let hueIPs = [];
  let hueIPsinc = 0;
  let hueLights = [];
  let hueEntertainmentConfigs = [];
  let hueEntertainmentServices = [];
  let groupLights = [];
  let groupChannels = [];
  let groupLightsLocations = [];
  let isAPIv2Ready = true;
  let isEntertainmentReady = true;

  function checkHueBridge(cb, hueUser) {
    const usr = (typeof hueUser != "undefined") ? hueUser : 'config';
    if (usr === 'config') {
      $('#wiz_hue_discovered').html("");
    }

    if (hueIPs[hueIPsinc]) {
      const host = hueIPs[hueIPsinc].host;
      const port = hueIPs[hueIPsinc].port;
      const bridgeid = hueIPs[hueIPsinc].bridgeid ? hueIPs[hueIPsinc].bridgeid.toUpperCase() : undefined;

      if (usr != '') {
        getProperties(cb, decodeURIComponent(host), port, bridgeid, usr);
      }
      else {
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
      $('#host').val(hueIPs[hueIPsinc].host);
      $('#port').val(hueIPs[hueIPsinc].port);
      $('#bridgeid').val(hueIPs[hueIPsinc].bridgeid);

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

    let hue_create_user = 'wiz_hue_e_create_user';
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
      const group = hueEntertainmentConfigs[id];

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

      const lightLocations = hueEntertainmentConfigs[id].locations;
      for (const locationID in lightLocations) {
        let lightLocation = {};

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

  function assignLightEntertainmentPos(isFocusCenter, position, name, id) {

    let x = position.x;
    let z = position.z;

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

    const h = x + 0.5;
    const v = -z + 0.5;

    const hmin = h - 0.05;
    const hmax = h + 0.05;
    const vmin = v - 0.05;
    const vmax = v + 0.05;

    let layoutObject = {
      hmin: hmin < 0 ? 0 : hmin,
      hmax: hmax > 1 ? 1 : hmax,
      vmin: vmin < 0 ? 0 : vmin,
      vmax: vmax > 1 ? 1 : vmax,
      name: name
    };

    if (id !== undefined && id !== null) {
      layoutObject.name += "_" + id;
    }
    return layoutObject;
  }

  function assignSegmentedLightPos(segment, position, name) {
    let layoutObjects = [];

    let segTotalLength = 0;
    for (const key in segment) {

      segTotalLength += segment[key].length;
    }

    let min;
    let max;
    let horizontal = true;

    let layoutObject = utils.assignLightPos(position, name);
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

    const step = (max - min) / segTotalLength;
    let start = min;

    for (const key in segment) {
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

  function updateBridgeDetails(ledType, key, username) {
    const ledDeviceProperties = getLedDeviceProperty(ledType, key, username);

    if (!jQuery.isEmptyObject(ledDeviceProperties)) {
      isEntertainmentReady = ledDeviceProperties.isEntertainmentReady;
      isAPIv2Ready = ledDeviceProperties.isAPIv2Ready;

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

  async function discover() {
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

        let discoveryMethod = "ssdp";
        if (res.info.discoveryMethod) {
          discoveryMethod = res.info.discoveryMethod;
        }

        for (const device of r.devices) {
          if (device) {
            let host;
            let port;
            let bridgeid;
            if (discoveryMethod === "ssdp") {
              // Use hostname + domain, if available and not an IP address
              if (device.hostname && device.domain && !(isValidIPv4(device.hostname) || isValidIPv6(device.hostname))) {
                host = device.hostname + "." + device.domain;
                port = device.port;
              } else {
                host = device.ip;
                port = device.port;
              }
              bridgeid = device.other?.["hue-bridgeid"]?.toUpperCase();
            } else {
                host = device.service;
                port = device.port;
                bridgeid = device.txt?.["bridgeid"]?.toUpperCase();
            }

            if (host) {
              if (!hueIPs.some(item => item.host === host)) {
                hueIPs.push({ host, port, bridgeid });
              }
            }
          }
        }

        $('#wiz_hue_ipstate').html("");
        $('#host').val(hueIPs[hueIPsinc].host)
        $('#port').val(hueIPs[hueIPsinc].port)
        $('#bridgeid').val(hueIPs[hueIPsinc].bridgeid)

        $('#hue_bridge_select').html("");

        for (const key in hueIPs) {
          $('#hue_bridge_select').append(createSelOpt(key, hueIPs[key].bridgeid));
        }

        $('.hue_bridge_sel_watch').on("click", function () {
          hueIPsinc = $(this).val();

          $('#host').val(hueIPs[hueIPsinc].host);
          $('#port').val(hueIPs[hueIPsinc].port);
          $('#bridgeid').val(hueIPs[hueIPsinc].bridgeid);

          const usr = $('#user').val();
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

  function getOrCreate(map, key, createValue) {
      if (!map.has(key)) {
          map.set(key, createValue());
      }
      return map.get(key);
  }

  function getLedDeviceProperty(ledType, key, username) {
      const ledTypeMap = devicesProperties.get(ledType);
      if (!ledTypeMap) return undefined;
      const keyMap = ledTypeMap.get(key);
      if (!keyMap) return undefined;
      return keyMap.get(username);
  }

    function setLedDeviceProperty(ledType, key, username, ledDeviceProperties) {
      const ledTypeMap = getOrCreate(devicesProperties, ledType, () => new Map());
      const keyMap = getOrCreate(ledTypeMap, key, () => new Map());
      keyMap.set(username, ledDeviceProperties);
  }

  async function getProperties(cb, hostAddress, port, bridgeid, username, resourceFilter) {
    let params = { host: hostAddress, bridgeid, username, filter: resourceFilter };
    if (port !== 'undefined') {
      params.port = parseInt(port);
    }

    const ledType = 'philipshue';
    const key = hostAddress;

    const cachedLedProperties = getLedDeviceProperty(ledType, key, username);
    // Use device's properties, if properties in cache
    if (cachedLedProperties) {
      updateBridgeDetails(ledType, key, username);
      cb(true, username);
    } else {
      const res = await requestLedDeviceProperties(ledType, params);
      if (res && !res.error) {
        let ledDeviceProperties = res.info.properties;
        if (!jQuery.isEmptyObject(ledDeviceProperties)) {

          isAPIv2Ready = res.info.isAPIv2Ready;
          isEntertainmentReady = res.info.isEntertainmentReady;

          ledDeviceProperties.isAPIv2Ready = isAPIv2Ready;
          ledDeviceProperties.isEntertainmentReady = isEntertainmentReady;
          setLedDeviceProperty(ledType, key, username, ledDeviceProperties);

          updateBridgeDetails(ledType, key, username);
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

  async function identify(hostAddress, port, bridgeid, username, name, id, id_v1) {
    const disabled = $('#btn_wiz_save').is(':disabled');
    // Take care that new record cannot be save during background process
    $('#btn_wiz_save').prop('disabled', true);

    let params = { host: decodeURIComponent(hostAddress), bridgeid, username, lightName: decodeURIComponent(name), lightId: id, lightId_v1: id_v1 };

    if (port !== 'undefined') {
      params.port = parseInt(port);
    }

    await requestLedDeviceIdentification('philipshue', params);

    if (!window.readOnlyMode) {
      $('#btn_wiz_save').prop('disabled', disabled);
    }
  }

  function begin() {
    const usr = utils.eV("username");
    if (usr != "") {
      $('#user').val(usr);
    }

    const clkey = utils.eV("clientkey");
    if (clkey != "") {
      $('#clientkey').val(clkey);
    }

    //check if host is empty/reachable/search for bridge
    if (utils.eV("host") == "") {
      hueIPs = [];
      hueIPsinc = 0;

      discover();
    }
    else {
      const host = utils.eV("host");
      $('#host').val(host);

      const port = utils.eV("port");
      if (port > 0) {
        $('#port').val(port);
      }
      else {
        $('#port').val('');
      }
      hueIPs.push({ host, port });

      if (usr != "") {
        checkHueBridge(checkUserResult, usr);
      } else {
        checkHueBridge(checkBridgeResult);
      }
    }

    $('#retry_bridge').off().on('click', function () {
      const host = $('#host').val();
      const port = parseInt($('#port').val());

      if (host != "") {

        const idx = hueIPs.findIndex(item => item.host === host && item.port === port);
        if (idx === -1) {
          hueIPs.push({ host: host, port: port });
          hueIPsinc = hueIPs.length - 1;
        } else {
          hueIPsinc = idx;
        }
      }
      else {
        discover();
      }

      const usr = $('#user').val();
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
    $('#btn_wiz_save').off().on("click", function () {
      let hueLedConfig = [];
      let finalLightIds = [];
      let channelNumber = 0;

      //create hue led config
      for (const key in groupLights) {
        const lightId = groupLights[key];

        if ($('#hue_' + lightId).val() != "disabled") {
          finalLightIds.push(lightId);

          let lightName;
          if (isAPIv2Ready) {
            const light = hueLights.find(light => light.id === lightId);
            lightName = light.metadata.name;
          } else {
            lightName = hueLights[lightId].name;
          }

          const position = $('#hue_' + lightId).val();
          const lightIdx = groupLights.indexOf(lightId);
          const lightLocation = groupLightsLocations[lightIdx];

          let serviceID;
          if (isAPIv2Ready) {
            if (lightLocation) {
              serviceID = lightLocation.service.rid;
            }
          }

          if (position.startsWith("entertainment")) {

            // Layout per entertainment area definition at bridge
            let isFocusCenter = false;
            if (position === "entertainment_center") {
              isFocusCenter = true;
            }

            if (isAPIv2Ready) {

              groupChannels.forEach((channel) => {
                if (channel.members[0].service.rid === serviceID) {
                  const layoutObject = assignLightEntertainmentPos(isFocusCenter, channel.position, lightName, channel.channel_id);
                  hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
                  ++channelNumber;
                }
              });
            } else {
              const layoutObject = assignLightEntertainmentPos(isFocusCenter, lightLocation.position, lightName);
              hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
            }
          }
          else {
            // Layout per manual settings
            let maxSegments = 1;

            if (isAPIv2Ready && serviceID) {
              const service = hueEntertainmentServices.find(service => service.id === serviceID);
              maxSegments = service.segments.max_segments;
            }

            if (maxSegments > 1) {
              const segment = service.segments.segments;
              const layoutObjects = assignSegmentedLightPos(segment, position, lightName);
              hueLedConfig.push(...layoutObjects);
            } else {
              const layoutObject = utils.assignLightPos(position, lightName);
              hueLedConfig.push(JSON.parse(JSON.stringify(layoutObject)));
            }
            channelNumber += maxSegments;
          }
        }
      }

      let sc = window.serverConfig;
      sc.leds = hueLedConfig;

      //Adjust gamma, brightness and compensation
      let c = sc.color.channelAdjustment[0];
      c.gammaBlue = 1.0;
      c.gammaRed = 1.0;
      c.gammaGreen = 1.0;
      c.brightness = 100;
      c.brightnessCompensation = 0;

      //device config

      //Start with a clean configuration
      let d = {};
      d.host = $('#host').val();
      d.port = parseInt($('#port').val());
      d.bridgeid = $('#bridgeid').val();
      d.username = $('#user').val();
      d.type = 'philipshue';
      d.colorOrder = 'rgb';
      d.lightIds = finalLightIds;
      d.transitiontime = parseInt(utils.eV("transitiontime", 1));
      d.restoreOriginalState = utils.eV("restoreOriginalState", false);
      d.switchOffOnBlack = utils.eV("switchOffOnBlack", false);

      d.blackLevel = parseFloat(utils.eV("blackLevel", 0.009));
      d.onBlackTimeToPowerOff = parseInt(utils.eV("onBlackTimeToPowerOff", 600));
      d.onBlackTimeToPowerOn = parseInt(utils.eV("onBlackTimeToPowerOn", 300));
      d.brightnessFactor = parseFloat(utils.eV("brightnessFactor", 1));

      d.clientkey = $('#clientkey').val();
      d.groupId = $('#groupId').val();
      d.blackLightsTimeout = parseInt(utils.eV("blackLightsTimeout", 5000));
      d.brightnessMin = parseFloat(utils.eV("brightnessMin", 0));
      d.brightnessMax = parseFloat(utils.eV("brightnessMax", 1));
      d.brightnessThreshold = parseFloat(utils.eV("brightnessThreshold", 0.0001));
      d.handshakeTimeoutMin = parseInt(utils.eV("handshakeTimeoutMin", 300));
      d.handshakeTimeoutMax = parseInt(utils.eV("handshakeTimeoutMax", 1000));
      d.verbose = utils.eV("verbose");

      d.autoStart = conf_editor.getEditor("root.generalOptions.autoStart").getValue();
      d.enableAttempts = parseInt(conf_editor.getEditor("root.generalOptions.enableAttempts").getValue());
      d.enableAttemptsInterval = parseInt(conf_editor.getEditor("root.generalOptions.enableAttemptsInterval").getValue());

      d.useEntertainmentAPI = isEntertainmentReady && (d.groupId !== "");
      d.useAPIv2 = isAPIv2Ready;

      if (d.useEntertainmentAPI) {
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
    const host = hueIPs[hueIPsinc].host;
    const port = hueIPs[hueIPsinc].port;

    let params = { host: host };
    if (port !== 'undefined') {
      params.port = parseInt(port);
    }

    let retryTime = 30;
    const retryInterval = 2;

    const UserInterval = setInterval(function () {

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
            const response = res.info;

            if (jQuery.isEmptyObject(response)) {
              debugMessage(retryTime + ": link button not pressed or device not reachable");
            } else {
              $('#wizp1').toggle(false);
              $('#wizp2').toggle(true);
              $('#wizp3').toggle(false);

              const username = response.username;
              if (username != 'undefined') {
                $('#user').val(username);
                conf_editor.getEditor("root.specificOptions.username").setValue(username);
                conf_editor.getEditor("root.specificOptions.host").setValue(host);
                conf_editor.getEditor("root.specificOptions.port").setValue(port);
              }

              if (isEntertainmentReady) {
                const clientkey = response.clientkey;
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
  const host = hueIPs[hueIPsinc]?.host;
  const ledProperties = getLedDeviceProperty('philipshue', host, username);
  if (ledProperties) {
    if (isAPIv2Ready && Array.isArray(ledProperties?.data) && ledProperties.data.length > 0) {
      hueEntertainmentConfigs = ledProperties.data.filter(config => config.type === "entertainment_configuration");
      hueEntertainmentServices = ledProperties.data.filter(config => config.type === "entertainment" && config.renderer === true);
    } else if (ledProperties?.groups && Object.keys(ledProperties.groups).length > 0) {
      hueEntertainmentConfigs = [];
      const hueGroups = ledProperties.groups;
      for (const [groupid, group] of Object.entries(hueGroups)) {
        if (group.type === 'Entertainment') {
          hueEntertainmentConfigs.push({ ...group, id: groupid });
        }
      }
    }

  if (hueEntertainmentConfigs?.length > 0) {

        $('.lidsb').html("");
        $('#wh_topcontainer').toggle(false);
        $('#hue_grp_ids_t').toggle(true);

        for (const groupid in hueEntertainmentConfigs) {
          $('.gidsb').append(createTableRow([groupid + ' (' + hueEntertainmentConfigs[groupid].name + ')',
          '<button class="btn btn-sm btn-primary btn-group" data-groupid="' + groupid + '" data-username="' + username + '")>'
          + $.i18n('wiz_hue_e_use_group') + '</button>']));
        }
        attachGroupButtonEvent();

      } else {
        noAPISupport('wiz_hue_e_noegrpids', username);
      }
    }
  }
  function attachIdentifyButtonEvent() {
    $('#wizp2_body').on('click', '.btn-identify', function () {
      const hostname = $(this).data('hostname');
      const port = $(this).data('port');
      const bridgeid = $(this).data('bridgeid');
      const user = $(this).data('user');
      const lightName = $(this).data('light-name');
      const lightId = $(this).data('light-id');
      const lightId_v1 = $(this).data('light-id-v1');

      identify(hostname, port, bridgeid, user, lightName, lightId, lightId_v1);
    });
  }
  function attachGroupButtonEvent() {
    $('#wizp2_body').on('click', '.btn-group', function () {
      const groupid = $(this).data('groupid');
      const username = $(this).data('username');

      useGroupId(groupid, username);
    });
  }

  function noAPISupport(txt, username) {
    showNotification('danger', $.i18n('wiz_hue_e_title'), $.i18n('wiz_hue_e_noapisupport_hint'));
    conf_editor.getEditor("root.specificOptions.useEntertainmentAPI").setValue(false);
    $("#root_specificOptions_useEntertainmentAPI").trigger("change");
    $('#btn_wiz_holder').append('<div class="bs-callout bs-callout-danger" style="margin-top:0px">' + $.i18n('wiz_hue_e_noapisupport_hint') + '</div>');
    $('#hue_grp_ids_t').toggle(false);
    const errorMessage = txt ? $.i18n(txt) : $.i18n('wiz_hue_e_nogrpids');
    $('<p style="font-weight:bold;color:red;">' + errorMessage + '<br />' + $.i18n('wiz_hue_e_noapisupport') + '</p>').insertBefore('#wizp2_body #hue_ids_t');
    $('#hue_id_headline').html($.i18n('wiz_hue_desc2'));

    get_hue_lights(username);
  }

  function get_hue_lights(username) {
    const host = hueIPs[hueIPsinc].host;
    const port = hueIPs[hueIPsinc].port;

    const ledProperties = getLedDeviceProperty('philipshue', host, username);
    if (ledProperties) {
      if (isAPIv2Ready) {
        const data = ledProperties?.data;
        if (Array.isArray(data) && data.length > 0) {
          hueLights = data.filter(config => config.type === "light");
        }
      } else if (Array.isArray(ledProperties?.lights) && ledProperties.lights.length > 0) {
        hueLights = ledProperties.lights;
      }

      if (Object.keys(hueLights).length > 0) {
        if (!isEntertainmentReady) {
          $('#wh_topcontainer').toggle(false);
        }
        $('#hue_ids_t, #btn_wiz_save').toggle(true);

        const lightOptions = utils.getLayoutPositions();
        if (isEntertainmentReady && hueEntertainmentConfigs.length > 0) {
          lightOptions.unshift("entertainment_center");
          lightOptions.unshift("entertainment");
        } else {
          lightOptions.unshift("disabled");
          groupLights = [];
          if (isAPIv2Ready) {
            for (const light in hueLights) {
              groupLights.push(hueLights[light].id);
            }
          } else {
            groupLights = Object.keys(hueLights);
          }
        }

        $('.lidsb').html("");

        let pos = "";
        for (const id in groupLights) {
          const lightId = groupLights[id];
          let lightId_v1 = "/lights/" + lightId;

          let lightName;
          if (isAPIv2Ready) {
            const light = hueLights.find(light => light.id === lightId);
            lightName = light.metadata.name;
            lightId_v1 = light.id_v1;
          } else {
            lightName = hueLights[lightId].name;
          }

          if (isEntertainmentReady) {
            let lightLocation = {};
            lightLocation = groupLightsLocations[id];
            if (lightLocation) {
              if (isAPIv2Ready) {
                pos = 0;
              } else {
                const x = lightLocation.position.x;
                const y = lightLocation.position.y;
                const z = lightLocation.position.z;

                let xval = (x < 0) ? "left" : "right";
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

          let options = "";
          for (const opt in lightOptions) {
            const val = lightOptions[opt];
            options += '<option value="' + val + '"';
            if (pos == val) options += ' selected="selected"';
            options += '>' + $.i18n('conf_leds_layout_cl_' + val) + '</option>';
          }

          $('.lidsb').append(createTableRow([id + ' (' + lightName + ')',
          '<select id="hue_' + lightId + '" class="hue_sel_watch form-control">'
          + options
          + '</select>',
          '<button class="btn btn-sm btn-primary btn-identify" data-hostname="' + encodeURIComponent(host) + '" data-port="' + port + '" data-bridgeid="' + $('#bridgeid').val() + '" data-user="' + $("#user").val() + '" data-light-name="' + encodeURIComponent(lightName) + '" data-light-id="' + lightId + '" data-light-id-v1="' + lightId_v1 + '">'
          + $.i18n('wiz_hue_blinkblue', id)
          + '</button>']));
        }
        attachIdentifyButtonEvent();

        if (!isEntertainmentReady) {
          $('.hue_sel_watch').on("change", function () {
            let cC = 0;
            for (const key in hueLights) {
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
        const txt = '<p style="font-weight:bold;color:red;">' + $.i18n('wiz_hue_noids') + '</p>';
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

  return {
    start: function (e) {
      //create html
      const hue_title = 'wiz_hue_title';
      const hue_intro1 = 'wiz_hue_e_intro1';
      const hue_desc1 = 'wiz_hue_desc1';
      const hue_create_user = 'wiz_hue_create_user';

      $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(hue_title));
      $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(hue_title) + '</h4><p>' + $.i18n(hue_intro1) + '</p>');
      $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
      $('#wizp2_body').html('<div id="wh_topcontainer"></div>');

      let topContainer_html = '<p class="text-left" style="font-weight:bold">' + $.i18n(hue_desc1) + '</p>' +
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

      // Hidden fields
      topContainer_html += '<div class="form-group" style="display:none"><input type="hidden" id="bridgeid" name="bridgeid"></div>';
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

      $('#usrcont').append('<p><span style="font-weight:bold;color:red" id="wiz_hue_usrstate"></span></p>' +
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
        begin();
        $('#wizp1').toggle(false);
        $('#wizp2').toggle(true);
      });
    }
  };
})();

export { philipshueWizard }

