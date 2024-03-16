// RGTB byte order wizard

let wIntveralId;
let new_rgb_order;

function changeColor() {
  let color = $("#wiz_canv_color").css('background-color');

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

function stopWizardRGB(reload) {
  console.log("stopWizardRGB - reload: ", reload);
  clearInterval(wIntveralId);
  resetWizard(reload);
}


function beginWizardRGB() {
  $("#wiz_switchtime_select").off().on('change', function () {
    clearInterval(wIntveralId);
    const time = $("#wiz_switchtime_select").val();
    wIntveralId = setInterval(function () { changeColor(); }, time * 1000);
  });

  $('.wselect').on("change", function () {
    let rgb_order = window.serverConfig.device.colorOrder.split("");
    const redS = $("#wiz_r_select").val();
    const greenS = $("#wiz_g_select").val();
    const blueS = rgb_order.toString().replace(/,/g, "").replace(redS, "").replace(greenS, "");

    for (const color of rgb_order) {
      if (redS == color)
        $('#wiz_g_select option[value=' + color + ']').prop('disabled', true);
      else
        $('#wiz_g_select option[value=' + color + ']').prop('disabled', false);
      if (greenS == color)
        $('#wiz_r_select option[value=' + color + ']').prop('disabled', true);
      else
        $('#wiz_r_select option[value=' + color + ']').prop('disabled', false);
    }

    if (redS != 'null' && greenS != 'null') {
      $('#btn_wiz_save').prop('disabled', false);

      for (let i = 0; i < rgb_order.length; i++) {
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

  $('#btn_wiz_abort').off().on('click', function () { stopWizardRGB(true); });

  $('#btn_wiz_checkok').off().on('click', function () {
    showInfoDialog('success', "", $.i18n('infoDialog_wizrgb_text'));
    stopWizardRGB();
  });

  $('#btn_wiz_save').off().on('click', function () {
    stopWizardRGB();
    window.serverConfig.device.colorOrder = new_rgb_order;
    requestWriteConfig({ "device": window.serverConfig.device });
  });
}

