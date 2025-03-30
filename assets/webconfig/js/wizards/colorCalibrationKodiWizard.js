//****************************
// Wizard Color calibration via Kodi
//****************************
const colorCalibrationKodiWizard = (() => {

  let ws;
  const defaultKodiPort = 9090;

  let kodiAddress = document.location.hostname;
  let kodiPort = defaultKodiPort;

  const kodiUrl = new URL("ws://" + kodiAddress);
  kodiUrl.port = kodiPort;
  kodiUrl.pathname = "/jsonrpc/websocket";

  let wiz_editor;
  let colorLength;
  let cobj;
  let step = 0;
  let withKodi = false;
  let profile = 0;
  let websAddress;
  let imgAddress;
  let picnr = 0;
  let id = 1;
  const vidAddress = "https://sourceforge.net/projects/hyperion-project/files/resources/vid/";
  const availVideos = ["Sweet_Cocoon", "Caminandes_2_GranDillama", "Caminandes_3_Llamigos"];

  if (getStorage("kodiAddress") != null) {

    kodiAddress = getStorage("kodiAddress");
    kodiUrl.host = kodiAddress;
  }

  if (getStorage("kodiPort") != null) {
    kodiPort = getStorage("kodiPort");
    kodiUrl.port = kodiPort;
  }

  $(window).on('beforeunload', function () {
    closeWebSocket();
  });

  function closeWebSocket() {
    // Check if the WebSocket is open
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.close();
    }
  }

  function sendToKodi(type, content) {
    let command;

    switch (type) {
      case "msg":
        command = { "jsonrpc": "2.0", "method": "GUI.ShowNotification", "params": { "title": $.i18n('wiz_cc_title'), "message": content, "image": "info", "displaytime": 5000 }, "id": id };
        break;
      case "stop":
        command = { "jsonrpc": "2.0", "method": "Player.Stop", "params": { "playerid": 2 }, "id": id };
        break;
      case "playP":
        content = imgAddress + content + '.png';
        command = { "jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "file": content } }, "id": id };
        break;
      case "playV":
        content = vidAddress + content;
        command = { "jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "file": content } }, "id": id };
        break;
      case "rotate":
        command = { "jsonrpc": "2.0", "method": "Player.Rotate", "params": { "playerid": 2 }, "id": id };
        break;
      default:
        console.error('Unknown Kodi command type: ', type);
    }

    if (ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(command));
      ++id;
    } else {
      console.error('WebSocket connection is not open. Unable to send command.');
    }
  }

  function performAction() {
    let h;

    if (step == 1) {
      $('#wiz_cc_desc').html($.i18n('wiz_cc_chooseid'));
      updateEditor(["id"]);
      $('#btn_wiz_back').prop("disabled", true);
    }
    else
      $('#btn_wiz_back').prop("disabled", false);

    if (step == 2) {
      updateEditor(["white"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_white_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_white_title'));
        sendToKodi('playP', "white");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_white_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 3) {
      updateEditor(["gammaRed", "gammaGreen", "gammaBlue"]);
      h = '<p>' + $.i18n('wiz_cc_adjustgamma') + '</p>';
      if (withKodi) {
        sendToKodi('playP', "HGradient");
        h += '<button id="wiz_cc_btn_sp" class="btn btn-primary">' + $.i18n('wiz_cc_btn_switchpic') + '</button>';
      }
      else
        h += '<p>' + $.i18n('wiz_cc_lettvshowm', "grey_1, grey_2, grey_3, HGradient, VGradient") + '</p>';
      $('#wiz_cc_desc').html(h);
      $('#wiz_cc_btn_sp').off().on('click', function () {
        switchPicture(["VGradient", "grey_1", "grey_2", "grey_3", "HGradient"]);
      });
    }
    if (step == 4) {
      updateEditor(["red"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_red_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_red_title'));
        sendToKodi('playP', "red");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_red_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 5) {
      updateEditor(["green"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_green_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_green_title'));
        sendToKodi('playP', "green");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_green_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 6) {
      updateEditor(["blue"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_blue_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_blue_title'));
        sendToKodi('playP', "blue");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_blue_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 7) {
      updateEditor(["cyan"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_cyan_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_cyan_title'));
        sendToKodi('playP', "cyan");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_cyan_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 8) {
      updateEditor(["magenta"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_magenta_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_magenta_title'));
        sendToKodi('playP', "magenta");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_magenta_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 9) {
      updateEditor(["yellow"]);
      h = $.i18n('wiz_cc_adjustit', $.i18n('edt_conf_color_yellow_title'));
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_yellow_title'));
        sendToKodi('playP', "yellow");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_yellow_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 10) {
      updateEditor(["backlightThreshold", "backlightColored"]);
      h = $.i18n('wiz_cc_backlight');
      if (withKodi) {
        h += '<br/>' + $.i18n('wiz_cc_kodishould', $.i18n('edt_conf_color_black_title'));
        sendToKodi('playP', "black");
      }
      else
        h += '<br/>' + $.i18n('wiz_cc_lettvshow', $.i18n('edt_conf_color_black_title'));
      $('#wiz_cc_desc').html(h);
    }
    if (step == 11) {
      updateEditor([""], true);
      h = '<p>' + $.i18n('wiz_cc_testintro') + '</p>';
      if (withKodi) {
        h += '<p>' + $.i18n('wiz_cc_testintrok') + '</p>';
        sendToKodi('stop');
        availVideos.forEach(video => {
          const txt = video.replace(/_/g, " ");
          h += `<div><button id="${video}" class="btn btn-sm btn-primary videobtn"><i class="fa fa-fw fa-play"></i> ${txt}</button></div>`;
        });

        h += '<div><button id="stop" class="btn btn-sm btn-danger videobtn" style="margin-bottom:15px"><i class="fa fa-fw fa-stop"></i> ' + $.i18n('wiz_cc_btn_stop') + '</button></div>';
      }
      else
        h += '<p>' + $.i18n('wiz_cc_testintrowok') + ' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/vid/" target="_blank">' + $.i18n('wiz_cc_link') + '</a></p>';
      h += '<p>' + $.i18n('wiz_cc_summary') + '</p>';
      $('#wiz_cc_desc').html(h);

      $('.videobtn').off().on('click', function (e) {
        if (e.target.id == "stop")
          sendToKodi("stop");
        else
          sendToKodi("playV", e.target.id + '.mp4');

        $(this).prop("disabled", true);
        setTimeout(function () { $('.videobtn').prop("disabled", false) }, 10000);
      });

      $('#btn_wiz_next').prop("disabled", true);
      $('#btn_wiz_save').toggle(true);
      window.readOnlyMode ? $('#btn_wiz_save').prop('disabled', true) : $('#btn_wiz_save').prop('disabled', false);
    }
    else {
      $('#btn_wiz_next').prop("disabled", false);
      $('#btn_wiz_save').toggle(false);
    }
  }



  function switchPicture(pictures) {
    if (typeof pictures[picnr] === 'undefined')
      picnr = 0;

    sendToKodi('playP', pictures[picnr]);
    picnr++;
  }


  function initializeWebSocket(cb) {
    if ("WebSocket" in window) {

      if (kodiUrl.port === '') {
        kodiUrl.port = defaultKodiPort;
      }

      if (!ws || ws.readyState !== WebSocket.OPEN) {

        // Establish WebSocket connection
        ws = new WebSocket(kodiUrl);

        // WebSocket onopen event
        ws.onopen = function (event) {
          withKodi = true;
          cb("opened");
        };

        // WebSocket onmessage event (handle incoming messages)
        ws.onmessage = function (event) {
          const response = JSON.parse(event.data);
          if (response.method === "System.OnQuit") {
            closeWebSocket();
          } else if (response.result != undefined) {
            if (response.result !== "OK") {
              cb("error");
            }
          }
        };

        // WebSocket onerror event
        ws.onerror = function (error) {
          cb("error");
        };

        // WebSocket onclose event
        ws.onclose = function (event) {
          withKodi = false;
          if (event.code === 1006) {
            // Ignore error 1006 due to Kodi issue
            console.log("WebSocket closed with error code 1006. Ignoring due to Kodi bug.");
          }
          else {
            console.error("WebSocket closed with code:", event.code);
          }
        };
      } else {
        console.log("WebSocket connection is already open.");
      }
    }
    else {
      console.log("Kodi Access: WebSocket NOT supported by this browser");
      cb("error");
    }
  }

  function setupEventListeners() {
    $('#btn_wiz_cancel').off().on('click', function () {
      stop(true);
    });
    $('#wiz_cc_kodiip').off().on('change', function () {

      kodiAddress = encodeURIComponent($(this).val().trim());

      $('#kodi_status').html('');
      if (kodiAddress !== "") {

        if (!isValidHostnameOrIP(kodiAddress)) {

          $('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">' + $.i18n('edt_msgcust_error_hostname_ip') + '</p>');
          withKodi = false;

        } else {

          if (isValidIPv6(kodiAddress)) {
            kodiUrl.hostname = "[" + kodiAddress + "]";
          } else {
            kodiUrl.hostname = kodiAddress;
          }

          $('#kodi_status').html('<p style="font-weight:bold;margin-top:5px">' + $.i18n('wiz_cc_try_connect') + '</p>');
          $('#btn_wiz_cont').prop('disabled', true);

          closeWebSocket();
          initializeWebSocket(function (cb) {

            if (cb == "opened") {
              setStorage("kodiAddress", kodiAddress);
              setStorage("kodiPort", defaultKodiPort);

              $('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">' + $.i18n('wiz_cc_kodicon') + '</p>');
              $('#btn_wiz_cont').prop('disabled', false);

              if (withKodi) {
                sendToKodi("msg", $.i18n('wiz_cc_kodimsg_start'));
              }
            }
            else {
              $('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">' + $.i18n('wiz_cc_kodidiscon') + '</p><p>' + $.i18n('wiz_cc_kodidisconlink') + ' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">' + $.i18n('wiz_cc_link') + '</p>');
              withKodi = false;
            }

            $('#btn_wiz_cont').prop('disabled', false);
          });
        }
      }
    });

    //listen for continue
    $('#btn_wiz_cont').off().on('click', function () {
      begin();
      $('#wizp1').toggle(false);
      $('#wizp2').toggle(true);
    });
  }

  function init() {
    colorLength = window.serverConfig.color.channelAdjustment;
    cobj = window.schema.color.properties.channelAdjustment.items.properties;
    websAddress = document.location.hostname + ':' + window.serverConfig.webConfig.port;
    imgAddress = 'http://' + websAddress + '/img/cc/';
    setStorage("wizardactive", true);
  }

  function initProfiles() {
    //check profile count
    if (colorLength.length > 1) {
      $('#multi_cali').html('<p style="font-weight:bold;">' + $.i18n('wiz_cc_morethanone') + '</p><select id="wiz_select" class="form-control" style="width:200px;margin:auto"></select>');
      for (let i = 0; i < colorLength.length; i++)
        $('#wiz_select').append(createSelOpt(i, i + 1 + ' (' + colorLength[i].id + ')'));

      $('#wiz_select').off().on('change', function () {
        profile = $(this).val();
      });
    }
  }

  function createEditor() {
    wiz_editor = createJsonEditor('editor_container_wiz', {
      color: window.schema.color
    }, true, true);

    $('#editor_container_wiz h4').toggle(false);
    $('#editor_container_wiz .btn-group').toggle(false);
    $('#editor_container_wiz [data-schemapath="root.color.imageToLedMappingType"]').toggle(false);
    $('#editor_container_wiz [data-schemapath="root.color.reducedPixelSetFactorFactor"]').toggle(false);
    for (let i = 0; i < colorLength.length; i++)
      $('#editor_container_wiz [data-schemapath*="root.color.channelAdjustment.' + i + '."]').toggle(false);
  }
  function updateEditor(el, all) {
    for (let key in cobj) {
      if (all === true || el[0] == key || el[1] == key || el[2] == key)
        $('#editor_container_wiz [data-schemapath*=".' + profile + '.' + key + '"]').toggle(true);
      else
        $('#editor_container_wiz [data-schemapath*=".' + profile + '.' + key + '"]').toggle(false);
    }
  }

  function stop(reload) {
    if (withKodi) {
      sendToKodi("stop");
    }
    closeWebSocket();
    resetWizard(reload);
  }

  function begin() {
    step = 0;

    $('#btn_wiz_next').off().on('click', function () {
      step++;
      performAction();
    });

    $('#btn_wiz_back').off().on('click', function () {
      step--;
      performAction();
    });

    $('#btn_wiz_abort').off().on('click', function () {
      stop(true);
    });

    $('#btn_wiz_save').off().on('click', function () {
      requestWriteConfig(wiz_editor.getValue());
      stop(true);
    });

    wiz_editor.on("change", function () {
      const { leds, ...adjustments } = wiz_editor.getEditor('root.color.channelAdjustment.' + profile).getValue();
      requestAdjustment(adjustments, "", true);
    });
    step++
    performAction();
  }

  return {
    start: function () {
      //create html
      $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n('wiz_cc_title'));
      $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('wiz_cc_title') + '</h4>' +
        '<p>' + $.i18n('wiz_cc_intro1') + '</p>' +
        '<label>' + $.i18n('wiz_cc_kwebs') + '</label>' +
        '<input class="form-control" style="width:280px;margin:auto" id="wiz_cc_kodiip" type="text" placeholder="' + kodiAddress + '" value="' + kodiAddress + '" />' +
        '<span id="kodi_status"></span><span id="multi_cali"></span>'
      );
      $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont" disabled="disabled">' + '<i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button>' +
        '<button type="button" class="btn btn-danger" id="btn_wiz_cancel" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>'
      );
      $('#wizp2_body').html('<div id="wiz_cc_desc" style="font-weight:bold"></div><div id="editor_container_wiz"></div>'
      );
      $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_back">' + '<i class="fa fa-fw fa-chevron-left"></i>' + $.i18n('general_btn_back') + '</button>' +
        '<button type="button" class="btn btn-primary" id="btn_wiz_next">' + $.i18n('general_btn_next') + '<i style="margin-left:4px;"class="fa fa-fw fa-chevron-right"></i>' + '</button>' +
        '<button type="button" class="btn btn-warning" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_save') + '</button>' +
        '<button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>'
      );

      if (getStorage("darkMode") == "on")
        $('#wizard_logo').prop("src", 'img/hyperion/logo_negativ.png');

      //open modal
      $("#wizard_modal").modal({
        backdrop: "static",
        keyboard: false,
        show: true
      });

      setupEventListeners();
      $('#wiz_cc_kodiip').trigger("change");
      init();
      initProfiles();
      createEditor();
    }
  };
})();

export { colorCalibrationKodiWizard };
