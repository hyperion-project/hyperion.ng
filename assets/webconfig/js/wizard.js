//clear priority and other tasks if people reload the page or lost connection while a wizard was active
$(window.hyperion).one("ready", function(event) {
  if(getStorage("wizardactive") === 'true')
  {
    requestPriorityClear();
    setStorage("wizardactive", false);
    if(getStorage("kodiAddress") != null)
    {
      kodiAddress = getStorage("kodiAddress");
      sendToKodi("stop");
    }
  }
});

function resetWizard(reload)
{
  $("#wizard_modal").modal('hide');
  clearInterval(wIntveralId);
  requestPriorityClear();
  setStorage("wizardactive", false);
  $('#wizp1').toggle(true);
  $('#wizp2').toggle(false);
  $('#wizp3').toggle(false);
  //cc
  if(withKodi)
    sendToKodi("stop");
  step = 0;
  if(!reload) location.reload();
}

//rgb byte order wizard
var wIntveralId;
var new_rgb_order;

function changeColor()
{
  var color = $("#wiz_canv_color").css('background-color');

  if (color == 'rgb(255, 0, 0)')
  {
    $("#wiz_canv_color").css('background-color','rgb(0, 255, 0)');
    requestSetColor('0','255','0');
  }
  else
  {
    $("#wiz_canv_color").css('background-color','rgb(255, 0, 0)');
    requestSetColor('255','0','0');
  }
}

function startWizardRGB()
{
  //create html
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_rgb_title'));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_rgb_title')+'</h4><p>'+$.i18n('wiz_rgb_intro1')+'</p><p style="font-weight:bold;">'+$.i18n('wiz_rgb_intro2')+'</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
  $('#wizp2_body').html('<p style="font-weight:bold">'+$.i18n('wiz_rgb_expl')+'</p>');
  $('#wizp2_body').append('<div class="form-group"><label>'+$.i18n('wiz_rgb_switchevery')+'</label><div class="input-group" style="width:100px"><select id="wiz_switchtime_select" class="form-control"></select><div class="input-group-addon">'+$.i18n('edt_append_s')+'</div></div></div>');
  $('#wizp2_body').append('<canvas id="wiz_canv_color" width="100" height="100" style="border-radius:60px;background-color:red; display:block; margin: 10px 0;border:4px solid grey;"></canvas><label>'+$.i18n('wiz_rgb_q')+'</label>');
  $('#wizp2_body').append('<table class="table borderless" style="width:200px"><tbody><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qrend')+'</label></td><td class="itd"><select id="wiz_r_select" class="form-control wselect"></select></td></tr><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qgend')+'</label></td><td class="itd"><select id="wiz_g_select" class="form-control wselect"></select></td></tr></tbody></table>');
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_save')+'</button><button type="button" class="btn btn-primary" id="btn_wiz_checkok" style="display:none" data-dismiss="modal"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_ok')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');

  //open modal
  $("#wizard_modal").modal({
    backdrop : "static",
    keyboard: false,
    show: true
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click',function() {
    beginWizardRGB();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function beginWizardRGB()
{
  $("#wiz_switchtime_select").off().on('change',function() {
    clearInterval(wIntveralId);
    var time = $("#wiz_switchtime_select").val();
    wIntveralId = setInterval(function() { changeColor(); }, time*1000);
  });

  $('.wselect').change(function () {
    var rgb_order = window.serverConfig.device.colorOrder.split("");
    var redS = $("#wiz_r_select").val();
    var greenS = $("#wiz_g_select").val();
    var blueS = rgb_order.toString().replace(/,/g,"").replace(redS, "").replace(greenS,"");

    for (var i = 0; i<rgb_order.length; i++)
    {
      if (redS == rgb_order[i])
        $('#wiz_g_select option[value='+rgb_order[i]+']').attr('disabled',true);
      else
        $('#wiz_g_select option[value='+rgb_order[i]+']').attr('disabled',false);
      if (greenS == rgb_order[i])
        $('#wiz_r_select option[value='+rgb_order[i]+']').attr('disabled',true);
      else
        $('#wiz_r_select option[value='+rgb_order[i]+']').attr('disabled',false);
    }

    if(redS != 'null' && greenS != 'null')
    {
      $('#btn_wiz_save').attr('disabled',false);

      for (var i = 0; i<rgb_order.length; i++)
      {
        if(rgb_order[i] == "r")
          rgb_order[i] = redS;
        else if(rgb_order[i] == "g")
          rgb_order[i] = greenS;
        else
          rgb_order[i] = blueS;
      }

      rgb_order = rgb_order.toString().replace(/,/g,"");

      if(redS == "r" && greenS == "g")
      {
        $('#btn_wiz_save').toggle(false);
        $('#btn_wiz_checkok').toggle(true);
      }
      else
      {
        $('#btn_wiz_save').toggle(true);
        $('#btn_wiz_checkok').toggle(false);
      }
      new_rgb_order = rgb_order;
    }
    else
      $('#btn_wiz_save').attr('disabled',true);
  });

  $("#wiz_switchtime_select").append(createSelOpt('5','5'),createSelOpt('10','10'),createSelOpt('15','15'),createSelOpt('30','30'));
  $("#wiz_switchtime_select").trigger('change');

  $("#wiz_r_select").append(createSelOpt("null", ""),createSelOpt('r', $.i18n('general_col_red')),createSelOpt('g', $.i18n('general_col_green')),createSelOpt('b', $.i18n('general_col_blue')));
  $("#wiz_g_select").html($("#wiz_r_select").html());
  $("#wiz_r_select").trigger('change');

  requestSetColor('255','0','0');
  setTimeout(requestSetSource, 100, 'auto');
  setStorage("wizardactive", true);

  $('#btn_wiz_abort').off().on('click',function() { resetWizard(true); });

  $('#btn_wiz_checkok').off().on('click',function() {
    showInfoDialog('success', "", $.i18n('infoDialog_wizrgb_text'));
    resetWizard();
  });

  $('#btn_wiz_save').off().on('click',function() {
    resetWizard();
    window.serverConfig.device.colorOrder = new_rgb_order;
    requestWriteConfig({"device" : window.serverConfig.device});
  });
}

$('#btn_wizard_byteorder').off().on('click',startWizardRGB);

//color calibration wizard
var kodiAddress = document.location.hostname+':8080';
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
var availVideos = ["Sweet_Cocoon","Caminandes_2_GranDillama","Caminandes_3_Llamigos"];

if(getStorage("kodiAddress") != null)
  kodiAddress = getStorage("kodiAddress");

function switchPicture(pictures)
{
  if(typeof pictures[picnr] === 'undefined')
    picnr = 0;

  sendToKodi('playP',pictures[picnr]);
  picnr++;
}

function sendToKodi(type, content, cb)
{
  var command;

  if(type == "playP")
    content = imgAddress+content+'.png';
  if(type == "playV")
    content = vidAddress+content;

  if(type == "msg")
    command = '{"jsonrpc":"2.0","method":"GUI.ShowNotification","params":{"title": "'+$.i18n('wiz_cc_title')+'", "message": "'+content+'", "image":"info", "displaytime":5000 },"id":"1"}';
  else if (type == "stop")
    command = '{"jsonrpc":"2.0","method":"Player.Stop","params":{"playerid": 2},"id":"1"}';
  else if (type == "playP" || type == "playV")
    command = '{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"file":"' + content + '"}},"id":"1"}';
  else if (type == "rotate")
    command = '{"jsonrpc":"2.0","method":"Player.Rotate","params":{"playerid": 2},"id":"1"}';

  $.ajax({
    url: 'http://' + kodiAddress + '/jsonrpc',
    dataType: 'jsonp',
    crossDomain: true,
    jsonpCallback: 'jsonCallback',
    type: 'POST',
    timeout: 2000,
    data: 'request=' + encodeURIComponent( command )
  })
  .done( function( data, textStatus, jqXHR ) {
    if ( jqXHR.status == 200 && data['result'] == 'OK' && type == "msg")
      cb("success");
  })
  // Older Versions Of Kodi/XBMC Tend To Fail Due To CORS But Typically If A '200' Is Returned Then It Has Worked!
  .fail( function( jqXHR, textStatus ) {
    if ( jqXHR.status != 200 && type == "msg")
      cb("error")
  });
}

function performAction()
{
  var h;

  if(step == 1)
  {
    $('#wiz_cc_desc').html($.i18n('wiz_cc_chooseid'));
    updateWEditor(["id"]);
    $('#btn_wiz_back').attr("disabled", true);
  }
  else
    $('#btn_wiz_back').attr("disabled", false);

  if(step == 2)
  {
    updateWEditor(["white"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_white_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_white_title'));
      sendToKodi('playP',"white");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_white_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 3)
  {
    updateWEditor(["gammaRed","gammaGreen","gammaBlue"]);
    h = '<p>'+$.i18n('wiz_cc_adjustgamma')+'</p>';
    if(withKodi)
    {
      sendToKodi('playP',"HGradient");
      h +='<button id="wiz_cc_btn_sp" class="btn btn-primary">'+$.i18n('wiz_cc_btn_switchpic')+'</button>';
    }
    else
      h += '<p>'+$.i18n('wiz_cc_lettvshowm', "gey_1, grey_2, grey_3, HGradient, VGradient")+'</p>';
    $('#wiz_cc_desc').html(h);
    $('#wiz_cc_btn_sp').off().on('click', function(){
      switchPicture(["VGradient","grey_1","grey_2","grey_3","HGradient"]);
    });
  }
  if(step == 4)
  {
    updateWEditor(["red"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_red_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_red_title'));
      sendToKodi('playP',"red");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_red_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 5)
  {
    updateWEditor(["green"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_green_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_green_title'));
      sendToKodi('playP',"green");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_green_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 6)
  {
    updateWEditor(["blue"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_blue_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_blue_title'));
      sendToKodi('playP',"blue");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_blue_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 7)
  {
    updateWEditor(["cyan"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_cyan_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_cyan_title'));
      sendToKodi('playP',"cyan");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_cyan_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 8)
  {
    updateWEditor(["magenta"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_magenta_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_magenta_title'));
      sendToKodi('playP',"magenta");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_magenta_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 9)
  {
    updateWEditor(["yellow"]);
    h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_yellow_title'));
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_yellow_title'));
      sendToKodi('playP',"yellow");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_yellow_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 10)
  {
    updateWEditor(["backlightThreshold","backlightColored"]);
    h = $.i18n('wiz_cc_backlight');
    if(withKodi)
    {
      h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_black_title'));
      sendToKodi('playP',"black");
    }
    else
      h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_black_title'));
    $('#wiz_cc_desc').html(h);
  }
  if(step == 11)
  {
    updateWEditor([""], true);
    h = '<p>'+$.i18n('wiz_cc_testintro')+'</p>';
    if(withKodi)
    {
      h += '<p>'+$.i18n('wiz_cc_testintrok')+'</p>';
      sendToKodi('stop');
      for(var i = 0; i<availVideos.length; i++)
      {
        var txt = availVideos[i].replace(/_/g," ");
        h +='<div><button id="'+availVideos[i]+'" class="btn btn-sm btn-primary videobtn"><i class="fa fa-fw fa-play"></i> '+txt+'</button></div>';
      }
      h +='<div><button id="stop" class="btn btn-sm btn-danger videobtn" style="margin-bottom:15px"><i class="fa fa-fw fa-stop"></i> '+$.i18n('wiz_cc_btn_stop')+'</button></div>';
    }
    else
      h += '<p>'+$.i18n('wiz_cc_testintrowok')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/vid/" target="_blank">'+$.i18n('wiz_cc_link')+'</a></p>';
    h += '<p>'+$.i18n('wiz_cc_summary')+'</p>';
    $('#wiz_cc_desc').html(h);

    $('.videobtn').off().on('click', function(e){
      if(e.target.id == "stop")
        sendToKodi("stop");
      else
        sendToKodi("playV",e.target.id+'.mp4');

      $(this).attr("disabled", true);
      setTimeout(function(){$('.videobtn').attr("disabled", false)},10000);
    });

    $('#btn_wiz_next').attr("disabled", true);
    $('#btn_wiz_save').toggle(true);
  }
  else
  {
    $('#btn_wiz_next').attr("disabled", false);
    $('#btn_wiz_save').toggle(false);
  }
}

function updateWEditor(el, all)
{
  for (var key in cobj)
  {
    if(all === true || el[0] == key || el[1] == key || el[2] == key)
      $('#editor_container_wiz [data-schemapath*=".'+profile+'.'+key+'"]').toggle(true);
    else
      $('#editor_container_wiz [data-schemapath*=".'+profile+'.'+key+'"]').toggle(false);
  }
}

function startWizardCC()
{
  //create html
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_cc_title'));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_cc_title')+'</h4><p>'+$.i18n('wiz_cc_intro1')+'</p><label>'+$.i18n('wiz_cc_kwebs')+'</label><input class="form-control" style="width:170px;margin:auto" id="wiz_cc_kodiip" type="text" placeholder="'+kodiAddress+'" value="'+kodiAddress+'" /><span id="kodi_status"></span><span id="multi_cali"></span>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont" disabled="disabled"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
  $('#wizp2_body').html('<div id="wiz_cc_desc" style="font-weight:bold"></div><div id="editor_container_wiz"></div>');
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_back"><i class="fa fa-fw fa-chevron-left"></i>'+$.i18n('general_btn_back')+'</button><button type="button" class="btn btn-primary" id="btn_wiz_next">'+$.i18n('general_btn_next')+'<i style="margin-left:4px;"class="fa fa-fw fa-chevron-right"></i></button><button type="button" class="btn btn-warning" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_save')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');

  //open modal
  $("#wizard_modal").modal({
    backdrop : "static",
    keyboard: false,
    show: true
  });

  $('#wiz_cc_kodiip').off().on('change',function() {
    kodiAddress = $(this).val();
    setStorage("kodiAddress", kodiAddress);
    sendToKodi("msg", $.i18n('wiz_cc_kodimsg_start'), function(cb){
      if(cb == "error")
      {
        $('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodidiscon')+'</p><p>'+$.i18n('wiz_cc_kodidisconlink')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">'+$.i18n('wiz_cc_link')+'</p>');
        withKodi = false;
      }
      else
      {
        $('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodicon')+'</p>');
        withKodi = true;
      }

      $('#btn_wiz_cont').attr('disabled', false);
    });
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click',function() {
    beginWizardCC();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });

  $('#wiz_cc_kodiip').trigger("change");
  colorLength = window.serverConfig.color.channelAdjustment;
  cobj = window.schema.color.properties.channelAdjustment.items.properties;
  websAddress = document.location.hostname+':'+window.serverConfig.webConfig.port;
  imgAddress = 'http://'+websAddress+'/img/cc/';
  setStorage("wizardactive", true);

  //check profile count
  if(colorLength.length > 1)
  {
    $('#multi_cali').html('<p style="font-weight:bold;">'+$.i18n('wiz_cc_morethanone')+'</p><select id="wiz_select" class="form-control" style="width:200px;margin:auto"></select>');
    for(var i = 0; i<colorLength.length; i++)
      $('#wiz_select').append(createSelOpt(i,i+1+' ('+colorLength[i].id+')'));

    $('#wiz_select').off().on('change', function(){
      profile = $(this).val();
    });
  }

  //prepare editor
  wiz_editor = createJsonEditor('editor_container_wiz', {
    color : window.schema.color
  }, true, true);

  $('#editor_container_wiz h4').toggle(false);
  $('#editor_container_wiz .btn-group').toggle(false);
  $('#editor_container_wiz [data-schemapath="root.color.imageToLedMappingType"]').toggle(false);
  for(var i = 0; i<colorLength.length; i++)
    $('#editor_container_wiz [data-schemapath*="root.color.channelAdjustment.'+i+'."]').toggle(false);
}

function beginWizardCC()
{

  $('#btn_wiz_next').off().on('click',function() {
    step++;
    performAction();
  });

  $('#btn_wiz_back').off().on('click',function() {
    step--;
    performAction();
  });

  $('#btn_wiz_abort').off().on('click', resetWizard);

  $('#btn_wiz_save').off().on('click',function() {
    requestWriteConfig(wiz_editor.getValue());
    resetWizard();
  });

  wiz_editor.on("change", function(e){
    var val = wiz_editor.getEditor('root.color.channelAdjustment.'+profile+'').getValue();
    var temp = JSON.parse(JSON.stringify(val));
    delete temp.leds
    requestAdjustment(JSON.stringify(temp),"",true);
  });

  step++
  performAction();
}

$('#btn_wizard_colorcalibration').off().on('click', startWizardCC);

// Layout positions
var lightPosTop         = {hmin: 0.15,	hmax: 0.85,	vmin: 0   ,	vmax: 0.2 };
var lightPosTopLeft     = {hmin: 0   ,	hmax: 0.15,	vmin: 0   ,	vmax: 0.15};
var lightPosTopRight    = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0   ,	vmax: 0.15};
var lightPosBottom      = {hmin: 0.15,	hmax: 0.85,	vmin: 0.8 ,	vmax: 1.0 };
var lightPosBottomLeft  = {hmin: 0   ,	hmax: 0.15,	vmin: 0.85,	vmax: 1.0 };
var lightPosBottomRight = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0.85,	vmax: 1.0 };
var lightPosLeft        = {hmin: 0   ,	hmax: 0.15,	vmin: 0.15,	vmax: 0.85};
var lightPosLeftTop     = {hmin: 0   ,	hmax: 0.15,	vmin: 0   ,	vmax: 0.5 };
var lightPosLeftMiddle  = {hmin: 0   ,	hmax: 0.15,	vmin: 0.25,	vmax: 0.75};
var lightPosLeftBottom  = {hmin: 0   ,	hmax: 0.15,	vmin: 0.5 ,	vmax: 1.0 };
var lightPosRight       = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0.15,	vmax: 0.85};
var lightPosRightTop    = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0   ,	vmax: 0.5 };
var lightPosRightMiddle = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0.25,	vmax: 0.75};
var lightPosRightBottom = {hmin: 0.85,	hmax: 1.0 ,	vmin: 0.5 ,	vmax: 1.0 };
var lightPosEntire      = {hmin: 0.0 ,	hmax: 1.0 ,	vmin: 0.0 ,	vmax: 1.0 };

function assignLightPos(id, pos, name)
{
  var i = null;

  if(pos === "top")
	i = lightPosTop;
  else if(pos === "topleft")
	i = lightPosTopLeft;
  else if(pos === "topright")
	i = lightPosTopRight;
  else if(pos === "bottom")
	i = lightPosBottom;
  else if(pos === "bottomleft")
	i = lightPosBottomLeft;
  else if(pos === "bottomright")
	i = lightPosBottomRight;
  else if(pos === "left")
	i = lightPosLeft;
  else if(pos === "lefttop")
	i = lightPosLeftTop;
  else if(pos === "leftmiddle")
	i = lightPosLeftMiddle;
  else if(pos === "leftbottom")
	i = lightPosLeftBottom;
  else if(pos === "right")
	i = lightPosRight;
  else if(pos === "righttop")
	i = lightPosRightTop;
  else if(pos === "rightmiddle")
	i = lightPosRightMiddle;
  else if(pos === "rightbottom")
	i = lightPosRightBottom;
  else
	i = lightPosEntire;

  i.name = name;
  return i;
}

//****************************
// Wizard Philips Hue
//****************************

var hueIPs = [];
var hueIPsinc = 0;
var lightIDs = null;
var groupIDs = null;
var lightLocation = [];
var groupLights = [];
var groupLightsLocations = [];
var hueType = "philipshue";

function startWizardPhilipsHue(e)
{
  if(typeof e.data.type != "undefined") hueType = e.data.type;

  //create html

  var hue_title = 'wiz_hue_title';
  var hue_intro1 = 'wiz_hue_intro1';
  var hue_desc1 = 'wiz_hue_desc1';
  var hue_create_user = 'wiz_hue_create_user';
  if(hueType == 'philipshueentertainment')
  {
    hue_title = 'wiz_hue_e_title';
    hue_intro1 = 'wiz_hue_e_intro1';
    hue_desc1 = 'wiz_hue_e_desc1';
    hue_create_user = 'wiz_hue_e_create_user';
  }
  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n(hue_title));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n(hue_title)+'</h4><p>'+$.i18n(hue_intro1)+'</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
  $('#wizp2_body').html('<div id="wh_topcontainer"></div>');
  $('#wh_topcontainer').append('<p style="font-weight:bold">'+$.i18n(hue_desc1)+'</p><div class="form-group"><label>'+$.i18n('wiz_hue_ip')+'</label><div class="input-group" style="width:175px"><input type="text" class="input-group form-control" id="ip"><span class="input-group-addon" id="retry_bridge" style="cursor:pointer"><i class="fa fa-refresh"></i></span></div></div><span style="font-weight:bold;color:red" id="wiz_hue_ipstate"></span><span style="font-weight:bold;" class="component-on" id="wiz_hue_discovered"></span>');
  $('#wh_topcontainer').append();
  $('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');
  if(hueType == 'philipshue')
  {
    $('#usrcont').append('<label>'+$.i18n('wiz_hue_username')+'</label><div class="input-group" style="width:250px"><input type="text" class="form-control" id="user"><span class="input-group-addon" id="retry_usr" style="cursor:pointer"><i class="fa fa-refresh"></i></span></div>');
  }
  if(hueType == 'philipshueentertainment')
  {
    $('#usrcont').append('<label>'+$.i18n('wiz_hue_username')+'</label><div class="input-group" style="width:250px"><input type="text" class="form-control" id="user"></div><label>'+$.i18n('wiz_hue_clientkey')+'</label><div class="input-group" style="width:250px"><input type="text" class="form-control" id="clientkey"><span class="input-group-addon" id="retry_usr" style="cursor:pointer"><i class="fa fa-refresh"></i></span></div><input type="hidden" id="groupId">');
  }
  $('#usrcont').append('<span style="font-weight:bold;color:red" id="wiz_hue_usrstate"></span><br><button type="button" class="btn btn-primary" style="display:none" id="wiz_hue_create_user"> <i class="fa fa-fw fa-plus"></i>'+$.i18n(hue_create_user)+'</button>');
  if(hueType == 'philipshueentertainment')
  {
    $('#wizp2_body').append('<div id="hue_grp_ids_t" style="display:none"><p style="font-weight:bold">'+$.i18n('wiz_hue_e_desc2')+'</p></div>');
    createTable("gidsh", "gidsb", "hue_grp_ids_t");
    $('.gidsh').append(createTableRow([$.i18n('edt_dev_spec_groupId_title'),$.i18n('wiz_hue_e_use_group')], true));
    $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p style="font-weight:bold" id="hue_id_headline">'+$.i18n('wiz_hue_e_desc3')+'</p></div>');
  }
  else
  {
    $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p style="font-weight:bold" id="hue_id_headline">'+$.i18n('wiz_hue_desc2')+'</p></div>');
  }
  createTable("lidsh", "lidsb", "hue_ids_t");
  $('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lightid_title'),$.i18n('wiz_pos'),$.i18n('wiz_identify')], true));
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_save')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
  $('#wizp3_body').html('<span>'+$.i18n('wiz_hue_press_link')+'</span> <br /><br /><center><span id="connectionTime"></span><br /><i class="fa fa-cog fa-spin" style="font-size:100px"></i></center>');

  //open modal
  $("#wizard_modal").modal({
    backdrop : "static",
    keyboard: false,
    show: true
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click',function() {
    beginWizardHue();
    $('#wizp1').toggle(false);
    $('#wizp2').toggle(true);
  });
}

function checkHueBridge(cb,hueUser) {
  var usr = (typeof hueUser != "undefined") ? hueUser : 'config';
  if(usr == 'config') $('#wiz_hue_discovered').html("");
  $.ajax({
    url: 'http://'+hueIPs[hueIPsinc].internalipaddress+'/api/'+usr,
    type: "GET",
    dataType: "json",
    success: function( json ) {
      if (json.config) {
        cb(true, usr);
      } else if( json.name && json.bridgeid && json.modelid) {
        $('#wiz_hue_discovered').html("Bridge: " + json.name + ", Modelid: " + json.modelid + ", API-Version: " + json.apiversion);
        cb(true);
      } else {
        cb(false);
      }

     },
    timeout: 2500
  }).fail(function() {
    cb(false);
  });
}

function checkBridgeResult(reply, usr){
  if(reply)
  {
    //abort checking, first reachable result is used
    $('#wiz_hue_ipstate').html("");
    $('#ip').val(hueIPs[hueIPsinc].internalipaddress)

    //now check hue user on this bridge
    $('#usrcont').toggle(true);
    checkHueBridge(checkUserResult,$('#user').val() ? $('#user').val() : "newdeveloper");
  }
  else
  {
    //increment and check again
    if(hueIPs.length-1 > hueIPsinc)
    {
      hueIPsinc++;
      checkHueBridge(checkBridgeResult);
    }
    else
    {
      $('#usrcont').toggle(false);
      $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
    }
  }
};

function checkUserResult(reply, usr) {

  if(reply)
  {
    $('#user').val(usr);
    if(hueType == 'philipshueentertainment' && $('#clientkey').val() == "") {
      $('#usrcont').toggle(true);
      $('#wiz_hue_usrstate').html($.i18n('wiz_hue_e_clientkey_needed'));
      $('#wiz_hue_create_user').toggle(true);
    } else {
      $('#wiz_hue_usrstate').html("");
      $('#wiz_hue_create_user').toggle(false);
      if(hueType == 'philipshue')
      {
        get_hue_lights();
      }
      if(hueType == 'philipshueentertainment')
      {
        get_hue_groups();
      }
    }
  }
  else
  {
    $('#wiz_hue_usrstate').html($.i18n('wiz_hue_failure_user'));
    $('#wiz_hue_create_user').toggle(true);
  }
};

function identHueId(id, off, oState)
{
  if(off !== true)
  {
    setTimeout(identHueId,1500,id,true,oState);
    var put_data = '{"on":true,"bri":254,"hue":47000,"sat":254}';
  }
  else
  {
    var put_data = '{"on":'+oState.on+',"bri":' + oState.bri +',"hue":' + oState.hue +',"sat":' + oState.sat +'}';
  }

  $.ajax({
    url: 'http://'+$('#ip').val()+'/api/'+$('#user').val()+'/lights/'+id+'/state',
    type: 'PUT',
    timeout: 2000,
    data: put_data
  })
}

function useGroupId(id)
{
  $('#groupId').val(id);
  groupLights = groupIDs[id].lights;
  groupLightsLocations = groupIDs[id].locations;
  get_hue_lights();
}

async function discover_hue_bridges(){

	const res = await requestLedDeviceDiscovery ('philipshue');
	
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info;

		// Process devices returned by discovery
		console.log(r);
		
		if(r.devices.length == 0)
			$('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
		else
		{
			for(const device of r.devices)
			{
				console.log("Device:", device);

				var ip = device.hostname + ":" + device.port;
				console.log("Host:", ip);

				hueIPs.push({internalipaddress : ip});
			}
			var usr = $('#user').val();
			if(usr != "") {
			   checkHueBridge(checkUserResult, usr);
			} else {
			    checkHueBridge(checkBridgeResult);
			  }
		}
	}	
}

async function getProperties_hue_bridge(hostAddress, username, resourceFilter){

	let params = { host: hostAddress, user: username, filter: resourceFilter};
	
	const res = await requestLedDeviceProperties ('philipshue', params);
	
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process properties returned
		console.log(r);
	}	
}

function identify_hue_device(hostAddress, username, id){

	console.log("identify_hue_device");

	let params = { host: hostAddress, user: username, lightId: id };
		
	const res = requestLedDeviceIdentification ("philipshue", params);
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info
		console.log(r);
	}	
}

function getHueIPs(){
  $('#wiz_hue_ipstate').html($.i18n('wiz_hue_searchb'));
  $.ajax({
    url: 'https://discovery.meethue.com',
    crossDomain: true,
    type: 'GET',
    timeout: 3000
  })
  .done( function( data, textStatus, jqXHR ) {
    if(data.length == 0) {
      $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
    } else {
      hueIPs = data;
      checkHueBridge(checkBridgeResult);
    }
  })
  .fail( function( jqXHR, textStatus ) {
    $('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
  });
};

//return editor Value
function eV(vn)
{
  return (vn) ? conf_editor.getEditor("root.specificOptions." + vn).getValue() : "";
}

function beginWizardHue()
{
  var usr = eV("username");
  if(usr != "") {
    $('#user').val(usr);
  }

  if(hueType == 'philipshueentertainment') {
    var clkey = eV("clientkey");
    if(clkey != "")
    {
      $('#clientkey').val(clkey);
    }
  }
  //check if ip is empty/reachable/search for bridge
  if(eV("output") == "")
  {
    //getHueIPs();
    discover_hue_bridges();
  }
  else
  {
    var ip = eV("output");
    $('#ip').val(ip);
    hueIPs.unshift({internalipaddress : ip});
    if(usr != "") {
      checkHueBridge(checkUserResult, usr);
    }else{
      checkHueBridge(checkBridgeResult);
    }
  }

  $('#retry_bridge').off().on('click', function(){
    if($('#ip').val()!="") 
	{
		hueIPs.unshift({internalipaddress : $('#ip').val()})
    	hueIPsinc = 0;
    }
    else  discover_hue_bridges();

    var usr = $('#user').val();
    if(usr != "") {
      checkHueBridge(checkUserResult, usr);
    }else{
      checkHueBridge(checkBridgeResult);
    }
  });

  $('#retry_usr').off().on('click', function(){
    checkHueBridge(checkUserResult,$('#user').val() ? $('#user').val() : "newdeveloper");
  });

  $('#wiz_hue_create_user').off().on('click',function() {
    if($('#ip').val()!="") hueIPs.unshift({internalipaddress : $('#ip').val()});
    createHueUser();
  });

  $('#btn_wiz_save').off().on("click", function(){
    var hueLedConfig = [];
    var finalLightIds = [];

    //create hue led config
    for(var key in lightIDs)
    {
      if(hueType == 'philipshueentertainment')
      {
        if(groupLights.indexOf(key) == -1) continue;
      }
      if($('#hue_'+key).val() != "disabled")
      {
        finalLightIds.push(key);
		var idx_content = assignLightPos(key, $('#hue_'+key).val(), lightIDs[key].name);
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
    d.output                = $('#ip').val();
    d.username              = $('#user').val();
    d.type                  = 'philipshue';
    d.colorOrder            = 'rgb';
    d.lightIds              = finalLightIds;
    d.latchTime             = 0;
    d.transitiontime        = parseInt(eV("transitiontime") );
    d.restoreOriginalState  = (eV("restoreOriginalState") == true);
    d.switchOffOnBlack      = (eV("switchOffOnBlack") == true);
    d.brightnessFactor      = parseFloat(eV("brightnessFactor"));

    d.clientkey             = $('#clientkey').val();
    d.groupId               = parseInt($('#groupId').val());
    d.blackLightsTimeout    = parseInt(eV("blackLightsTimeout"));
    d.brightnessMin         = parseFloat(eV("brightnessMin"));
    d.brightnessMax         = parseFloat(eV("brightnessMax"));
    d.brightnessThreshold   = parseFloat(eV("brightnessThreshold"));
    d.sslReadTimeout        = parseInt(eV("sslReadTimeout"));
    d.sslHSTimeoutMin       = parseInt(eV("sslHSTimeoutMin"));
    d.sslHSTimeoutMax       = parseInt(eV("sslHSTimeoutMax"));
    d.verbose               = (eV("verbose") == true);
    d.debugStreamer         = (eV("debugStreamer") == true);
    d.debugLevel            = (eV("debugLevel"));

    if(hueType == 'philipshue')
    {
      d.useEntertainmentAPI = false;
      d.hardwareLedCount    = finalLightIds.length;
      d.rewriteTime         = 0;
      d.verbose             = false;
      //smoothing off
      sc.smoothing.enable = false;
    }

    if(hueType == 'philipshueentertainment')
    {
      d.useEntertainmentAPI = true;
      d.hardwareLedCount    = groupLights.length;
      d.rewriteTime         = 20;
      //smoothing on
      sc.smoothing.enable = true;
    }

    window.serverConfig.device = d;

    requestWriteConfig(sc, true);
    resetWizard();
  });

  $('#btn_wiz_abort').off().on('click', resetWizard);
}

function createHueUser()
{
  var connectionRetries = 30;
  var data = {"devicetype":"hyperion#"+Date.now()}
  if(hueType == 'philipshueentertainment')
  {
    data = {"devicetype":"hyperion#"+Date.now(), "generateclientkey":true}
  }
  var UserInterval = setInterval(function(){
  $.ajax({
    type: "POST",
    url: 'http://'+$("#ip").val()+'/api',
    processData: false,
    timeout: 1000,
    contentType: 'application/json',
    data: JSON.stringify(data),
    success: function(r) {
      $('#wizp1').toggle(false);
      $('#wizp2').toggle(false);
      $('#wizp3').toggle(true);

      connectionRetries--;
      $("#connectionTime").html(connectionRetries);
      if(connectionRetries == 0) {
        abortConnection(UserInterval);
      }
      else
      {
        if (typeof r[0].error != 'undefined') {
          console.log(connectionRetries+": link not pressed");
        }
        if (typeof r[0].success != 'undefined') {
          $('#wizp1').toggle(false);
          $('#wizp2').toggle(true);
          $('#wizp3').toggle(false);
          if(r[0].success.username != 'undefined') {
            $('#user').val(r[0].success.username);
            conf_editor.getEditor("root.specificOptions.username").setValue( r[0].success.username );
          }
          if(hueType == 'philipshueentertainment')
          {
            if(r[0].success.clientkey != 'undefined') {
              $('#clientkey').val(r[0].success.clientkey);
              conf_editor.getEditor("root.specificOptions.clientkey").setValue( r[0].success.clientkey );
            }
          }
          checkHueBridge(checkUserResult,r[0].success.username);
          clearInterval(UserInterval);
        }
      }
    },
    error: function(XMLHttpRequest, textStatus, errorThrown) {
      $('#wizp1').toggle(false);
      $('#wizp2').toggle(true);
      $('#wizp3').toggle(false);
      clearInterval(UserInterval);
    }
  });
  },1000);
}

function get_hue_groups(){
  $.ajax({
    type: "GET",
    url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/groups',
    processData: false,
    contentType: 'application/json',
    success: function(r) {
      if(Object.keys(r).length > 0)
      {
        $('#wh_topcontainer').toggle(false);
        $('#hue_grp_ids_t').toggle(true);

        groupIDs = r;

        var gC = 0;
        for(var groupid in r)
        {
          if(r[groupid].type=='Entertainment')
          {
            $('.gidsb').append(createTableRow([groupid+' ('+r[groupid].name+')', '<button class="btn btn-sm btn-primary" onClick=useGroupId('+groupid+')>'+$.i18n('wiz_hue_e_use_groupid',groupid)+'</button>']));
            gC++;
          }
        }
        if(gC == 0)
        {
          noAPISupport('wiz_hue_e_noegrpids');
        }
      }
      else
      {
        noAPISupport('wiz_hue_e_nogrpids');
      }
    }
  });
}

function noAPISupport(txt)
{
  showNotification('danger', $.i18n('wiz_hue_e_title'), $.i18n('wiz_hue_e_noapisupport_hint'));
  conf_editor.getEditor("root.specificOptions.useEntertainmentAPI").setValue( false );
  $("#root_specificOptions_useEntertainmentAPI").trigger("change");
  $('#btn_wiz_holder').append('<div class="bs-callout bs-callout-danger" style="margin-top:0px">'+$.i18n('wiz_hue_e_noapisupport_hint')+'</div>');
  $('#hue_grp_ids_t').toggle(false);
  var txt = (txt) ? $.i18n(txt) : $.i18n('wiz_hue_e_nogrpids');
  $('<p style="font-weight:bold;color:red;">'+txt+'<br />'+$.i18n('wiz_hue_e_noapisupport')+'</p>').insertBefore('#wizp2_body #hue_ids_t');
  $('#hue_id_headline').html($.i18n('wiz_hue_desc2'));
  hueType = 'philipshue';
  get_hue_lights();
}

function get_light_state(id){
  $.ajax({
    type: "GET",
    url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/lights/'+id,
    processData: false,
    contentType: 'application/json',
    success: function(r) {
      if(Object.keys(r).length > 0)
      {
        identHueId(id, false, r['state']);
      }
    }
  });
}

function get_hue_lights(){
  $.ajax({
    type: "GET",
    url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/lights',
    processData: false,
    contentType: 'application/json',
    success: function(r) {
      if(Object.keys(r).length > 0)
      {
        if(hueType == 'philipshue')
        {
          $('#wh_topcontainer').toggle(false);
        }
        $('#hue_ids_t, #btn_wiz_save').toggle(true);
        lightIDs = r;
        var lightOptions = [
              "top", "topleft", "topright",
              "bottom", "bottomleft", "bottomright",
              "left", "lefttop", "leftmiddle", "leftbottom",
              "right", "righttop", "rightmiddle", "rightbottom",
              "entire"
              ];

        if(hueType == 'philipshue')
        {
          lightOptions.unshift("disabled");
        }

        $('.lidsb').html("");
        var pos = "";
        for(var lightid in r)
        {
          if(hueType == 'philipshueentertainment')
          {
            if(groupLights.indexOf(lightid) == -1) continue;

            if(groupLightsLocations.hasOwnProperty(lightid))
            {
              lightLocation = groupLightsLocations[lightid];
              var x = lightLocation[0];
              var y = lightLocation[1];
              var z = lightLocation[2];
              var xval = (x < 0) ? "left" : "right";
              if(z != 1 && x >= -0.25 && x <= 0.25 ) xval = "";
              switch (z)
              {
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
          for(var opt in lightOptions)
          {
            var val = lightOptions[opt];
            var txt = (val != 'entire' && val != 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
            options+= '<option value="'+val+'"';
            if(pos == val) options+=' selected="selected"';
            options+= '>'+$.i18n(txt+val)+'</option>';
          }
          $('.lidsb').append(createTableRow([lightid+' ('+r[lightid].name+')', '<select id="hue_'+lightid+'" class="hue_sel_watch form-control">'
          + options
          + '</select>','<button class="btn btn-sm btn-primary" onClick=identify_hue_device("'+$("#ip").val()+'","'+$("#user").val()+'",'+lightid+')>'+$.i18n('wiz_hue_blinkblue',lightid)+'</button>']));
        }

        if(hueType != 'philipshueentertainment')
        {
          $('.hue_sel_watch').bind("change", function(){
            var cC = 0;
            for(var key in lightIDs)
            {
              if($('#hue_'+key).val() != "disabled")
              {
                cC++;
              }
            }
            cC == 0 ? $('#btn_wiz_save').attr("disabled",true) : $('#btn_wiz_save').attr("disabled",false);
          });
        }
        $('.hue_sel_watch').trigger('change');
      }
      else
      {
        var txt = '<p style="font-weight:bold;color:red;">'+$.i18n('wiz_hue_noids')+'</p>';
        $('#wizp2_body').append(txt);
      }
    }
  });
}

function abortConnection(UserInterval){
  clearInterval(UserInterval);
  $('#wizp1').toggle(false);
  $('#wizp2').toggle(true);
  $('#wizp3').toggle(false);
  $("#wiz_hue_usrstate").html($.i18n('wiz_hue_failure_connection'));
}

//****************************
// Wizard WLED
//****************************
var lights = null;
function startWizardWLED(e)
{
  //create html

  var wled_title = 'wiz_wled_title';
  var wled_intro1 = 'wiz_wled_intro1';

  $('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n(wled_title));
  $('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n(wled_title)+'</h4><p>'+$.i18n(wled_intro1)+'</p>');
  $('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');

  /*$('#wizp2_body').html('<div id="wh_topcontainer"></div>');

  $('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

  $('#wizp2_body').append('<div id="hue_ids_t" style="display:none"><p style="font-weight:bold" id="hue_id_headline">'+$.i18n('wiz_wled_desc2')+'</p></div>');

  createTable("lidsh", "lidsb", "hue_ids_t");
  $('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lights_title'),$.i18n('wiz_pos'),$.i18n('wiz_identify')], true));
  $('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_save')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
*/
  //open modal
  $("#wizard_modal").modal({
    backdrop : "static",
    keyboard: false,
    show: true
  });

  //listen for continue
  $('#btn_wiz_cont').off().on('click',function() {

// For testing only
  discover_wled();

  var hostAddress = conf_editor.getEditor("root.specificOptions.host").getValue();
  if(hostAddress != "")
  {
    getProperties_wled(hostAddress,"info");
    identify_wled(hostAddress)  
  }

// For testing only

  });
}

async function discover_wled(){

	const res = await requestLedDeviceDiscovery ('wled');

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process devices returned by discovery
		console.log(r);

		if(r.devices.length == 0)
			$('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
		else
		{
			for(const device of r.devices)
			{
				console.log("Device:", device);

				var ip = device.hostname + ":" + device.port;
				console.log("Host:", ip);

				//wledIPs.push({internalipaddress : ip});
			}
		}
	}
}

async function getProperties_wled(hostAddress, resourceFilter){

	let params = { host: hostAddress, filter: resourceFilter};

	const res = await requestLedDeviceProperties ('wled', params);

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process properties returned
		console.log(r);
	}
}

function identify_wled(hostAddress){

	let params = { host: hostAddress };

	const res = requestLedDeviceIdentification ("wled", params);
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){

		const r = res.info
		console.log(r);
	}
}

//****************************
// Wizard Yeelight
//****************************
var lights = null;
function startWizardYeelight(e)
{
	//create html

	var yeelight_title = 'wiz_yeelight_title';
	var yeelight_intro1 = 'wiz_yeelight_intro1';

	$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n(yeelight_title));
	$('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n(yeelight_title)+'</h4><p>'+$.i18n(yeelight_intro1)+'</p>');

	$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'
	+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'
	+$.i18n('general_btn_cancel')+'</button>');

	$('#wizp2_body').html('<div id="wh_topcontainer"></div>');

	$('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

	$('#wizp2_body').append('<div id="yee_ids_t" style="display:none"><p style="font-weight:bold" id="yee_id_headline">'+$.i18n('wiz_yeelight_desc2')+'</p></div>');

	createTable("lidsh", "lidsb", "yee_ids_t");
	$('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lights_title'),$.i18n('wiz_pos'),$.i18n('wiz_identify')], true));
	$('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'
	+$.i18n('general_btn_save')+'</button><buttowindow.serverConfig.device = d;n type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'
	+$.i18n('general_btn_cancel')+'</button>');

	//open modal
	$("#wizard_modal").modal({backdrop : "static", keyboard: false, show: true });

	//listen for continue
	$('#btn_wiz_cont').off().on('click',function() {
		beginWizardYeelight();
		$('#wizp1').toggle(false);
		$('#wizp2').toggle(true);
	});
}

function beginWizardYeelight()
{
	lights = [];
	configuredLights = conf_editor.getEditor("root.specificOptions.lights").getValue();

	discover_yeelight_lights();

	$('#btn_wiz_save').off().on("click", function(){
		var yeelightLedConfig = [];
		var finalLights = [];

		//create yeelight led config
		for(var key in lights)
		{
			if($('#yee_'+key).val() !== "disabled")
			{
				//delete lights[key].model;

				// Set Name to layout-position, if empty
				if ( lights[key].name === "" )
				{
					lights[key].name = $.i18n( 'conf_leds_layout_cl_'+$('#yee_'+key).val() );
				}

				finalLights.push( lights[key]);

				var name = lights[key].host;
				if ( lights[key].name !== "")
					name += '_'+lights[key].name;

				var idx_content = assignLightPos(key, $('#yee_'+key).val(), name);
				yeelightLedConfig.push(JSON.parse(JSON.stringify(idx_content)));
			}
		}

		//LED layout
		window.serverConfig.leds = yeelightLedConfig;

		//LED device config
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

		//smoothing off
		window.serverConfig.smoothing.enable = false;

		requestWriteConfig(window.serverConfig, true);
		resetWizard();
	});

	$('#btn_wiz_abort').off().on('click', resetWizard);
}

function getHostInLights(hostname) {
	return lights.filter(
				function(lights) {
					return lights.host === hostname
				}
				);
}

async function discover_yeelight_lights(){

  var light = {};
  // Get discovered lights
  const res = await requestLedDeviceDiscovery ('yeelight');

  // TODO: error case unhandled
  // res can be: false (timeout) or res.error (not found)
  if(res && !res.error){
    const r = res.info;

	// Process devices returned by discovery
	for(const device of r.devices)
	{
		//console.log("Device:", device);

	    if( device.hostname !== "")
	    {
		    if ( getHostInLights ( device.hostname ).length === 0 )
		    {
		      light = {};
		      light.host = device.hostname;
		      light.port = device.port;

		      light.name = device.other.name;
		      light.model = device.other.model;
		      lights.push(light);
		    }
	    }
	}

    // Add additional items from configuration
    for(var keyConfig in configuredLights)
    {

      var [host, port]= configuredLights[keyConfig].host.split(":", 2);

      //In case port has been explicitly provided, overwrite port given as part of hostname
      if ( configuredLights[keyConfig].port !== 0 )
        port = configuredLights[keyConfig].port;

      if ( host !== "" )
        if ( getHostInLights ( host ).length === 0 )
        {
          light = {};
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

function assign_yeelight_lights(){

	var models =  ['color', 'color1', 'color2', 'color4', 'stripe', 'strip1'];

	// If records are left for configuration
	if(Object.keys(lights).length > 0)
	{
		$('#wh_topcontainer').toggle(false);
		$('#yee_ids_t, #btn_wiz_save').toggle(true);

		var lightOptions = [
					"top", "topleft", "topright",
					"bottom", "bottomleft", "bottomright",
					"left", "lefttop", "leftmiddle", "leftbottom",
					"right", "righttop", "rightmiddle", "rightbottom",
					"entire"
				];

		lightOptions.unshift("disabled");

		$('.lidsb').html("");
		var pos = "";

		for(var lightid in lights)
		{
			var lightHostname = lights[lightid].host;
			var lightPort = lights[lightid].port;
			var lightName = lights[lightid].name;

			if ( lightName === "" )
				lightName = $.i18n('edt_dev_spec_lights_itemtitle');

			var options = "";
			for(var opt in lightOptions)
			{
				var val = lightOptions[opt];
				var txt = (val !== 'entire' && val !== 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
				options+= '<option value="'+val+'"';
				if(pos === val) options+=' selected="selected"';
				options+= '>'+$.i18n(txt+val)+'</option>';
			}

			var enabled = 'enabled'
			if (! models.includes (lights[lightid].model) )
			{
				var enabled = 'disabled';
				options = '<option value=disabled>'+$.i18n('wiz_yeelight_unsupported')+'</option>';
			}

			$('.lidsb').append(createTableRow([(parseInt(lightid, 10) + 1)+'. '+lightName+'<br>('+lightHostname+')', '<select id="yee_'+lightid+'" '+enabled+' class="yee_sel_watch form-control">'
											   + options
											   + '</select>','<button class="btn btn-sm btn-primary" onClick=identify_yeelight_device("'+lightHostname+'",'+lightPort+')>'
											   + $.i18n('wiz_identify_light',lightName)+'</button>']));
		}

		$('.yee_sel_watch').bind("change", function(){
			var cC = 0;
			for(var key in lights)
			{
				if($('#yee_'+key).val() !== "disabled")
				{
					cC++;
				}
			}
			if ( cC === 0)
				$('#btn_wiz_save').attr("disabled",true);
			else
				$('#btn_wiz_save').attr("disabled",false);
		});
		$('.yee_sel_watch').trigger('change');
	}
	else
	{
		var noLightsTxt = '<p style="font-weight:bold;color:red;">'+$.i18n('wiz_yeelight_noLights')+'</p>';
		$('#wizp2_body').append(noLightsTxt);
	}
}

async function getProperties_yeelight(hostname, port){

	let params = { hostname: hostname, port: port};

	const res = await requestLedDeviceProperties ('yeelight', params);

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process properties returned
		console.log(r);
	}	
}

function identify_yeelight_device(hostname, port){

	let params = { hostname: hostname, port: port };

	const res = requestLedDeviceIdentification ("yeelight", params);
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		//const r = res.info;
	}
}

//****************************
// Wizard AtmoOrb
//****************************
var lights = null;
function startWizardAtmoOrb(e)
{
	//create html

	var atmoorb_title = 'wiz_atmoorb_title';
	var atmoorb_intro1 = 'wiz_atmoorb_intro1';

	$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n(atmoorb_title));
	$('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n(atmoorb_title)+'</h4><p>'+$.i18n(atmoorb_intro1)+'</p>');

	$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'
	+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'
	+$.i18n('general_btn_cancel')+'</button>');

	$('#wizp2_body').html('<div id="wh_topcontainer"></div>');

	$('#wh_topcontainer').append('<div class="form-group" id="usrcont" style="display:none"></div>');

	$('#wizp2_body').append('<div id="orb_ids_t" style="display:none"><p style="font-weight:bold" id="orb_id_headline">'+$.i18n('wiz_atmoorb_desc2')+'</p></div>');

	createTable("lidsh", "lidsb", "orb_ids_t");
	$('.lidsh').append(createTableRow([$.i18n('edt_dev_spec_lights_title'),$.i18n('wiz_pos'),$.i18n('wiz_identify')], true));
	$('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'
	+$.i18n('general_btn_save')+'</button><buttowindow.serverConfig.device = d;n type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'
	+$.i18n('general_btn_cancel')+'</button>');

	//open modal
	$("#wizard_modal").modal({backdrop : "static", keyboard: false, show: true });

	//listen for continue
	$('#btn_wiz_cont').off().on('click',function() {
		beginWizardAtmoOrb();
		$('#wizp1').toggle(false);
		$('#wizp2').toggle(true);
	});
}

function beginWizardAtmoOrb()
{
	lights = [];
	configuredLights = [];

	configruedOrbIds =  conf_editor.getEditor("root.specificOptions.orbIds").getValue().trim();
	if ( configruedOrbIds.length !== 0 )
	{
	    configuredLights =  configruedOrbIds.split(",").map( Number );
	}

    var multiCastGroup = conf_editor.getEditor("root.specificOptions.output").getValue();
    var multiCastPort = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());

	discover_atmoorb_lights(multiCastGroup, multiCastPort);

	$('#btn_wiz_save').off().on("click", function(){
		var atmoorbLedConfig = [];
		var finalLights = [];

		//create atmoorb led config
		for(var key in lights)
		{
			if($('#orb_'+key).val() !== "disabled")
			{
				// Set Name to layout-position, if empty
				if ( lights[key].name === "" )
				{
					lights[key].name = $.i18n( 'conf_leds_layout_cl_'+$('#orb_'+key).val() );
				}

				finalLights.push( lights[key].id);

				var name = lights[key].id;
				if ( lights[key].host !== "")
					name += ':' + lights[key].host;

				var idx_content = assignLightPos(key, $('#orb_'+key).val(), name);
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
		d.useOrbSmoothing  = (eV("useOrbSmoothing") == true);

		d.output = conf_editor.getEditor("root.specificOptions.output").getValue();
		d.port = parseInt(conf_editor.getEditor("root.specificOptions.port").getValue());
		d.latchTime = parseInt(conf_editor.getEditor("root.specificOptions.latchTime").getValue());;

		window.serverConfig.device = d;

		requestWriteConfig(window.serverConfig, true);
		resetWizard();
	});

	$('#btn_wiz_abort').off().on('click', resetWizard);
}

function getIdInLights(id) {
	return lights.filter(
				function(lights) {
					return lights.id === id
				}
				);
}

async function discover_atmoorb_lights(multiCastGroup, multiCastPort){

  var light = {};

  if ( multiCastGroup === "" )
  	multiCastGroup = "239.255.255.250";

  if ( multiCastPort === "")
    multiCastPort = 49692;

  let params = { multiCastGroup : multiCastGroup, multiCastPort : multiCastPort};

  // Get discovered lights
  const res = await requestLedDeviceDiscovery ('atmoorb', params);

  // TODO: error case unhandled
  // res can be: false (timeout) or res.error (not found)
  if(res && !res.error){
    const r = res.info

	// Process devices returned by discovery
	for(const device of r.devices)
	{
	    if( device.id !== "")
	    {
		    if ( getIdInLights ( device.id ).length === 0 )
		    {
		      light = {};
		      light.id = device.id;
		      light.ip = device.ip;
		      light.host = device.hostname;
		      lights.push(light);
		    }
	    }
	}

    // Add additional items from configuration
    for(const keyConfig in configuredLights)
    {
      if ( configuredLights[keyConfig] !== "" && !isNaN(configuredLights[keyConfig]) ) 
      {
		  if ( getIdInLights ( configuredLights[keyConfig] ).length === 0 )
		  {
		  	light = {};
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

function assign_atmoorb_lights(){

	// If records are left for configuration
	if(Object.keys(lights).length > 0)
	{
		$('#wh_topcontainer').toggle(false);
		$('#orb_ids_t, #btn_wiz_save').toggle(true);

		var lightOptions = [
					"top", "topleft", "topright",
					"bottom", "bottomleft", "bottomright",
					"left", "lefttop", "leftmiddle", "leftbottom",
					"right", "righttop", "rightmiddle", "rightbottom",
					"entire"
				];

		lightOptions.unshift("disabled");

		$('.lidsb').html("");
		var pos = "";

		for(var lightid in lights)
		{
			var orbId = lights[lightid].id;
			var orbIp = lights[lightid].ip;
			var orbHostname = lights[lightid].host;

			if ( orbHostname === "" )
				orbHostname = $.i18n('edt_dev_spec_lights_itemtitle');

			var options = "";
			for(var opt in lightOptions)
			{
				var val = lightOptions[opt];
				var txt = (val !== 'entire' && val !== 'disabled') ? 'conf_leds_layout_cl_' : 'wiz_ids_';
				options+= '<option value="'+val+'"';
				if(pos === val) options+=' selected="selected"';
				options+= '>'+$.i18n(txt+val)+'</option>';
			}

			var enabled = 'enabled'
			if ( orbId < 1 || orbId > 255 )
			{
				enabled = 'disabled'
				options = '<option value=disabled>'+$.i18n('wiz_atmoorb_unsupported')+'</option>';
			}

			var lightAnnotation ="";
			if ( orbIp !== "" )
			{
		      lightAnnotation = ': '+orbIp+'<br>('+orbHostname+')';
			}

			$('.lidsb').append(createTableRow([orbId + lightAnnotation, '<select id="orb_'+lightid+'" '+enabled+' class="orb_sel_watch form-control">'
											   + options
											   + '</select>','<button class="btn btn-sm btn-primary" ' +enabled+ ' onClick=identify_atmoorb_device('+orbId+')>'
											   + $.i18n('wiz_identify_light',orbId)+'</button>']));
		}

		$('.orb_sel_watch').bind("change", function(){
			var cC = 0;
			for(var key in lights)
			{
				if($('#orb_'+key).val() !== "disabled")
				{
					cC++;
				}
			}
			if ( cC === 0)
				$('#btn_wiz_save').attr("disabled",true);
			else
				$('#btn_wiz_save').attr("disabled",false);
		});
		$('.orb_sel_watch').trigger('change');
	}
	else
	{
		var noLightsTxt = '<p style="font-weight:bold;color:red;">'+$.i18n('wiz_atmoorb_noLights')+'</p>';
		$('#wizp2_body').append(noLightsTxt);
	}
}

function identify_atmoorb_device(orbId){

	let params = { id : orbId };

	const res = requestLedDeviceIdentification ("atmoorb", params);
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info
	}
}

//****************************
// Wizard/Routines Nanoleaf
//****************************
async function discover_nanoleaf(){

	const res = await requestLedDeviceDiscovery ('nanoleaf');

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process devices returned by discovery
		console.log(r);

		if(r.devices.length == 0)
			$('#wiz_hue_ipstate').html($.i18n('wiz_hue_failure_ip'));
		else
		{
			for(const device of r.devices)
			{
				console.log("Device:", device);

				var ip = device.hostname + ":" + device.port;
				console.log("Host:", ip);

				//nanoleafIPs.push({internalipaddress : ip});
			}
		}
	}
}

async function getProperties_nanoleaf(hostAddress, authToken, resourceFilter){

	let params = { host: hostAddress, token: authToken, filter: resourceFilter};

	const res = await requestLedDeviceProperties ('nanoleaf', params);

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process properties returned
		console.log(r);
	}
}

function identify_nanoleaf(hostAddress, authToken){

	let params = { host: hostAddress, token: authToken};

	const res = requestLedDeviceIdentification ("nanoleaf", params);
	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){

		const r = res.info
		console.log(r);
	}
}

//****************************
// Wizard/Routines RS232-Devices
//****************************
async function discover_providerRs232(rs232Type){

	const res = await requestLedDeviceDiscovery (rs232Type);

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process serialPorts returned by discover
		console.log(r);
	}
}

//****************************
// Wizard/Routines HID (USB)-Devices
//****************************
async function discover_providerHid(hidType){

	const res = await requestLedDeviceDiscovery (hidType);
	console.log("discover_providerHid" ,res);	

	// TODO: error case unhandled
  	// res can be: false (timeout) or res.error (not found)
	if(res && !res.error){
		const r = res.info

		// Process HID returned by discover
		console.log(r);
	}
}

