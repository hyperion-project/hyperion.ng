//clear priority and other tasks if people reload the page or lost connection while a wizard was active
$(window.hyperion).one("ready", function (event) {
  if (getStorage("wizardactive") === 'true') {
    requestPriorityClear();
    setStorage("wizardactive", false);
    if (getStorage("kodiAddress") != null) {
      kodiAddress = getStorage("kodiAddress");

      if (getStorage("kodiPort") != null) {
        kodiPort = getStorage("kodiPort");
      }
      sendToKodi("stop");
    }
  }
});

function resetWizard(reload) {
  $("#wizard_modal").modal('hide');
  clearInterval(wIntveralId);
  requestPriorityClear();
  setStorage("wizardactive", false);
  $('#wizp1').toggle(true);
  $('#wizp2').toggle(false);
  $('#wizp3').toggle(false);
  //cc
  if (withKodi)
    sendToKodi("stop");
  step = 0;
  if (!reload) location.reload();
}

//rgb byte order wizard
var wIntveralId;
var new_rgb_order;

function changeColor() {
  var color = $("#wiz_canv_color").css('background-color');

  if (color == 'rgb(255, 0, 0)') {
    $("#wiz_canv_color").css('background-color', 'rgb(0, 255, 0)');
    requestSetColor('0', '255', '0');
  }
  else {
    $("#wiz_canv_color").css('background-color', 'rgb(255, 0, 0)');
    requestSetColor('255', '0', '0');
  }
}

function startWizardRGB() {
  //create html
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n('wiz_rgb_title'));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('wiz_rgb_title') + '</h4><p>' + $.i18n('wiz_rgb_intro1') + '</p><p style="font-weight:bold;">' + $.i18n('wiz_rgb_intro2') + '</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  $('#wizp2_body').html('<p style="font-weight:bold">' + $.i18n('wiz_rgb_expl') + '</p>');
  $('#wizp2_body').append('<div class="form-group"><label>' + $.i18n('wiz_rgb_switchevery') + '</label><div class="input-group" style="width:100px"><select id="wiz_switchtime_select" class="form-control"></select><div class="input-group-addon">' + $.i18n('edt_append_s') + '</div></div></div>');
  $('#wizp2_body').append('<canvas id="wiz_canv_color" width="100" height="100" style="border-radius:60px;background-color:red; display:block; margin: 10px 0;border:4px solid grey;"></canvas><label>' + $.i18n('wiz_rgb_q') + '</label>');
  $('#wizp2_body').append('<table class="table borderless" style="width:200px"><tbody><tr><td class="ltd"><label>' + $.i18n('wiz_rgb_qrend') + '</label></td><td class="itd"><select id="wiz_r_select" class="form-control wselect"></select></td></tr><tr><td class="ltd"><label>' + $.i18n('wiz_rgb_qgend') + '</label></td><td class="itd"><select id="wiz_g_select" class="form-control wselect"></select></td></tr></tbody></table>');
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_save') + '</button><button type="button" class="btn btn-primary" id="btn_wiz_checkok" style="display:none" data-dismiss="modal"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_ok') + '</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');

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
    beginWizardRGB();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function beginWizardRGB() {
  $("#wiz_switchtime_select").off().on('change', function () {
    clearInterval(wIntveralId);
    var time = $("#wiz_switchtime_select").val();
    wIntveralId = setInterval(function () { changeColor(); }, time * 1000);
  });

  $('.wselect').on("change", function () {
    var rgb_order = window.serverConfig.device.colorOrder.split("");
    var redS = $("#wiz_r_select").val();
    var greenS = $("#wiz_g_select").val();
    var blueS = rgb_order.toString().replace(/,/g, "").replace(redS, "").replace(greenS, "");

    for (var i = 0; i < rgb_order.length; i++) {
      if (redS == rgb_order[i])
        $('#wiz_g_select option[value=' + rgb_order[i] + ']').prop('disabled', true);
      else
        $('#wiz_g_select option[value=' + rgb_order[i] + ']').prop('disabled', false);
      if (greenS == rgb_order[i])
        $('#wiz_r_select option[value=' + rgb_order[i] + ']').prop('disabled', true);
      else
        $('#wiz_r_select option[value=' + rgb_order[i] + ']').prop('disabled', false);
    }

    if (redS != 'null' && greenS != 'null') {
      $('#btn_wiz_save').prop('disabled', false);

      for (var i = 0; i < rgb_order.length; i++) {
        if (rgb_order[i] == "r")
          rgb_order[i] = redS;
        else if (rgb_order[i] == "g")
          rgb_order[i] = greenS;
        else
          rgb_order[i] = blueS;
      }

      rgb_order = rgb_order.toString().replace(/,/g, "");

      if (redS == "r" && greenS == "g") {
        $('#btn_wiz_save').toggle(false);
        $('#btn_wiz_checkok').toggle(true);

        window.readOnlyMode ? $('#btn_wiz_checkok').prop('disabled', true) : $('#btn_wiz_checkok').prop('disabled', false);
      }
      else {
        $('#btn_wiz_save').toggle(true);
        window.readOnlyMode ? $('#btn_wiz_save').prop('disabled', true) : $('#btn_wiz_save').prop('disabled', false);

        $('#btn_wiz_checkok').toggle(false);
      }
      new_rgb_order = rgb_order;
    }
    else
      $('#btn_wiz_save').prop('disabled', true);
  });

  $("#wiz_switchtime_select").append(createSelOpt('5', '5'), createSelOpt('10', '10'), createSelOpt('15', '15'), createSelOpt('30', '30'));
  $("#wiz_switchtime_select").trigger('change');

  $("#wiz_r_select").append(createSelOpt("null", ""), createSelOpt('r', $.i18n('general_col_red')), createSelOpt('g', $.i18n('general_col_green')), createSelOpt('b', $.i18n('general_col_blue')));
  $("#wiz_g_select").html($("#wiz_r_select").html());
  $("#wiz_r_select").trigger('change');

  requestSetColor('255', '0', '0');
  setTimeout(requestSetSource, 100, 'auto');
  setStorage("wizardactive", true);

  $('#btn_wiz_abort').off().on('click', function () { resetWizard(true); });

  $('#btn_wiz_checkok').off().on('click', function () {
    showInfoDialog('success', "", $.i18n('infoDialog_wizrgb_text'));
    resetWizard();
  });

  $('#btn_wiz_save').off().on('click', function () {
    resetWizard();
    window.serverConfig.device.colorOrder = new_rgb_order;
    requestWriteConfig({ "device": window.serverConfig.device });
  });
}

$('#btn_wizard_byteorder').off().on('click', startWizardRGB);

//color calibration wizard

const defaultKodiPort = 9090;

var kodiAddress = document.location.hostname;
var kodiPort = defaultKodiPort;

var kodiUrl = new URL("ws://" + kodiAddress);
kodiUrl.port = kodiPort;
kodiUrl.pathname = "/jsonrpc/websocket";

var wiz_editor;
var colorLength;
var cobj;
var step = 0;
var withKodi = false;
var profile = 0;
var websAddress;
var imgAddress;
var vidAddress = "https://sourceforge.net/projects/hyperion-project/files/resources/vid/";
var picnr = 0;
var availVideos = ["Sweet_Cocoon", "Caminandes_2_GranDillama", "Caminandes_3_Llamigos"];

if (getStorage("kodiAddress") != null) {

  kodiAddress = getStorage("kodiAddress");
  kodiUrl.host = kodiAddress;
}

if (getStorage("kodiPort") != null) {
  kodiPort = getStorage("kodiPort");
  kodiUrl.port = kodiPort;
}

function switchPicture(pictures) {
  if (typeof pictures[picnr] === 'undefined')
    picnr = 0;

  sendToKodi('playP', pictures[picnr]);
  picnr++;
}

function sendToKodi(type, content, cb) {
  var command;

  switch (type) {
    case "msg":
      command = { "jsonrpc": "2.0", "method": "GUI.ShowNotification", "params": { "title": $.i18n('wiz_cc_title'), "message": content, "image": "info", "displaytime": 5000 }, "id": "1" };
      break;
    case "stop":
      command = { "jsonrpc": "2.0", "method": "Player.Stop", "params": { "playerid": 2 }, "id": "1" };
      break;
    case "playP":
      content = imgAddress + content + '.png';
      command = { "jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "file": content } }, "id": "1" };
      break;
    case "playV":
      content = vidAddress + content;
      command = { "jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "file": content } }, "id": "1" };
      break;
    case "rotate":
      command = { "jsonrpc": "2.0", "method": "Player.Rotate", "params": { "playerid": 2 }, "id": "1" };
      break;
    default:
      if (cb != undefined) {
        cb("error");
      }
  }

  if ("WebSocket" in window) {

    if (kodiUrl.port === '') {
      kodiUrl.port = defaultKodiPort;
    }
    var ws = new WebSocket(kodiUrl);

    ws.onopen = function () {
      ws.send(JSON.stringify(command));
    };

    ws.onmessage = function (evt) {
      var response = JSON.parse(evt.data);
      if (response.method === "System.OnQuit") {
        ws.close();
      } else {
        if (cb != undefined) {
          if (response.result != undefined) {
            if (response.result === "OK") {
              cb("success");
              ws.close();
            } else {
              cb("error");
              ws.close();
            }
          }
        }
      }
    };

    ws.onerror = function (evt) {
      if (cb != undefined) {
        cb("error");
        ws.close();
      }
    };

    ws.onclose = function (evt) {
    };

  }
  else {
    console.log("Kodi Access: WebSocket NOT supported by this browser");
    cb("error");
  }
}

function performAction() {
  var h;

  if (step == 1) {
    $('#wiz_cc_desc').html($.i18n('wiz_cc_chooseid'));
    updateWEditor(["id"]);
    $('#btn_wiz_back').prop("disabled", true);
  }
  else
    $('#btn_wiz_back').prop("disabled", false);

  if (step == 2) {
    updateWEditor(["white"]);
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
    updateWEditor(["gammaRed", "gammaGreen", "gammaBlue"]);
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
    updateWEditor(["red"]);
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
    updateWEditor(["green"]);
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
    updateWEditor(["blue"]);
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
    updateWEditor(["cyan"]);
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
    updateWEditor(["magenta"]);
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
    updateWEditor(["yellow"]);
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
    updateWEditor(["backlightThreshold", "backlightColored"]);
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
    updateWEditor([""], true);
    h = '<p>' + $.i18n('wiz_cc_testintro') + '</p>';
    if (withKodi) {
      h += '<p>' + $.i18n('wiz_cc_testintrok') + '</p>';
      sendToKodi('stop');
      for (var i = 0; i < availVideos.length; i++) {
        var txt = availVideos[i].replace(/_/g, " ");
        h += '<div><button id="' + availVideos[i] + '" class="btn btn-sm btn-primary videobtn"><i class="fa fa-fw fa-play"></i> ' + txt + '</button></div>';
      }
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

function updateWEditor(el, all) {
  for (var key in cobj) {
    if (all === true || el[0] == key || el[1] == key || el[2] == key)
      $('#editor_container_wiz [data-schemapath*=".' + profile + '.' + key + '"]').toggle(true);
    else
      $('#editor_container_wiz [data-schemapath*=".' + profile + '.' + key + '"]').toggle(false);
  }
}

function startWizardCC() {

  //create html
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n('wiz_cc_title'));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('wiz_cc_title') + '</h4>' +
    '<p>' + $.i18n('wiz_cc_intro1') + '</p>' +
    '<label>' + $.i18n('wiz_cc_kwebs') + '</label>' +
    '<input class="form-control" style="width:280px;margin:auto" id="wiz_cc_kodiip" type="text" placeholder="' + kodiAddress + '" value="' + kodiAddress + '" />' +
    '<span id="kodi_status"></span><span id="multi_cali"></span>'
  );
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont" disabled="disabled">' + '<i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button>' +
    '<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>'
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

        sendToKodi("msg", $.i18n('wiz_cc_kodimsg_start'), function (cb) {
          if (cb == "error") {
            $('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">' + $.i18n('wiz_cc_kodidiscon') + '</p><p>' + $.i18n('wiz_cc_kodidisconlink') + ' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">' + $.i18n('wiz_cc_link') + '</p>');
            withKodi = false;
          }
          else {
            setStorage("kodiAddress", kodiAddress);
            setStorage("kodiPort", defaultKodiPort);

            $('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">' + $.i18n('wiz_cc_kodicon') + '</p>');
            withKodi = true;
          }

          $('#btn_wiz_cont').prop('disabled', false);
        });
      }
    }
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click', function () {
    beginWizardCC();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });

  $('#wiz_cc_kodiip').trigger("change");
  colorLength = window.serverConfig.color.channelAdjustment;
  cobj = window.schema.color.properties.channelAdjustment.items.properties;
  websAddress = document.location.hostname + ':' + window.serverConfig.webConfig.port;
  imgAddress = 'http://' + websAddress + '/img/cc/';
  setStorage("wizardactive", true);

  //check profile count
  if (colorLength.length > 1) {
    $('#multi_cali').html('<p style="font-weight:bold;">' + $.i18n('wiz_cc_morethanone') + '</p><select id="wiz_select" class="form-control" style="width:200px;margin:auto"></select>');
    for (var i = 0; i < colorLength.length; i++)
      $('#wiz_select').append(createSelOpt(i, i + 1 + ' (' + colorLength[i].id + ')'));

    $('#wiz_select').off().on('change', function () {
      profile = $(this).val();
    });
  }

  //prepare editor
  wiz_editor = createJsonEditor('editor_container_wiz', {
    color: window.schema.color
  }, true, true);

  $('#editor_container_wiz h4').toggle(false);
  $('#editor_container_wiz .btn-group').toggle(false);
  $('#editor_container_wiz [data-schemapath="root.color.imageToLedMappingType"]').toggle(false);
  for (var i = 0; i < colorLength.length; i++)
    $('#editor_container_wiz [data-schemapath*="root.color.channelAdjustment.' + i + '."]').toggle(false);
}

function beginWizardCC() {
  $('#btn_wiz_next').off().on('click', function () {
    step++;
    performAction();
  });

  $('#btn_wiz_back').off().on('click', function () {
    step--;
    performAction();
  });

  $('#btn_wiz_abort').off().on('click', resetWizard);

  $('#btn_wiz_save').off().on('click', function () {
    requestWriteConfig(wiz_editor.getValue());
    resetWizard();
  });

  wiz_editor.on("change", function (e) {
    var val = wiz_editor.getEditor('root.color.channelAdjustment.' + profile + '').getValue();
    var temp = JSON.parse(JSON.stringify(val));
    delete temp.leds
    requestAdjustment(JSON.stringify(temp), "", true);
  });

  step++
  performAction();
}

$('#btn_wizard_colorcalibration').off().on('click', startWizardCC);

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

function assignLightPos(id, pos, name) {
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
var hueLights = null;
var hueGroups = null;
var lightLocation = [];
var groupLights = [];
var groupLightsLocations = [];
var hueType = "philipshue";

function startWizardPhilipsHue(e) {
  if (typeof e.data.type != "undefined") hueType = e.data.type;

  //create html

  var hue_title = 'wiz_hue_title';
  var hue_intro1 = 'wiz_hue_intro1';
  var hue_desc1 = 'wiz_hue_desc1';
  var hue_create_user = 'wiz_hue_create_user';
  if (hueType == 'philipshueentertainment') {
    hue_title = 'wiz_hue_e_title';
    hue_intro1 = 'wiz_hue_e_intro1';
    hue_desc1 = 'wiz_hue_e_desc1';
    hue_create_user = 'wiz_hue_e_create_user';
  }
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>' + $.i18n(hue_title));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n(hue_title) + '</h4><p>' + $.i18n(hue_intro1) + '</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>' + $.i18n('general_btn_continue') + '</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  $('#wizp2_body').html('<div id="wh_topcontainer"></div>');

  var hidePort = "hidden-lg";
  if (storedAccess === 'expert') {
    hidePort = "";
  }

  $('#wh_topcontainer').append('<p class="text-left" style="font-weight:bold">' + $.i18n(hue_desc1) + '</p>' +
    '<div class="row">' +
    '<div class="col-md-2">' +
    '  <p class="text-left">' + $.i18n('wiz_hue_ip') + '</p></div>' +
    '  <div class="col-md-7"><div class="input-group">' +
    '    <span class="input-group-addon" id="retry_bridge" style="cursor:pointer"><i class="fa fa-refresh"></i></span>' +
    '    <input type="text" class="input-group form-control" id="host" placeholder="' + $.i18n('wiz_hue_ip') + '"></div></div>' +
    '  <div class="col-md-3 ' + hidePort + '"><div class="input-group">' +
    '    <span class="input-group-addon">:</span>' +
    '    <input type="text" class="input-group form-control" id="port" placeholder="' + $.i18n('edt_conf_general_port_title') + '"></div></div>' +
    '</div><p><span style="font-weight:bold;color:red" id="wiz_hue_ipstate"></span><span style="font-weight:bold;" id="wiz_hue_discovered"></span></p>'
  );
  $('#wh_topcontainer').append();
  $('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

  $('#usrcont').append('<div class="row"><div class="col-md-2"><p class="text-left">' + $.i18n('wiz_hue_username') + '</p ></div>' +
    '<div class="col-md-7">' +
    '<div class="input-group">' +
    '  <span class="input-group-addon" id="retry_usr" style="cursor:pointer"><i class="fa fa-refresh"></i></span>' +
    '  <input type="text" class="input-group form-control" id="user">' +
    '</div></div></div><br>' +
    '</div><input type="hidden" id="groupId">'
  );

  if (hueType == 'philipshueentertainment') {
    $('#usrcont').append('<div class="row"><div class="col-md-2"><p class="text-left">' + $.i18n('wiz_hue_clientkey') +
      '</p></div><div class="col-md-7"><input class="form-control" id="clientkey" type="text"></div></div><br>');
  }

  $('#usrcont').append('<p><span style="font-weight:bold;color:red" id="wiz_hue_usrstate"></span><\p>' +
    '<button type="button" class="btn btn-primary" style="display:none" id="wiz_hue_create_user"> <i class="fa fa-fw fa-plus"></i>' + $.i18n(hue_create_user) + '</button>');

  if (hueType == 'philipshueentertainment') {
    $('#wizp2_body').append('<div id="hue_grp_ids_t" style="display:none"><p class="text-left" style="font-weight:bold">' + $.i18n('wiz_hue_e_desc2') + '</p></div>');
    createTable("gidsh", "gidsb", "hue_grp_ids_t");
    $('.gidsh').append(createTableRow([$.i18n('edt_dev_spec_groupId_title'), $.i18n('wiz_hue_e_use_group')], true));
    $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p class="text-left" style="font-weight:bold" id="hue_id_headline">' + $.i18n('wiz_hue_e_desc3') + '</p></div>');
  }
  else {
    $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p class="text-left" style="font-weight:bold" id="hue_id_headline">' + $.i18n('wiz_hue_desc2') + '</p></div>');
  }
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
  if (usr == 'config') $('#wiz_hue_discovered').html("");

  if (hueIPs[hueIPsinc]) {
    var host = hueIPs[hueIPsinc].host;
    var port = hueIPs[hueIPsinc].port;

    getProperties_hue_bridge(cb, decodeURIComponent(host), port, usr);
  }
}

function checkBridgeResult(reply, usr) {
  if (reply) {
    //abort checking, first reachable result is used
    $('#wiz_hue_ipstate').html("");
    $('#host').val(hueIPs[hueIPsinc].host)
    $('#port').val(hueIPs[hueIPsinc].port)

    $('#usrcont').toggle(true);
    checkHueBridge(checkUserResult, $('#user').val() ? $('#user').val() : "newdeveloper");
  }
  else {
    //increment and check again
    if (hueIPs.length - 1 > hueIPsinc) {
      hueIPsinc++;
      checkHueBridge(checkBridgeResult);
    }
    else {
      $('#usrcont').toggle(false);
      $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
    }
  }
};

function checkUserResult(reply, usr) {
  $('#usrcont').toggle(true);
  if (reply) {
    $('#user').val(usr);
    if (hueType == 'philipshueentertainment' && $('#clientkey').val() == "") {

      $('#wiz_hue_usrstate').html($.i18n('wiz_hue_e_clientkey_needed'));
      $('#wiz_hue_create_user').toggle(true);
    } else {
      $('#wiz_hue_usrstate').html("");
      $('#wiz_hue_create_user').toggle(false);
      if (hueType == 'philipshue') {
        get_hue_lights();
      }
      if (hueType == 'philipshueentertainment') {
        get_hue_groups();
      }
    }
  }
  else {
    //abort checking, first reachable result is used
    $('#wiz_hue_usrstate').html($.i18n('wiz_hue_failure_user'));
    $('#wiz_hue_create_user').toggle(true);
  }
};

function useGroupId(id) {
  $('#groupId').val(id);
  groupLights = hueGroups[id].lights;
  groupLightsLocations = hueGroups[id].locations;
  get_hue_lights();
}

async function discover_hue_bridges() {
  $('#wiz_hue_ipstate').html($.i18n('edt_dev_spec_devices_discovery_inprogress'));
  $('#wiz_hue_discovered').html("")
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

          //Remap https port to http port until Hue-API v2 is supported
          if (port == 443) {
            port = 80;
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

      var usr = $('#user').val();
      if (usr != "") {
        checkHueBridge(checkUserResult, usr);
      } else {
        checkHueBridge(checkBridgeResult);
      }
    }
  }
}

async function getProperties_hue_bridge(cb, hostAddress, port, username, resourceFilter) {
  let params = { host: hostAddress, user: username, filter: resourceFilter };
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
  if (devicesProperties[ledType][key]) {
    cb(true, username);
  } else {
    const res = await requestLedDeviceProperties(ledType, params);


    if (res && !res.error) {
      var ledDeviceProperties = res.info.properties;
      if (!jQuery.isEmptyObject(ledDeviceProperties)) {

        if (username === "config") {
          if (ledDeviceProperties.name && ledDeviceProperties.bridgeid && ledDeviceProperties.modelid) {
            $('#wiz_hue_discovered').html("Bridge: " + ledDeviceProperties.name + ", Modelid: " + ledDeviceProperties.modelid + ", API-Version: " + ledDeviceProperties.apiversion);
            cb(true);
          }
        } else {
          devicesProperties[ledType][key] = ledDeviceProperties;
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

async function identify_hue_device(hostAddress, port, username, id) {
  var disabled = $('#btn_wiz_save').is(':disabled');
  // Take care that new record cannot be save during background process
  $('#btn_wiz_save').prop('disabled', true);

  let params = { host: decodeURIComponent(hostAddress), user: username, lightId: id };

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

  if (hueType == 'philipshueentertainment') {
    var clkey = eV("clientkey");
    if (clkey != "") {
      $('#clientkey').val(clkey);
    }
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
    hueIPs.unshift({ host: host, port: port });

    if (usr != "") {
      checkHueBridge(checkUserResult, usr);
    } else {
      checkHueBridge(checkBridgeResult);
    }
  }

  $('#retry_bridge').off().on('click', function () {

    if ($('#host').val() != "") {

      hueIPs = [];
      hueIPsinc = 0;
      hueIPs.push({ host: $('#host').val(), port: $('#port').val() });
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
    checkHueBridge(checkUserResult, $('#user').val() ? $('#user').val() : "newdeveloper");
  });

  $('#wiz_hue_create_user').off().on('click', function () {
    if ($('#host').val() != "") {
      hueIPs.unshift({ host: $('#host').val(), port: $('#port').val() });
    }
    createHueUser();
  });

  $('#btn_wiz_save').off().on("click", function () {
    var hueLedConfig = [];
    var finalLightIds = [];

    //create hue led config
    for (var key in hueLights) {
      if (hueType == 'philipshueentertainment') {
        if (groupLights.indexOf(key) == -1) continue;
      }
      if ($('#hue_' + key).val() != "disabled") {
        finalLightIds.push(key);
        var idx_content = assignLightPos(key, $('#hue_' + key).val(), hueLights[key].name);
        hueLedConfig.push(JSON.parse(JSON.stringify(idx_content)));
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
    d.groupId = parseInt($('#groupId').val());
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

    if (hueType == 'philipshue') {
      d.useEntertainmentAPI = false;
      d.hardwareLedCount = finalLightIds.length;
      d.verbose = false;
      if (window.serverConfig.device.type !== d.type) {
        //smoothing off, if new device
        sc.smoothing = { enable: false };
      }
    }

    if (hueType == 'philipshueentertainment') {
      d.useEntertainmentAPI = true;
      d.hardwareLedCount = groupLights.length;
      if (window.serverConfig.device.type !== d.type) {
        //smoothing on, if new device
        sc.smoothing = { enable: true };
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
            if (hueType == 'philipshueentertainment') {
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

function get_hue_groups() {

  var host = hueIPs[hueIPsinc].host;

  if (devicesProperties['philipshue'][host]) {
    var ledProperties = devicesProperties['philipshue'][host];

    if (!jQuery.isEmptyObject(ledProperties)) {
      hueGroups = ledProperties.groups;
      if (Object.keys(hueGroups).length > 0) {

        $('.lidsb').html("");
        $('#wh_topcontainer').toggle(false);
        $('#hue_grp_ids_t').toggle(true);

        var gC = 0;
        for (var groupid in hueGroups) {
          if (hueGroups[groupid].type == 'Entertainment') {
            $('.gidsb').append(createTableRow([groupid + ' (' + hueGroups[groupid].name + ')', '<button class="btn btn-sm btn-primary" onClick=useGroupId(' + groupid + ')>' + $.i18n('wiz_hue_e_use_groupid', groupid) + '</button>']));
            gC++;
          }
        }
        if (gC == 0) {
          noAPISupport('wiz_hue_e_noegrpids');
        }
      }
    }
  }
}

function noAPISupport(txt) {
  showNotification('danger', $.i18n('wiz_hue_e_title'), $.i18n('wiz_hue_e_noapisupport_hint'));
  conf_editor.getEditor("root.specificOptions.useEntertainmentAPI").setValue(false);
  $("#root_specificOptions_useEntertainmentAPI").trigger("change");
  $('#btn_wiz_holder').append('<div class="bs-callout bs-callout-danger" style="margin-top:0px">' + $.i18n('wiz_hue_e_noapisupport_hint') + '</div>');
  $('#hue_grp_ids_t').toggle(false);
  var txt = (txt) ? $.i18n(txt) : $.i18n('wiz_hue_e_nogrpids');
  $('<p style="font-weight:bold;color:red;">' + txt + '<br />' + $.i18n('wiz_hue_e_noapisupport') + '</p>').insertBefore('#wizp2_body #hue_ids_t');
  $('#hue_id_headline').html($.i18n('wiz_hue_desc2'));
  hueType = 'philipshue';
  get_hue_lights();
}

function get_hue_lights() {

  var host = hueIPs[hueIPsinc].host;

  if (devicesProperties['philipshue'][host]) {
    var ledProperties = devicesProperties['philipshue'][host];

    if (!jQuery.isEmptyObject(ledProperties.lights)) {
      hueLights = ledProperties.lights;
      if (Object.keys(hueLights).length > 0) {
        if (hueType == 'philipshue') {
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

        if (hueType == 'philipshue') {
          lightOptions.unshift("disabled");
        }

        $('.lidsb').html("");
        var pos = "";
        for (var lightid in hueLights) {
          if (hueType == 'philipshueentertainment') {
            if (groupLights.indexOf(lightid) == -1) continue;

            if (groupLightsLocations.hasOwnProperty(lightid)) {
              lightLocation = groupLightsLocations[lightid];
              var x = lightLocation[0];
              var y = lightLocation[1];
              var z = lightLocation[2];
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
          var options = "";
          for (var opt in lightOptions) {
            var val = lightOptions[opt];
            var txt = (val != 'entire' && val != 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
            options += '<option value="' + val + '"';
            if (pos == val) options += ' selected="selected"';
            options += '>' + $.i18n(txt + val) + '</option>';
          }
          $('.lidsb').append(createTableRow([lightid + ' (' + hueLights[lightid].name + ')', '<select id="hue_' + lightid + '" class="hue_sel_watch form-control">'
            + options
            + '</select>', '<button class="btn btn-sm btn-primary" onClick=identify_hue_device("' + encodeURIComponent($("#host").val()) + '","' + $('#port').val() + '","' + $("#user").val() + '",' + lightid + ')>' + $.i18n('wiz_hue_blinkblue', lightid) + '</button>']));
        }

        if (hueType != 'philipshueentertainment') {
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

        var idx_content = assignLightPos(key, $('#yee_' + key).val(), name);
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

        var idx_content = assignLightPos(key, $('#orb_' + key).val(), name);
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

