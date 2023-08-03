var onLedLayoutTab = false;
var nonBlacklistLedArray = [];
var ledBlacklist = [];
var finalLedArray = [];
var conf_editor = null;
var blacklist_editor = null;
var aceEdt = null;
var imageCanvasNodeCtx;
var canvas_height;
var canvas_width;
var topLeftPoint = null;
var topRightPoint = null;
var bottomRightPoint = null;
var bottomLeftPoint = null;
var topLeft2topRight = null;
var topRight2bottomRight = null;
var bottomRight2bottomLeft = null;
var bottomLeft2topLeft = null;
var toggleKeystoneCorrectionArea = false;

var devRPiSPI = ['apa102', 'apa104', 'ws2801', 'lpd6803', 'lpd8806', 'p9813', 'sk6812spi', 'sk6822spi', 'sk9822', 'ws2812spi'];
var devRPiPWM = ['ws281x'];
var devRPiGPIO = ['piblaster'];
var devNET = ['atmoorb', 'cololight', 'fadecandy', 'philipshue', 'nanoleaf', 'razer', 'tinkerforge', 'tpm2net', 'udpe131', 'udpartnet', 'udpddp', 'udph801', 'udpraw', 'wled', 'yeelight'];
var devSerial = ['adalight', 'dmx', 'atmo', 'sedu', 'tpm2', 'karate'];
var devHID = ['hyperionusbasp', 'lightpack', 'paintpack', 'rawhid'];

var infoTextDefault = '<span>' + $.i18n("conf_leds_device_info_log") + ' </span><a href="" onclick="SwitchToMenuItem(\'MenuItemLogging\')" style="cursor:pointer">' + $.i18n("main_menu_logging_token") + '</a>';

var configPanel = "text";

function round(number) {
  var factor = Math.pow(10, 4);
  var tempNumber = number * factor;
  var roundedTempNumber = Math.round(tempNumber);
  return roundedTempNumber / factor;
};

function createLedPreview(leds) {
  if (configPanel == "classic") {
    $('#previewcreator').html($.i18n('conf_leds_layout_preview_originCL'));
    $('#leds_preview').css("padding-top", "56.25%");
  }
  else if (configPanel == "text") {
    $('#previewcreator').html($.i18n('conf_leds_layout_preview_originTEXT'));
    $('#leds_preview').css("padding-top", "56.25%");
  }
  else if (configPanel == "matrix") {
    $('#previewcreator').html($.i18n('conf_leds_layout_preview_originMA'));
    $('#leds_preview').css("padding-top", "100%");
  }

  $('#previewledcount').html($.i18n('conf_leds_layout_preview_totalleds', leds.length));
  $('#previewledpower').html($.i18n('conf_leds_layout_preview_ledpower', ((leds.length * 0.06) * 1.1).toFixed(1)));

  $('.st_helper').css("border", "8px solid grey");

  canvas_height = $('#leds_preview').innerHeight();
  canvas_width = $('#leds_preview').innerWidth();

  imageCanvasNodeCtx = document.getElementById("image_preview").getContext("2d");
  $('#image_preview').css({ "width": canvas_width, "height": canvas_height });

  var leds_html = "";
  for (var idx = leds.length - 1; idx >= 0; idx--) {
    var led = leds[idx];
    var led_id = 'ledc_' + [idx];
    var bgcolor = "background-color:hsla(" + (idx * 360 / leds.length) + ",100%,50%,0.75);";
    var pos = "left:" + (led.hmin * canvas_width) + "px;" +
      "top:" + (led.vmin * canvas_height) + "px;" +
      "width:" + ((led.hmax - led.hmin) * (canvas_width - 1)) + "px;" +
      "height:" + ((led.vmax - led.vmin) * (canvas_height - 1)) + "px;";
    leds_html += '<div id="' + led_id + '" class="led" style="' + bgcolor + pos + '" title="' + idx + '"><span id="' + led_id + '_num" class="led_prev_num">' + ((led.name) ? led.name : idx) + '</span></div>';
  }
  $('#leds_preview').html(leds_html);
  $('#ledc_0').css({ "background-color": "black", "z-index": "12" });
  $('#ledc_1').css({ "background-color": "grey", "z-index": "11" });
  $('#ledc_2').css({ "background-color": "#A9A9A9", "z-index": "10" });

  if ($('#leds_prev_toggle_num').hasClass('btn-success'))
    $('.led_prev_num').css("display", "inline");

  if (onLedLayoutTab && configPanel == "classic" && toggleKeystoneCorrectionArea) {
    // Calculate corner size (min/max:10px/18px)
    var size = Math.min(Math.max(canvas_width / 100 * 2, 10), 18);
    var corner_size = "width:" + size + "px; height:" + size + "px;";

    var corners =
      '<div id="top_left_point" class="keystone_correction_corners cursor_nwse" style="' + corner_size + '"></div>' +
      '<div id="top_right_point" class="keystone_correction_corners cursor_nesw" style="' + corner_size + '"></div>' +
      '<div id="bottom_right_point" class="keystone_correction_corners cursor_nwse" style="' + corner_size + '"></div>' +
      '<div id="bottom_left_point" class="keystone_correction_corners cursor_nesw" style="' + corner_size + '"></div>';
    $('#keystone_correction_area').html(corners).css({ "width": canvas_width, "height": canvas_height });

    var top_left_point = document.getElementById('top_left_point'),
      top_right_point = document.getElementById('top_right_point'),
      bottom_right_point = document.getElementById('bottom_right_point'),
      bottom_left_point = document.getElementById('bottom_left_point');

    var maxWidth = $('#keystone_correction_area').innerWidth(),
      maxHeight = $('#keystone_correction_area').innerHeight();

    // Deactivate build-in cursor
    PlainDraggable.draggableCursor = false;
    PlainDraggable.draggingCursor = false;

    // Top Left Point
    topLeftPoint = new PlainDraggable(top_left_point, {
      containment: {
        left: parseInt($('#keystone_correction_area').offset().left - size / 2),
        top: parseInt($('#keystone_correction_area').offset().top - size / 2),
        width: parseInt(maxWidth + $('#top_left_point').outerWidth()),
        height: parseInt(maxHeight + $('#top_left_point').outerHeight()),
      },
      onMove: function (newPosition) {
        var keystone_correction_area_offsets = $('#keystone_correction_area').offset();
        var left = newPosition.left - keystone_correction_area_offsets.left + size / 2;
        var top = newPosition.top - keystone_correction_area_offsets.top + size / 2;
        var ptlh = Math.min(Math.max((((left * 1) / maxWidth).toFixed(2) * 100).toFixed(0), 0), 100);
        var ptlv = Math.min(Math.max((((top * 1) / maxHeight).toFixed(2) * 100).toFixed(0), 0), 100);

        $('#ip_cl_ptlh').val(ptlh);
        $('#ip_cl_ptlv').val(ptlv);
        $("#ip_cl_ptlh, #ip_cl_ptlv").trigger("change");
      }
    });

    // Initialize position
    topLeftPoint.left = $('#keystone_correction_area').offset().left + maxWidth / 100 * $('#ip_cl_ptlh').val() - size / 2;
    topLeftPoint.top = $('#keystone_correction_area').offset().top + maxHeight / 100 * $('#ip_cl_ptlv').val() - size / 2;

    // Top right point
    topRightPoint = new PlainDraggable(top_right_point, {
      containment: {
        left: parseInt($('#keystone_correction_area').offset().left - $('#top_right_point').outerWidth() + size / 2),
        top: parseInt($('#keystone_correction_area').offset().top - size / 2),
        width: parseInt(maxWidth + $('#top_right_point').outerWidth()),
        height: parseInt(maxHeight + $('#top_right_point').outerHeight())
      },
      onMove: function (newPosition) {
        var keystone_correction_area_offsets = $('#keystone_correction_area').offset();
        var left = newPosition.left - keystone_correction_area_offsets.left + $('#top_right_point').outerWidth() - size / 2;
        var top = newPosition.top - keystone_correction_area_offsets.top + size / 2;
        var ptrh = Math.min(Math.max((((left * 1) / maxWidth).toFixed(2) * 100).toFixed(0), 0), 100);
        var ptrv = Math.min(Math.max((((top * 1) / maxHeight).toFixed(2) * 100).toFixed(0), 0), 100);

        $('#ip_cl_ptrh').val(ptrh);
        $('#ip_cl_ptrv').val(ptrv);
        $("#ip_cl_ptrh, #ip_cl_ptrv").trigger("change");
      }
    });

    // Initialize position
    topRightPoint.left = $('#keystone_correction_area').offset().left + maxWidth / 100 * $('#ip_cl_ptrh').val() - size / 2;
    topRightPoint.top = $('#keystone_correction_area').offset().top + maxHeight / 100 * $('#ip_cl_ptrv').val() - size / 2;

    // Bottom right point
    bottomRightPoint = new PlainDraggable(bottom_right_point, {
      containment: {
        left: parseInt($('#keystone_correction_area').offset().left - $('#bottom_right_point').outerWidth() + size / 2),
        top: parseInt($('#keystone_correction_area').offset().top - $('#bottom_right_point').outerHeight() + size / 2),
        width: parseInt(maxWidth + $('#bottom_right_point').outerWidth()),
        height: parseInt(maxHeight + $('#bottom_right_point').outerHeight())
      },
      onMove: function (newPosition) {
        var keystone_correction_area_offsets = $('#keystone_correction_area').offset();
        var left = newPosition.left - keystone_correction_area_offsets.left + $('#bottom_right_point').outerWidth() - size / 2;
        var top = newPosition.top - keystone_correction_area_offsets.top + $('#bottom_right_point').outerHeight() - size / 2;
        var pbrh = Math.min(Math.max((((left * 1) / maxWidth).toFixed(2) * 100).toFixed(0), 0), 100);
        var pbrv = Math.min(Math.max((((top * 1) / maxHeight).toFixed(2) * 100).toFixed(0), 0), 100);

        $('#ip_cl_pbrh').val(pbrh);
        $('#ip_cl_pbrv').val(pbrv);
        $("#ip_cl_pbrh, #ip_cl_pbrv").trigger("change");
      }
    });

    // Initialize position
    bottomRightPoint.left = $('#keystone_correction_area').offset().left + maxWidth / 100 * $('#ip_cl_pbrh').val() - size / 2;
    bottomRightPoint.top = $('#keystone_correction_area').offset().top + maxHeight / 100 * $('#ip_cl_pbrv').val() - size / 2;

    // Bottom left point
    bottomLeftPoint = new PlainDraggable(bottom_left_point, {
      containment: {
        left: parseInt($('#keystone_correction_area').offset().left - size / 2),
        top: parseInt($('#keystone_correction_area').offset().top - $('#bottom_left_point').outerHeight() + size / 2),
        width: parseInt(maxWidth + $('#bottom_left_point').outerWidth()),
        height: parseInt(maxHeight + $('#bottom_left_point').outerHeight())
      },
      onMove: function (newPosition) {
        var keystone_correction_area_offsets = $('#keystone_correction_area').offset();
        var left = newPosition.left - keystone_correction_area_offsets.left + size / 2;
        var top = newPosition.top - keystone_correction_area_offsets.top + $('#bottom_left_point').outerHeight() - size / 2;
        var pblh = Math.min(Math.max((((left * 1) / maxWidth).toFixed(2) * 100).toFixed(0), 0), 100);
        var pblv = Math.min(Math.max((((top * 1) / maxHeight).toFixed(2) * 100).toFixed(0), 0), 100);

        $('#ip_cl_pblh').val(pblh);
        $('#ip_cl_pblv').val(pblv);
        $("#ip_cl_pblh, #ip_cl_pblv").trigger("change");
      }
    });

    // Initialize position
    bottomLeftPoint.left = $('#keystone_correction_area').offset().left + maxWidth / 100 * $('#ip_cl_pblh').val() - size / 2;
    bottomLeftPoint.top = $('#keystone_correction_area').offset().top + maxHeight / 100 * $('#ip_cl_pblv').val() - size / 2;

    // Remove existing lines
    if (topLeft2topRight != null) {
      topLeft2topRight.remove();
    }

    if (topRight2bottomRight != null) {
      topRight2bottomRight.remove();
    }

    if (bottomRight2bottomLeft != null) {
      bottomRight2bottomLeft.remove();
    }

    if (bottomLeft2topLeft != null) {
      bottomLeft2topLeft.remove();
    }

    // Get border color from keystone correction corners
    var lineColor = $(".keystone_correction_corners").css("border-color");

    // Add lines
    topLeft2topRight = new LeaderLine(LeaderLine.pointAnchor(top_left_point, { x: '50%', y: '50%' }), LeaderLine.pointAnchor(top_right_point, { x: '50%', y: '50%' }), { path: 'straight', size: 1, color: lineColor, endPlug: 'behind' });
    topRight2bottomRight = new LeaderLine(LeaderLine.pointAnchor(top_right_point, { x: '50%', y: '50%' }), LeaderLine.pointAnchor(bottom_right_point, { x: '50%', y: '50%' }), { path: 'straight', size: 1, color: lineColor, endPlug: 'behind' });
    bottomRight2bottomLeft = new LeaderLine(LeaderLine.pointAnchor(bottom_right_point, { x: '50%', y: '50%' }), LeaderLine.pointAnchor(bottom_left_point, { x: '50%', y: '50%' }), { path: 'straight', size: 1, color: lineColor, endPlug: 'behind' });
    bottomLeft2topLeft = new LeaderLine(LeaderLine.pointAnchor(bottom_left_point, { x: '50%', y: '50%' }), LeaderLine.pointAnchor(top_left_point, { x: '50%', y: '50%' }), { path: 'straight', size: 1, color: lineColor, endPlug: 'behind' });
  } else {
    $('#keystone_correction_area').html("").css({ "width": 0, "height": 0 });

    // Remove existing lines
    if (topLeft2topRight != null) {
      topLeft2topRight.remove();
      topLeft2topRight = null;
    }

    if (topRight2bottomRight != null) {
      topRight2bottomRight.remove();
      topRight2bottomRight = null;
    }

    if (bottomRight2bottomLeft != null) {
      bottomRight2bottomLeft.remove();
      bottomRight2bottomLeft = null;
    }

    if (bottomLeft2topLeft != null) {
      bottomLeft2topLeft.remove();
      bottomLeft2topLeft = null;
    }
  }

  // Change on window resize. Is this correct?
  $(window).off("resize.createLedPreview");
  $(window).on("resize.createLedPreview", (function () {
    createLedPreview(leds);
  }));
}

function createClassicLedLayoutSimple(ledstop, ledsleft, ledsright, ledsbottom, position, reverse) {
  let params = {
    ledstop: 0, ledsleft: 0, ledsright: 0, ledsbottom: 0,
    ledsglength: 0, ledsgpos: 0, position: 0,
    ledsHDepth: 0.08, ledsVDepth: 0.05, overlap: 0,
    edgeVGap: 0,
    ptblh: 0, ptblv: 1, ptbrh: 1, ptbrv: 1,
    pttlh: 0, pttlv: 0, pttrh: 1, pttrv: 0,
    reverse: false
  };

  params.ledstop = ledstop;
  params.ledsleft = ledsleft;
  params.ledsright = ledsright;
  params.ledsbottom = ledsbottom;
  params.position = position;
  params.reverse = reverse;

  return createClassicLedLayout(params);
}

function createClassicLedLayout(params) {
  var edgeHGap = params.edgeVGap / (16 / 9);
  var ledArray = [];

  function createFinalArray(array) {
    var finalLedArray = [];
    for (var i = 0; i < array.length; i++) {
      var hmin = array[i].hmin;
      var hmax = array[i].hmax;
      var vmin = array[i].vmin;
      var vmax = array[i].vmax;
      finalLedArray[i] = { "hmax": hmax, "hmin": hmin, "vmax": vmax, "vmin": vmin }
    }
    return finalLedArray;
  }

  function rotateArray(array, times) {
    if (times > 0) {
      while (times--) {
        array.push(array.shift())
      }
      return array;
    }
    else {
      while (times++) {
        array.unshift(array.pop())
      }
      return array;
    }
  }

  function valScan(val) {
    if (val > 1)
      return 1;
    if (val < 0)
      return 0;
    return val;
  }

  function ovl(scan, val) {
    if (scan == "+")
      return valScan(val += params.overlap);
    else
      return valScan(val -= params.overlap);
  }

  function createLedArray(hmin, hmax, vmin, vmax) {
    hmin = round(hmin);
    hmax = round(hmax);
    vmin = round(vmin);
    vmax = round(vmax);
    ledArray.push({ "hmin": hmin, "hmax": hmax, "vmin": vmin, "vmax": vmax });
  }

  function createTopLeds() {
    var steph = (params.pttrh - params.pttlh - (2 * edgeHGap)) / params.ledstop;
    var stepv = (params.pttrv - params.pttlv) / params.ledstop;

    for (var i = 0; i < params.ledstop; i++) {
      var hmin = ovl("-", params.pttlh + (steph * Number([i])) + edgeHGap);
      var hmax = ovl("+", params.pttlh + (steph * Number([i + 1])) + edgeHGap);
      var vmin = params.pttlv + (stepv * Number([i]));
      var vmax = vmin + params.ledsHDepth;
      createLedArray(hmin, hmax, vmin, vmax);
    }
  }

  function createRightLeds() {
    var steph = (params.ptbrh - params.pttrh) / params.ledsright;
    var stepv = (params.ptbrv - params.pttrv - (2 * params.edgeVGap)) / params.ledsright;

    for (var i = 0; i < params.ledsright; i++) {
      var hmax = params.pttrh + (steph * Number([i + 1]));
      var hmin = hmax - params.ledsVDepth;
      var vmin = ovl("-", params.pttrv + (stepv * Number([i])) + params.edgeVGap);
      var vmax = ovl("+", params.pttrv + (stepv * Number([i + 1])) + params.edgeVGap);
      createLedArray(hmin, hmax, vmin, vmax);
    }
  }

  function createBottomLeds() {
    var steph = (params.ptbrh - params.ptblh - (2 * edgeHGap)) / params.ledsbottom;
    var stepv = (params.ptbrv - params.ptblv) / params.ledsbottom;

    for (var i = params.ledsbottom - 1; i > -1; i--) {
      var hmin = ovl("-", params.ptblh + (steph * Number([i])) + edgeHGap);
      var hmax = ovl("+", params.ptblh + (steph * Number([i + 1])) + edgeHGap);
      var vmax = params.ptblv + (stepv * Number([i]));
      var vmin = vmax - params.ledsHDepth;
      createLedArray(hmin, hmax, vmin, vmax);
    }
  }

  function createLeftLeds() {
    var steph = (params.ptblh - params.pttlh) / params.ledsleft;
    var stepv = (params.ptblv - params.pttlv - (2 * params.edgeVGap)) / params.ledsleft;

    for (var i = params.ledsleft - 1; i > -1; i--) {
      var hmin = params.pttlh + (steph * Number([i]));
      var hmax = hmin + params.ledsVDepth;
      var vmin = ovl("-", params.pttlv + (stepv * Number([i])) + params.edgeVGap);
      var vmax = ovl("+", params.pttlv + (stepv * Number([i + 1])) + params.edgeVGap);
      createLedArray(hmin, hmax, vmin, vmax);
    }
  }

  //rectangle
  createTopLeds();
  createRightLeds();
  createBottomLeds();
  createLeftLeds();

  //check led gap pos
  if (params.ledsgpos + params.ledsglength > ledArray.length) {
    var mpos = Math.max(0, ledArray.length - params.ledsglength);
    //$('#ip_cl_ledsgpos').val(mpos);
    ledsgpos = mpos;
  }

  //check led gap length
  if (params.ledsglength >= ledArray.length) {
    //$('#ip_cl_ledsglength').val(ledArray.length-1);
    params.ledsglength = ledArray.length - params.ledsglength - 1;
  }

  if (params.ledsglength != 0) {
    ledArray.splice(params.ledsgpos, params.ledsglength);
  }

  if (params.position != 0) {
    rotateArray(ledArray, params.position);
  }

  if (params.reverse)
    ledArray.reverse();

  return createFinalArray(ledArray);
}

function createClassicLeds() {
  //get values
  let params = {
    ledstop: parseInt($("#ip_cl_top").val()),
    ledsbottom: parseInt($("#ip_cl_bottom").val()),
    ledsleft: parseInt($("#ip_cl_left").val()),
    ledsright: parseInt($("#ip_cl_right").val()),
    ledsglength: parseInt($("#ip_cl_glength").val()),
    ledsgpos: parseInt($("#ip_cl_gpos").val()),
    position: parseInt($("#ip_cl_position").val()),
    reverse: $("#ip_cl_reverse").is(":checked"),

    //advanced values
    ledsVDepth: parseInt($("#ip_cl_vdepth").val()) / 100,
    ledsHDepth: parseInt($("#ip_cl_hdepth").val()) / 100,
    edgeVGap: parseInt($("#ip_cl_edgegap").val()) / 100 / 2,
    //cornerVGap : parseInt($("#ip_cl_cornergap").val())/100/2,
    overlap: $("#ip_cl_overlap").val() / 100,

    //trapezoid values % -> float
    ptblh: parseInt($("#ip_cl_pblh").val()) / 100,
    ptblv: parseInt($("#ip_cl_pblv").val()) / 100,
    ptbrh: parseInt($("#ip_cl_pbrh").val()) / 100,
    ptbrv: parseInt($("#ip_cl_pbrv").val()) / 100,
    pttlh: parseInt($("#ip_cl_ptlh").val()) / 100,
    pttlv: parseInt($("#ip_cl_ptlv").val()) / 100,
    pttrh: parseInt($("#ip_cl_ptrh").val()) / 100,
    pttrv: parseInt($("#ip_cl_ptrv").val()) / 100,
  }

  nonBlacklistLedArray = createClassicLedLayout(params);
  finalLedArray = blackListLeds(nonBlacklistLedArray, ledBlacklist);

  //check led gap pos
  if (params.ledsgpos + params.ledsglength > finalLedArray.length) {
    var mpos = Math.max(0, finalLedArray.length - params.ledsglength);
    $('#ip_cl_ledsgpos').val(mpos);
  }
  //check led gap length
  if (params.ledsglength >= finalLedArray.length) {
    $('#ip_cl_ledsglength').val(finalLedArray.length - 1);
  }

  createLedPreview(finalLedArray);
  aceEdt.set(finalLedArray);
}

function createMatrixLayout(ledshoriz, ledsvert, cabling, start, direction) {
  // Big thank you to RanzQ (Juha Rantanen) from Github for this script
  // https://raw.githubusercontent.com/RanzQ/hyperion-audio-effects/master/matrix-config.js

  var parallel = false
  var leds = []
  var hblock = 1.0 / ledshoriz
  var vblock = 1.0 / ledsvert

  if (cabling == "parallel") {
    parallel = true
  }

  /**
   * Adds led to the hyperion config led array
   * @param {Number} x     Horizontal position in matrix
   * @param {Number} y     Vertical position in matrix
   */
  function addLed(x, y) {
    var hscanMin = x * hblock
    var hscanMax = (x + 1) * hblock
    var vscanMin = y * vblock
    var vscanMax = (y + 1) * vblock

    hscanMin = round(hscanMin);
    hscanMax = round(hscanMax);
    vscanMin = round(vscanMin);
    vscanMax = round(vscanMax);

    leds.push({
      hmin: hscanMin,
      hmax: hscanMax,
      vmin: vscanMin,
      vmax: vscanMax
    })
  }

  var startYX = start.split('-')
  var startX = startYX[1] === 'right' ? ledshoriz - 1 : 0
  var startY = startYX[0] === 'bottom' ? ledsvert - 1 : 0
  var endX = startX === 0 ? ledshoriz - 1 : 0
  var endY = startY === 0 ? ledsvert - 1 : 0
  var forward = startX < endX

  var downward = startY < endY

  var x, y

  if (direction === 'vertical') {
    for (x = startX; forward && x <= endX || !forward && x >= endX; x += forward ? 1 : -1) {
      for (y = startY; downward && y <= endY || !downward && y >= endY; y += downward ? 1 : -1) {

        addLed(x, y)
      }
      if (!parallel) {
        downward = !downward
        var tmp = startY
        startY = endY
        endY = tmp
      }
    }
  } else {
    for (y = startY; downward && y <= endY || !downward && y >= endY; y += downward ? 1 : -1) {
      for (x = startX; forward && x <= endX || !forward && x >= endX; x += forward ? 1 : -1) {
        addLed(x, y)
      }
      if (!parallel) {
        forward = !forward
        var tmp = startX
        startX = endX
        endX = tmp
      }
    }
  }

  return leds;
}

function createMatrixLeds() {
  // Big thank you to RanzQ (Juha Rantanen) from Github for this script
  // https://raw.githubusercontent.com/RanzQ/hyperion-audio-effects/master/matrix-config.js

  //get values
  var ledshoriz = parseInt($("#ip_ma_ledshoriz").val());
  var ledsvert = parseInt($("#ip_ma_ledsvert").val());
  var cabling = $("#ip_ma_cabling").val();
  var direction = $("#ip_ma_direction").val();
  var start = $("#ip_ma_start").val();

  nonBlacklistLedArray = createMatrixLayout(ledshoriz, ledsvert, cabling, start, direction);
  finalLedArray = blackListLeds(nonBlacklistLedArray, ledBlacklist);

  createLedPreview(finalLedArray);
  aceEdt.set(finalLedArray);
}

function blackListLeds(nonBlacklistLedArray, blackList) {

  var blacklistedLedArray = [...nonBlacklistLedArray];
  if (blackList && blackList.length > 0) {

    for (let item of blackList) {
      var start = item.start;
      var num = item.num
      var layoutSize = blacklistedLedArray.length;

      //Only consider rules which are in rage of defined number of LEDs
      if (start >= 0 && start < layoutSize) {
        // If number of LEDs exceeds layoutSize, use apply number until layout size
        if (start + num > layoutSize) {
          num = layoutSize - start;

        }
        for (var i = 0; i < num; i++) {
          blacklistedLedArray[start + i] = { hmax: 0, hmin: 0, vmax: 0, vmin: 0 };
        }
      }
    }
  }

  return blacklistedLedArray;
}

function getLedConfig() {

  var ledConfig = { classic: {}, matrix: {} };

  var classicSchema = window.serverSchema.properties.ledConfig.properties.classic.properties;
  for (var key in classicSchema) {
    if (classicSchema[key].type === "boolean")
      ledConfig.classic[key] = $('#ip_cl_' + key).is(':checked');
    else if (classicSchema[key].type === "integer")
      ledConfig.classic[key] = parseInt($('#ip_cl_' + key).val());
    else
      ledConfig.classic[key] = $('#ip_cl_' + key).val();
  }

  var matrixSchema = window.serverSchema.properties.ledConfig.properties.matrix.properties;
  for (var key in matrixSchema) {
    if (matrixSchema[key].type === "boolean")
      ledConfig.matrix[key] = $('#ip_ma_' + key).is(':checked');
    else if (matrixSchema[key].type === "integer")
      ledConfig.matrix[key] = parseInt($('#ip_ma_' + key).val());
    else
      ledConfig.matrix[key] = $('#ip_ma_' + key).val();
  }

  ledConfig.ledBlacklist = blacklist_editor.getEditor("root.ledBlacklist").getValue();

  return ledConfig;
}

function isEmpty(obj) {
  for (var key in obj) {
    if (obj.hasOwnProperty(key))
      return false;
  }
  return true;
}

$(document).ready(function () {
  // translate
  performTranslation();

  // update instance listing
  updateHyperionInstanceListing();

  //add intros
  if (window.showOptHelp) {
    createHintH("intro", $.i18n('conf_leds_device_intro'), "leddevice_intro");
    createHintH("intro", $.i18n('conf_leds_layout_intro'), "layout_intro");
    $('#led_vis_help').html('<div><div class="led_ex" style="background-color:black;margin-right:5px;margin-top:3px"></div><div style="display:inline-block;vertical-align:top">' + $.i18n('conf_leds_layout_preview_l1') + '</div></div><div class="led_ex" style="background-color:grey;margin-top:3px;margin-right:2px"></div><div class="led_ex" style="background-color: rgb(169, 169, 169);margin-right:5px;margin-top:3px;"></div><div style="display:inline-block;vertical-align:top">' + $.i18n('conf_leds_layout_preview_l2') + '</div>');
  }

  //**************************************************
  // Handle LED-Layout Configuration
  //**************************************************
  var slConfig = window.serverConfig.ledConfig;

  //restore ledConfig - Classic
  for (var key in slConfig.classic) {
    if (typeof (slConfig.classic[key]) === "boolean")
      $('#ip_cl_' + key).prop('checked', slConfig.classic[key]);
    else
      $('#ip_cl_' + key).val(slConfig.classic[key]);
  }

  //restore ledConfig - Matrix
  for (var key in slConfig.matrix) {
    if (typeof (slConfig.matrix[key]) === "boolean")
      $('#ip_ma_' + key).prop('checked', slConfig.matrix[key]);
    else
      $('#ip_ma_' + key).val(slConfig.matrix[key]);
  }

  // check access level and adjust ui
  if (storedAccess == "default") {
    $('#texfield_panel').toggle(false);
    $('#previewcreator').toggle(false);
  }

  // Wiki link
  $('#leds_wl').append('<p style="font-weight:bold">' + $.i18n('general_wiki_moreto', $.i18n('conf_leds_nav_label_ledlayout')) + buildWL("user/advanced/Advanced.html#led-layout", "Wiki") + '</p>');

  // bind change event to all inputs
  $('.ledCLconstr').on("change", function () {

    //Ensure Values are in min/max ranges
    if ($(this).val() < $(this).attr('min') * 1) { $(this).val($(this).attr('min')); }
    if ($(this).val() > $(this).attr('max') * 1) { $(this).val($(this).attr('max')); }

    //top/bottom and left/right must not overlap
    switch (this.id) {
      case "ip_cl_ptlh":
        var ptrh = parseInt($("#ip_cl_ptrh").val());
        if (this.value > ptrh) {
          $(this).val(ptrh);
        }
        var pbrh = parseInt($("#ip_cl_pbrh").val());
        if (this.value > pbrh) {
          $(this).val(pbrh);
        }
        break;
      case "ip_cl_ptrh":
        var ptlh = parseInt($("#ip_cl_ptlh").val());
        if (this.value < ptlh) {
          $(this).val(ptlh);
        }
        var pblh = parseInt($("#ip_cl_pblh").val());
        if (this.value < pblh) {
          $(this).val(pblh);
        }
        break;
      case "ip_cl_pblh":
        var pbrh = parseInt($("#ip_cl_pbrh").val());
        if (this.value > pbrh) {
          $(this).val(pbrh);
        }
        var ptrh = parseInt($("#ip_cl_ptrh").val());
        if (this.value > ptrh) {
          $(this).val(ptrh);
        }

        break;
      case "ip_cl_pbrh":
        var pblh = parseInt($("#ip_cl_pblh").val());
        if (this.value < pblh) {
          $(this).val(pblh);
        }
        var ptlh = parseInt($("#ip_cl_ptlh").val());
        if (this.value < ptlh) {
          $(this).val(ptlh);
        }
        break;
      case "ip_cl_ptlv":
        var pblv = parseInt($("#ip_cl_pblv").val());
        if (this.value > pblv) {
          $(this).val(pblv);
        }
        var pbrv = parseInt($("#ip_cl_pbrv").val());
        if (this.value > pbrv) {
          $(this).val(pbrv);
        }
        break;
      case "ip_cl_pblv":
        var ptrv = parseInt($("#ip_cl_ptrv").val());
        if (this.value < ptrv) {
          $(this).val(ptrv);
        }
        var ptlv = parseInt($("#ip_cl_ptlv").val());
        if (this.value < ptlv) {
          $(this).val(ptlv);
        }
        break;
      case "ip_cl_ptrv":
        var pbrv = parseInt($("#ip_cl_pbrv").val());
        if (this.value > pbrv) {
          $(this).val(pbrv);
        }
        var pblv = parseInt($("#ip_cl_pblv").val());
        if (this.value > pblv) {
          $(this).val(pblv);
        }
        break;
      case "ip_cl_pbrv":
        var ptrv = parseInt($("#ip_cl_ptrv").val());
        if (this.value < ptrv) {
          $(this).val(ptrv);
        }
        var ptlv = parseInt($("#ip_cl_ptlv").val());
        if (this.value < ptlv) {
          $(this).val(ptlv);
        }
        break;

      case "ip_cl_top":
      case "ip_cl_bottom":
      case "ip_cl_left":
      case "ip_cl_right":
      case "ip_cl_glength":
      case "ip_cl_gpos":
        var ledstop = parseInt($("#ip_cl_top").val());
        var ledsbottom = parseInt($("#ip_cl_bottom").val());
        var ledsleft = parseInt($("#ip_cl_left").val());
        var ledsright = parseInt($("#ip_cl_right").val());
        var maxLEDs = ledstop + ledsbottom + ledsleft + ledsright;

        var gpos = parseInt($("#ip_cl_gpos").val());
        $("#ip_cl_gpos").attr({ 'max': maxLEDs - 1 });

        var max = maxLEDs - gpos;
        if (gpos == 0) {
          --max;
        }
        $("#ip_cl_glength").attr({ 'max': max });

        var glength = parseInt($("#ip_cl_glength").val());
        if (glength + gpos >= maxLEDs) {
          $("#ip_cl_glength").val($("#ip_cl_glength").attr('max'));
        }
        break;

      default:
    }
    createClassicLeds();
  });

  $('.ledMAconstr').on("change", function () {
    valValue(this.id, this.value, this.min, this.max);
    createMatrixLeds();
  });

  $('#collapse1').on('shown.bs.collapse', function () {
    configPanel = "classic";
    $("#leds_prev_toggle_keystone_correction_area").show();
    createClassicLeds();
  });

  $('#collapse2').on('shown.bs.collapse', function () {
    configPanel = "matrix";
    $("#leds_prev_toggle_keystone_correction_area").hide();
    createMatrixLeds();
  });

  $('#collapse5').on('shown.bs.collapse', function () {
    configPanel = "text";
    $("#leds_prev_toggle_keystone_correction_area").hide();
    createLedPreview(finalLedArray);
    aceEdt.set(finalLedArray);
  });

  // Initialise from config and apply blacklist rules
  nonBlacklistLedArray = window.serverConfig.leds;
  ledBlacklist = window.serverConfig.ledConfig.ledBlacklist;
  finalLedArray = blackListLeds(nonBlacklistLedArray, ledBlacklist);

  var blacklistOptions = window.serverSchema.properties.ledConfig.properties.ledBlacklist;
  blacklist_editor = createJsonEditor('editor_container_blacklist_conf', {
    ledBlacklist: blacklistOptions,
  });
  blacklist_editor.getEditor("root.ledBlacklist").setValue(ledBlacklist);

  // v4 of json schema with diff required assignment - remove when hyperion schema moved to v4
  var ledschema = { "items": { "additionalProperties": false, "required": ["hmin", "hmax", "vmin", "vmax"], "properties": { "name": { "type": "string" }, "colorOrder": { "enum": ["rgb", "bgr", "rbg", "brg", "gbr", "grb"], "type": "string" }, "hmin": { "maximum": 1, "minimum": 0, "type": "number" }, "hmax": { "maximum": 1, "minimum": 0, "type": "number" }, "vmin": { "maximum": 1, "minimum": 0, "type": "number" }, "vmax": { "maximum": 1, "minimum": 0, "type": "number" } }, "type": "object" }, "type": "array" };
  //create jsonace editor
  aceEdt = new JSONACEEditor(document.getElementById("aceedit"), {
    mode: 'code',
    schema: ledschema,
    onChange: function () {
      var success = true;
      try {
        aceEdt.get();
      }
      catch (err) {
        success = false;
      }

      if (success) {
        $('#leds_custom_updsim').prop("disabled", false);
        $('#leds_custom_save').prop("disabled", false);
      }
      else {
        $('#leds_custom_updsim').prop("disabled", true);
        $('#leds_custom_save').prop("disabled", true);
      }

      if (window.readOnlyMode) {
        $('#leds_custom_save').prop('disabled', true);
      }
    }
  }, finalLedArray);

  //TODO: HACK! No callback for schema validation - Add it!
  setInterval(function () {
    if ($('#aceedit table').hasClass('jsoneditor-text-errors')) {
      $('#leds_custom_updsim').prop("disabled", true);
      $('#leds_custom_save').prop("disabled", true);
    }
  }, 1000);

  $('.jsoneditor-menu').toggle();

  // validate textfield and update preview
  $("#leds_custom_updsim").off().on("click", function () {
    nonBlacklistLedArray = aceEdt.get();
    finalLedArray = blackListLeds(nonBlacklistLedArray, ledBlacklist);
    createLedPreview(finalLedArray);
  });

  // save led layout, the generated textfield configuration always represents the latest layout
  $("#btn_ma_save, #btn_cl_save, #btn_bl_save, #leds_custom_save").off().on("click", function () {
    var hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
    var layoutLedCount = aceEdt.get().length;

    if (hardwareLedCount < layoutLedCount) {
      // Not enough hardware LEDs for configured layout
      showInfoDialog('error', $.i18n("conf_leds_config_error"), $.i18n('conf_leds_error_hwled_lt_layout', hardwareLedCount, layoutLedCount));
    } else {
      saveLedConfig(false);
    }
  });

  // toggle right icon on "Advanced Settings" click
  $('#advanced_settings').on('click', function (e) {
    $('#advanced_settings_right_icon').toggleClass('fa-angle-down fa-angle-up');
  });

  // toggle fullscreen button in led preview
  $(".fullscreen-btn").mousedown(function (e) {
    e.preventDefault();
  });

  $(".fullscreen-btn").click(function (e) {
    e.preventDefault();
    $(this).children('i')
      .toggleClass('fa-expand')
      .toggleClass('fa-compress');
    $('#layout_type').toggle();
    $('#layout_preview').toggleClass('col-lg-6 col-lg-12');
    window.dispatchEvent(new Event('resize'));
  });

  // toggle led numbers
  $('#leds_prev_toggle_num').off().on("click", function () {
    $('.led_prev_num').toggle();
    toggleClass('#leds_prev_toggle_num', "btn-danger", "btn-success");
  });

  // toggle live video
  $('#leds_prev_toggle_live_video').off().on("click", function () {
    setClassByBool('#leds_prev_toggle_live_video', window.imageStreamActive, "btn-success", "btn-danger");

    if (onLedLayoutTab && window.imageStreamActive) {
      imageCanvasNodeCtx.clear();
      if (!$('#leds_toggle_live_video').hasClass("btn-success")) {
        requestLedImageStop();
      }
    }
    else {
      requestLedImageStart();
    }
  });

  // toggle keystone correction area
  $('#leds_prev_toggle_keystone_correction_area').off().on("click", function () {
    toggleKeystoneCorrectionArea = !toggleKeystoneCorrectionArea
    toggleClass('#leds_prev_toggle_keystone_correction_area', "btn-success", "btn-danger");
    window.dispatchEvent(new Event('resize'));
  });

  $(window.hyperion).on("cmd-ledcolors-imagestream-update", function (event) {
    //Only update Image, if LED Layout Tab is visible
    if (onLedLayoutTab && window.imageStreamActive) {
      setClassByBool('#leds_prev_toggle_live_video', window.imageStreamActive, "btn-danger", "btn-success");
      var imageData = (event.response.result.image);

      var image = new Image();
      image.onload = function () {
        imageCanvasNodeCtx.drawImage(image, 0, 0, imageCanvasNodeCtx.canvas.width, imageCanvasNodeCtx.canvas.height);
      };
      image.src = imageData;
    }
  });

  // open checklist
  $('#leds_prev_checklist').off().on("click", function () {
    var liList = [$.i18n('conf_leds_layout_checkp1'), $.i18n('conf_leds_layout_checkp3'), $.i18n('conf_leds_layout_checkp2'), $.i18n('conf_leds_layout_checkp4')];
    var ul = document.createElement("ul");
    ul.className = "checklist"

    for (var i = 0; i < liList.length; i++) {
      var li = document.createElement("li");
      li.innerHTML = liList[i];
      ul.appendChild(li);
    }
    showInfoDialog('checklist', "", ul);
  });

  // nav
  $('#leds_cfg_nav a[data-toggle="tab"]').off().on('shown.bs.tab', function (e) {
    var target = $(e.target).attr("href") // activated tab
    if (target == "#menu_gencfg") {
      onLedLayoutTab = true;
      $('#leds_custom_updsim').trigger('click');
    } else {
      onLedLayoutTab = false;
      window.dispatchEvent(new Event('resize')); // remove keystone correction lines
    }

    blacklist_editor.on('change', function () {
      // only update preview, if config is valid
      if (blacklist_editor.validate().length <= 0) {

        ledBlacklist = blacklist_editor.getEditor("root.ledBlacklist").getValue();
        finalLedArray = blackListLeds(nonBlacklistLedArray, ledBlacklist);
        createLedPreview(finalLedArray);
        aceEdt.set(finalLedArray);
      }

      // change save button state based on validation result
      blacklist_editor.validate().length || window.readOnlyMode ? $('#btn_bl_save').prop('disabled', true) : $('#btn_bl_save').prop('disabled', false);
    });

  });

  //**************************************************
  // Handle LED-Device Configuration
  //**************************************************

  // External properties properties, 2-dimensional arry of [ledType][key]
  devicesProperties = {};

  addJsonEditorHostValidation();

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    var errors = [];

    if (path === "root.specificOptions.segments.segmentList") {
      var overlapSegNames = validateWledSegmentConfig(value);
      if (overlapSegNames.length > 0) {
        errors.push({
          path: "root.specificOptions.segments",
          message: $.i18n('edt_dev_spec_segmentsOverlapValidation_error', overlapSegNames.length, overlapSegNames.join(', '))
        });
      }
    }
    return errors;
  });

  $("#leddevices").off().on("change", function () {
    var generalOptions = window.serverSchema.properties.device;

    var ledType = $(this).val();

    // philipshueentertainment backward fix
    if (ledType == "philipshueentertainment")
      ledType = "philipshue";

    var specificOptions = window.serverSchema.properties.alldevices[ledType];

    conf_editor = createJsonEditor('editor_container_leddevice', {
      specificOptions: specificOptions,
      generalOptions: generalOptions,
    });

    var values_general = {};
    var values_specific = {};
    var isCurrentDevice = (window.serverConfig.device.type == ledType);

    for (var key in window.serverConfig.device) {
      if (key != "type" && key in generalOptions.properties) values_general[key] = window.serverConfig.device[key];
    };
    conf_editor.getEditor("root.generalOptions").setValue(values_general);

    if (isCurrentDevice) {
      var specificOptions_val = conf_editor.getEditor("root.specificOptions").getValue();
      for (var key in specificOptions_val) {
        values_specific[key] = (key in window.serverConfig.device) ? window.serverConfig.device[key] : specificOptions_val[key];
      };
      conf_editor.getEditor("root.specificOptions").setValue(values_specific);
    };

    $("#info_container_text").html(infoTextDefault);

    // change save button state based on validation result
    conf_editor.validate().length || window.readOnlyMode ? $('#btn_submit_controller').prop('disabled', true) : $('#btn_submit_controller').prop('disabled', false);

    // led controller sepecific wizards
    $('#btn_wiz_holder').html("");
    $('#btn_led_device_wiz').off();

    if (ledType == "philipshue") {
      $('#root_specificOptions_useEntertainmentAPI').on("change", function () {
        var ledWizardType = (this.checked) ? "philipshueentertainment" : ledType;
        var data = { type: ledWizardType };
        var hue_title = (this.checked) ? 'wiz_hue_e_title' : 'wiz_hue_title';
        changeWizard(data, hue_title, startWizardPhilipsHue);
      });
      $("#root_specificOptions_useEntertainmentAPI").trigger("change");
    }
    else if (ledType == "atmoorb") {
      var ledWizardType = (this.checked) ? "atmoorb" : ledType;
      var data = { type: ledWizardType };
      var atmoorb_title = 'wiz_atmoorb_title';
      changeWizard(data, atmoorb_title, startWizardAtmoOrb);
    }
    else if (ledType == "yeelight") {
      var ledWizardType = (this.checked) ? "yeelight" : ledType;
      var data = { type: ledWizardType };
      var yeelight_title = 'wiz_yeelight_title';
      changeWizard(data, yeelight_title, startWizardYeelight);
    }

    function changeWizard(data, hint, fn) {
      $('#btn_wiz_holder').html("")
      createHint("wizard", $.i18n(hint), "btn_wiz_holder", "btn_led_device_wiz");
      $('#btn_led_device_wiz').off().on('click', data, fn);
    }

    conf_editor.on('ready', function () {
      var hwLedCountDefault = 1;
      var colorOrderDefault = "rgb";
      var filter = {};

      $('#btn_test_controller').hide();

      switch (ledType) {
        case "wled":
        case "cololight":
        case "nanoleaf":
          showAllDeviceInputOptions("hostList", false);
        case "apa102":
        case "apa104":
        case "ws2801":
        case "lpd6803":
        case "lpd8806":
        case "p9813":
        case "sk6812spi":
        case "sk6822spi":
        case "sk9822":
        case "ws2812spi":
        case "piblaster":
        case "ws281x":

        //Serial devices
        case "adalight":
        case "atmo":
        case "dmx":
        case "karate":
        case "sedu":
        case "tpm2":
          if (storedAccess === 'expert') {
            filter.discoverAll = true;
          }

          $('#btn_submit_controller').prop('disabled', true);

          discover_device(ledType, filter)
            .then(discoveryResult => {
              updateOutputSelectList(ledType, discoveryResult);
            })
            .then(discoveryResult => {
              if (ledType === "wled") {
                updateElementsWled(ledType);
              }
            })
            .catch(error => {
              showNotification('danger', "Device discovery for " + ledType + " failed with error:" + error);
            });

          hwLedCountDefault = 1;
          colorOrderDefault = "rgb";
          break;

        case "philipshue":
          disableAutoResolvedGeneralOptions();

          var lights = conf_editor.getEditor("root.specificOptions.lightIds").getValue();
          hwLedCountDefault = lights.length;
          colorOrderDefault = "rgb";
          break;

        case "yeelight":
          disableAutoResolvedGeneralOptions();

          var lights = conf_editor.getEditor("root.specificOptions.lights").getValue();
          hwLedCountDefault = lights.length;
          colorOrderDefault = "rgb";
          break;

        case "atmoorb":
          disableAutoResolvedGeneralOptions();

          var configruedOrbIds = conf_editor.getEditor("root.specificOptions.orbIds").getValue().trim();
          if (configruedOrbIds.length !== 0) {
            hwLedCountDefault = configruedOrbIds.split(",").map(Number).length;
          } else {
            hwLedCountDefault = 0;
          }
          colorOrderDefault = "rgb";
          break;

        case "razer":
          disableAutoResolvedGeneralOptions();
          hwLedCountDefault = 1;
          colorOrderDefault = "bgr";

          var subType = conf_editor.getEditor("root.specificOptions.subType").getValue();
          let params = { subType: subType };
          getProperties_device(ledType, subType, params);
          break;

        default:
      }

      if (ledType !== window.serverConfig.device.type) {
        var hwLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount");
        if (hwLedCount) {
          hwLedCount.setValue(hwLedCountDefault);
        }
        var colorOrder = conf_editor.getEditor("root.generalOptions.colorOrder");
        if (colorOrder) {
          colorOrder.setValue(colorOrderDefault);
        }
      }
    });

    conf_editor.on('change', function () {
      // //Check, if device can be identified/tested and/or saved
      var canIdentify = false;
      var canSave = false;

      switch (ledType) {

        case "atmoorb":
        case "fadecandy":
        case "tinkerforge":
        case "tpm2net":
        case "udpe131":
        case "udpartnet":
        case "udpddp":
        case "udph801":
        case "udpraw":
          var host = conf_editor.getEditor("root.specificOptions.host").getValue();
          if (host !== "") {
            canSave = true;
          }
          break;

        case "adalight":
        case "atmo":
        case "karate":
        case "dmx":
        case "sedu":
        case "tpm2":
          var rate = conf_editor.getEditor("root.specificOptions.rate").getValue();
          if (rate > 0) {
            canSave = true;
          }
          break;

        case "philipshue":
          var host = conf_editor.getEditor("root.specificOptions.host").getValue();
          var username = conf_editor.getEditor("root.specificOptions.username").getValue();
          if (host !== "" && username != "") {
            var useEntertainmentAPI = conf_editor.getEditor("root.specificOptions.useEntertainmentAPI").getValue();
            var clientkey = conf_editor.getEditor("root.specificOptions.clientkey").getValue();
            if (!useEntertainmentAPI || clientkey !== "") {
              canSave = true;
            }
          }
          break;

        case "wled":
        case "cololight":
          var hostList = conf_editor.getEditor("root.specificOptions.hostList").getValue();
          if (hostList !== "SELECT") {
            var host = conf_editor.getEditor("root.specificOptions.host").getValue();
            if (host !== "") {
              canIdentify = true;
              canSave = true;
            }
          }
          break;

        case "nanoleaf":
          var hostList = conf_editor.getEditor("root.specificOptions.hostList").getValue();
          if (hostList !== "SELECT") {
            var host = conf_editor.getEditor("root.specificOptions.host").getValue();
            var token = conf_editor.getEditor("root.specificOptions.token").getValue();
            if (host !== "" && token !== "") {
              canIdentify = true;
              canSave = true;
            }
          }
          break;
        default:
          canIdentify = false;
          canSave = true;
      }

      if (!conf_editor.validate().length) {
        if (canIdentify) {
          $("#btn_test_controller").show();
          $('#btn_test_controller').prop('disabled', false);
        } else {
          $('#btn_test_controller').hide();
          $('#btn_test_controller').prop('disabled', true);
        }
      } else {
        canSave = false;
      }

      if (canSave) {
        if (!window.readOnlyMode) {
          $('#btn_submit_controller').prop('disabled', false);
        }
      }
      else {
        $('#btn_submit_controller').prop('disabled', true);
      }

      window.readOnlyMode ? $('#btn_cl_save').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
      window.readOnlyMode ? $('#btn_ma_save').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
      window.readOnlyMode ? $('#leds_custom_save').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
    });

    conf_editor.watch('root.specificOptions.hostList', () => {
      var specOptPath = 'root.specificOptions.';

      //Disable General Options, as LED count will be resolved from device itself
      disableAutoResolvedGeneralOptions();

      var hostList = conf_editor.getEditor("root.specificOptions.hostList");
      if (hostList) {
        var val = hostList.getValue();
        var host = conf_editor.getEditor("root.specificOptions.host");
        var showOptions = true;

        switch (val) {
          case 'CUSTOM':
          case '':
            host.enable();
            //Populate existing host for current custom config
            if (ledType === window.serverConfig.device.type) {
              host.setValue(window.serverConfig.device.host);
            } else {
              host.setValue("");
            }
            break;
          case 'NONE':
            host.enable();
            //Trigger getProperties via host value
            conf_editor.notifyWatchers(specOptPath + "host");
            break;
          case 'SELECT':
            host.setValue("");
            host.disable();
            showOptions = false;
            break;
          default:
            host.disable();
            host.setValue(val);
            //Trigger getProperties via host value
            conf_editor.notifyWatchers(specOptPath + "host");
            break;
        }

        showAllDeviceInputOptions("hostList", showOptions);

        if (!host.isEnabled() && host.getValue().endsWith("._tcp.local")) {
          showInputOptionForItem(conf_editor, 'specificOptions', 'host', false);
        }
      }
    });

    conf_editor.watch('root.specificOptions.host', () => {
      var host = conf_editor.getEditor("root.specificOptions.host").getValue();

      if (host === "") {
        conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(1);
      }
      else {
        let params = {};
        switch (ledType) {

          case "cololight":
            params = { host: host };
            getProperties_device(ledType, host, params);
            break;

          case "nanoleaf":
            var token = conf_editor.getEditor("root.specificOptions.token").getValue();
            if (token === "") {
              return;
            }
            params = { host: host, token: token };
            getProperties_device(ledType, host, params);
            break;

          case "wled":
            //Ensure that elements are defaulted for new host
            updateElementsWled(ledType, host);
            params = { host: host };
            getProperties_device(ledType, host, params);
            break;

          case "udpraw":
            getProperties_device(ledType, host, params);
            break;

          default:
        }
      }
    });

    conf_editor.watch('root.specificOptions.output', () => {
      var output = conf_editor.getEditor("root.specificOptions.output").getValue();
      if (output === "NONE" || output === "SELECT" || output === "") {

        $('#btn_submit_controller').prop('disabled', true);
        $('#btn_test_controller').prop('disabled', true);
        $('#btn_test_controller').hide();

        conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(1);
        showAllDeviceInputOptions("output", false);
      }
      else {
        showAllDeviceInputOptions("output", true);
        let params = {};
        var canIdentify = false;
        switch (ledType) {
          case "adalight":
            canIdentify = true;
            break;
          case "atmo":
          case "karate":
            params = { serialPort: output };
            getProperties_device(ledType, output, params);
            break;
          case "dmx":
          case "sedu":
          case "tpm2":
          case "apa102":
          case "apa104":
          case "ws2801":
          case "lpd6803":
          case "lpd8806":
          case "p9813":
          case "sk6812spi":
          case "sk6822spi":
          case "sk9822":
          case "ws2812spi":
          case "piblaster":
          default:
        }

        if ($.inArray(ledType, devSerial) != -1) {
          var rateList = conf_editor.getEditor("root.specificOptions.rateList").getValue();
          var showRate = false;
          if (rateList == "CUSTOM") {
            showRate = true;
          }
          showInputOptionForItem(conf_editor, 'specificOptions', 'rate', showRate);
        }

        if (!conf_editor.validate().length) {
          if (canIdentify) {
            $("#btn_test_controller").show();
            $('#btn_test_controller').prop('disabled', false);
          } else {
            $('#btn_test_controller').hide();
            $('#btn_test_controller').prop('disabled', true);
          }
          if (!window.readOnlyMode) {
            $('#btn_submit_controller').prop('disabled', false);
          }
        }
      }
    });

    conf_editor.watch('root.specificOptions.subType', () => {
      var subType = conf_editor.getEditor("root.specificOptions.subType").getValue();
      let params = {};

      switch (ledType) {
        case "razer":
          params = { subType: subType };
          getProperties_device(ledType, subType, params);
          break;
        default:
      }
    });

    conf_editor.watch('root.specificOptions.streamProtocol', () => {
      var streamProtocol = conf_editor.getEditor("root.specificOptions.streamProtocol").getValue();

      switch (ledType) {
        case "adalight":
          var rate;
          if (streamProtocol === window.serverConfig.device.streamProtocol) {
            rate = window.serverConfig.device.rate.toString();
          } else {
            // Set default rates per protocol type
            switch (streamProtocol) {
              case "2":
                rate = "2000000";
                break;
              case "0":
              case "1":
              default:
                rate = "115200";
            }
          }
          conf_editor.getEditor("root.specificOptions.rateList").setValue(rate);
          break;
        case "wled":
          var hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
          validateWledLedCount(hardwareLedCount);
          break;
        default:
      }
    });

    conf_editor.watch('root.specificOptions.rateList', () => {
      var specOptPath = 'root.specificOptions.';
      var rateList = conf_editor.getEditor("root.specificOptions.rateList");
      if (rateList) {
        var val = rateList.getValue();
        var rate = conf_editor.getEditor("root.specificOptions.rate");

        switch (val) {
          case 'CUSTOM':
          case '':
            rate.enable();
            //Populate existing rate for current custom config
            if (ledType === window.serverConfig.device.type) {
              rate.setValue(window.serverConfig.device.rate);
            } else {
              rate.setValue("");
            }
            break;
          default:
            rate.disable();
            rate.setValue(val);
            //Trigger getProperties via rate value
            conf_editor.notifyWatchers(specOptPath + "rate");
            break;
        }
      }
      showInputOptionForItem(conf_editor, 'specificOptions', 'rate', rate.isEnabled());
    });

    conf_editor.watch('root.specificOptions.token', () => {
      var token = conf_editor.getEditor("root.specificOptions.token").getValue();

      if (token !== "") {
        let params = {};

        var host = "";
        switch (ledType) {
          case "nanoleaf":
            host = conf_editor.getEditor("root.specificOptions.host").getValue();
            if (host === "") {
              return
            }
            params = { host: host, token: token };
            break;
          default:
        }

        getProperties_device(ledType, host, params);
      }
    });

    //Yeelight
    conf_editor.watch('root.specificOptions.lights', () => {
      //Disable General Options, as LED count will be resolved from number of lights configured
      disableAutoResolvedGeneralOptions();

      var hwLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount")
      if (hwLedCount) {
        var lights = conf_editor.getEditor("root.specificOptions.lights").getValue();
        hwLedCount.setValue(lights.length);
      }
    });

    //Philips Hue
    conf_editor.watch('root.specificOptions.lightIds', () => {
      //Disable General Options, as LED count will be resolved from number of lights configured
      disableAutoResolvedGeneralOptions();

      var hwLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount")
      if (hwLedCount) {
        var lights = conf_editor.getEditor("root.specificOptions.lightIds").getValue();
        hwLedCount.setValue(lights.length);
      }
    });

    //Atmo Orb
    conf_editor.watch('root.specificOptions.orbIds', () => {
      //Disable General Options, as LED count will be resolved from number of lights configured
      disableAutoResolvedGeneralOptions();

      var hwLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount")
      if (hwLedCount) {
        var lights = 0;
        var configruedOrbIds = conf_editor.getEditor("root.specificOptions.orbIds").getValue().trim();
        if (configruedOrbIds.length !== 0) {
          lights = configruedOrbIds.split(",").map(Number);
        }
        hwLedCount.setValue(lights.length);
      }
    });

    //WLED
    conf_editor.watch('root.specificOptions.segments.segmentList', () => {

      //Update hidden streamSegmentId element
      var selectedSegment = conf_editor.getEditor("root.specificOptions.segments.segmentList").getValue();
      var streamSegmentId = parseInt(selectedSegment);
      conf_editor.getEditor("root.specificOptions.segments.streamSegmentId").setValue(streamSegmentId);

      if (devicesProperties[ledType]) {
        var host = conf_editor.getEditor("root.specificOptions.host").getValue();
        var ledDeviceProperties = devicesProperties[ledType][host];

        if (ledDeviceProperties) {
          var hardwareLedCount = 1;
          if (streamSegmentId > -1) {
            // Set hardware LED count to segment length
            if (ledDeviceProperties.state) {
              var segments = ledDeviceProperties.state.seg;
              var segmentConfig = segments.filter(seg => seg.id == streamSegmentId)[0];
              hardwareLedCount = segmentConfig.len;
            }
          } else {
            //"Use main segment only" is disabled, i.e. stream to all LEDs
            if (ledDeviceProperties.info) {
              hardwareLedCount = ledDeviceProperties.info.leds.count;
            }
          }
          conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(hardwareLedCount);
        }
      }
    });

    //Handle Hardware Led Count constraint list
    conf_editor.watch('root.generalOptions.hardwareLedCountList', () => {
      var hwLedCountSelected = conf_editor.getEditor("root.generalOptions.hardwareLedCountList").getValue();
      conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(Number(hwLedCountSelected));
    });

    //Handle Hardware Led update and constraints
    conf_editor.watch('root.generalOptions.hardwareLedCount', () => {
      var hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
      switch (ledType) {
        case "wled":
          validateWledLedCount(hardwareLedCount);
          break;
        default:
      }
    });
  });

  //philipshueentertainment backward fix
  if (window.serverConfig.device.type == "philipshueentertainment") window.serverConfig.device.type = "philipshue";

  // create led device selection
  var ledDevices = window.serverInfo.ledDevices.available;

  var optArr = [[]];
  optArr[1] = [];
  optArr[2] = [];
  optArr[3] = [];
  optArr[4] = [];
  optArr[5] = [];

  for (var idx = 0; idx < ledDevices.length; idx++) {
    if ($.inArray(ledDevices[idx], devRPiSPI) != -1)
      optArr[0].push(ledDevices[idx]);
    else if ($.inArray(ledDevices[idx], devRPiPWM) != -1)
      optArr[1].push(ledDevices[idx]);
    else if ($.inArray(ledDevices[idx], devRPiGPIO) != -1)
      optArr[2].push(ledDevices[idx]);
    else if ($.inArray(ledDevices[idx], devNET) != -1)
      optArr[3].push(ledDevices[idx]);
    else if ($.inArray(ledDevices[idx], devSerial) != -1)
      optArr[4].push(ledDevices[idx]);
    else if ($.inArray(ledDevices[idx], devHID) != -1)
      optArr[4].push(ledDevices[idx]);
    else
      optArr[5].push(ledDevices[idx]);
  }

  $("#leddevices").append(createSel(optArr[0], $.i18n('conf_leds_optgroup_RPiSPI')));
  $("#leddevices").append(createSel(optArr[1], $.i18n('conf_leds_optgroup_RPiPWM')));
  $("#leddevices").append(createSel(optArr[2], $.i18n('conf_leds_optgroup_RPiGPIO')));
  $("#leddevices").append(createSel(optArr[3], $.i18n('conf_leds_optgroup_network')));
  $("#leddevices").append(createSel(optArr[4], $.i18n('conf_leds_optgroup_usb')));

  if (storedAccess === 'expert' || window.serverConfig.device.type === "file") {
    $("#leddevices").append(createSel(optArr[5], $.i18n('conf_leds_optgroup_other')));
  }

  $("#leddevices").val(window.serverConfig.device.type);
  $("#leddevices").trigger("change");

  // Identify/ Test LED-Device
  $("#btn_test_controller").off().on("click", function () {
    var ledType = $("#leddevices").val();
    let params = {};

    switch (ledType) {
      case "cololight":
      case "wled":
        var host = conf_editor.getEditor("root.specificOptions.host").getValue();
        params = { host: host };
        break;

      case "nanoleaf":
        var host = conf_editor.getEditor("root.specificOptions.host").getValue();
        var token = conf_editor.getEditor("root.specificOptions.token").getValue();
        params = { host: host, token: token };
        break;

      case "adalight":
        var currentLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
        params = Object.assign(conf_editor.getEditor("root.generalOptions").getValue(),
          conf_editor.getEditor("root.specificOptions").getValue(),
          { currentLedCount }
        );
      default:
    }

    identify_device(ledType, params);
  });

  // Save LED device config
  $("#btn_submit_controller").off().on("click", function (event) {
    var hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
    var layoutLedCount = aceEdt.get().length;

    if (hardwareLedCount === layoutLedCount) {
      saveLedConfig(false);
    } else {
      if (hardwareLedCount > layoutLedCount) {
        // More Hardware LEDs than on layout
        $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
        $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n("conf_leds_config_warning") + '</h4>');
        $('#id_body').append($.i18n('conf_leds_error_hwled_gt_layout', hardwareLedCount, layoutLedCount, hardwareLedCount - layoutLedCount));
        $('#id_body').append('<hr>');
        $('#id_body').append($.i18n('conf_leds_note_layout_overwrite', hardwareLedCount));
        $('#id_footer').html('<button type="button" class="btn btn-secondary" id="btn_back" data-dismiss="modal"><i class="fa fa-fw fa-chevron-left"></i>' + $.i18n('general_btn_back') + '</button>');
        $('#id_footer').append('<button type="button" class="btn btn-danger" id="btn_overwrite" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_overwrite') + '</button>');
        $('#id_footer').append('<button type="button" class="btn btn-primary" id="btn_continue" data-dismiss="modal">' + $.i18n('general_btn_continue') + '<i style="margin-left:4px;"class="fa fa-fw fa-chevron-right"></i> </button>');
      }
      else {
        // Less Hardware LEDs than on layout
        $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-error">');
        $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n("conf_leds_config_error") + '</h4>');
        $('#id_body').append($.i18n('conf_leds_error_hwled_lt_layout', hardwareLedCount, layoutLedCount));
        $('#id_body').append('<hr>');
        $('#id_body').append($.i18n('conf_leds_note_layout_overwrite', hardwareLedCount));
        $('#id_footer').html('<button type="button" class="btn btn-primary" id="btn_back" data-dismiss="modal"><i class="fa fa-fw fa-chevron-left"></i>' + $.i18n('general_btn_back') + '</button>');
        $('#id_footer').append('<button type="button" class="btn btn-danger" id="btn_overwrite" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_overwrite') + '</button>');
      }

      $("#modal_dialog").modal({
        backdrop: "static",
        keyboard: false,
        show: true
      });

      $('#btn_back').off().on('click', function () {
        //Continue with the configuration
      });

      $('#btn_continue').off().on('click', function () {
        saveLedConfig(false);
      });

      $('#btn_overwrite').off().on('click', function () {
        saveLedConfig(true);
      });
    }
  });

  removeOverlay();
});

function saveLedConfig(genDefLayout = false) {
  var ledType = $("#leddevices").val();
  var result = { device: {} };

  var general = conf_editor.getEditor("root.generalOptions").getValue();
  var specific = conf_editor.getEditor("root.specificOptions").getValue();
  for (var key in general) {
    result.device[key] = general[key];
  }

  for (var key in specific) {
    result.device[key] = specific[key];
  }
  result.device.type = ledType;

  var ledConfig = {};
  var leds = [];

  var hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
  result.device.hardwareLedCount = hardwareLedCount;

  // Special handling per LED-type
  switch (ledType) {
    case "cololight":

      var host = conf_editor.getEditor("root.specificOptions.host").getValue();
      if (window.serverConfig.device.type !== ledType) {
        //smoothing off, if new device
        result.smoothing = { enable: false };
      }

      if (genDefLayout === true) {

        if (!jQuery.isEmptyObject(devicesProperties) && devicesProperties[ledType][host].modelType === "Strip") {
          ledConfig = {
            "classic": {
              "top": hardwareLedCount / 2,
              "bottom": 0,
              "left": hardwareLedCount / 4,
              "right": hardwareLedCount / 4,
              "position": hardwareLedCount / 4 * 3
            },
            "matrix": { "cabling": "snake", "ledshoriz": 1, "ledsvert": 1, "start": "top-left" }
          };
          leds = createClassicLedLayoutSimple(hardwareLedCount / 2, hardwareLedCount / 4, hardwareLedCount / 4, 0, hardwareLedCount / 4 * 3, false);
        }
        else {
          ledConfig = {
            "classic": {
              "top": hardwareLedCount,
              "bottom": 0,
              "left": 0,
              "right": 0
            },
            "matrix": { "cabling": "snake", "ledshoriz": 1, "ledsvert": 1, "start": "top-left" }
          };
          leds = createClassicLedLayoutSimple(hardwareLedCount, 0, 0, 0, 0, false);
        }
        result.ledConfig = ledConfig;
        result.leds = leds;
      }
      break;

    case "nanoleaf":
    case "wled":
    case "yeelight":
      if (window.serverConfig.device.type !== ledType) {
        //smoothing off, if new device
        result.smoothing = { enable: false };
      }

    case "adalight":
    case "atmo":
    case "dmx":
    case "karate":
    case "sedu":
    case "tpm2":
    case "apa102":
    case "apa104":
    case "ws2801":
    case "lpd6803":
    case "lpd8806":
    case "p9813":
    case "sk6812spi":
    case "sk6822spi":
    case "sk9822":
    case "ws2812spi":
    case "piblaster":
    default:
      if (genDefLayout === true) {
        ledConfig = {
          "classic": {
            "top": hardwareLedCount,
            "bottom": 0,
            "left": 0,
            "right": 0
          },
          "matrix": { "cabling": "snake", "ledshoriz": 1, "ledsvert": 1, "start": "top-left" }
        }
          ;
        result.ledConfig = ledConfig;
        leds = createClassicLedLayoutSimple(hardwareLedCount, 0, 0, 0, 0, false);
        result.leds = leds;
      }
      break;
  }

  //Rewrite whole LED & Layout configuration, in case changes were done accross tabs and no default layout
  if (genDefLayout !== true) {
    result.ledConfig = getLedConfig();
    result.leds = JSON.parse(aceEdt.getText());
  }

  requestWriteConfig(result);
  location.reload();
}

// build dynamic enum for hosts or output paths
var updateOutputSelectList = function (ledType, discoveryInfo) {
  // Only update, if ledType is equal of selected controller type and discovery info exists
  if (ledType !== $("#leddevices").val() || !discoveryInfo.devices) {
    return;
  }

  let addSchemaElements = {
  };

  var key;
  var enumVals = [];
  var enumTitleVals = [];
  var enumDefaultVal = "";
  var addSelect = false;
  var addCustom = false;

  var ledTypeGroup;

  if ($.inArray(ledType, devNET) != -1) {
    ledTypeGroup = "devNET";
  } else if ($.inArray(ledType, devSerial) != -1) {
    ledTypeGroup = "devSerial";
  } else if ($.inArray(ledType, devRPiSPI) != -1) {
    ledTypeGroup = "devRPiSPI";
  } else if ($.inArray(ledType, devRPiGPIO) != -1) {
    ledTypeGroup = "devRPiGPIO";
  } else if ($.inArray(ledType, devRPiPWM) != -1) {
    ledTypeGroup = "devRPiPWM";
  }

  switch (ledTypeGroup) {
    case "devNET":
      key = "hostList";

      if (discoveryInfo.devices.length === 0) {
        enumVals.push("NONE");
        enumTitleVals.push($.i18n('edt_dev_spec_devices_discovered_none'));
      }
      else {
        var name;

        var discoveryMethod = "ssdp";
        if (discoveryInfo.discoveryMethod) {
          discoveryMethod = discoveryInfo.discoveryMethod;
        }

        for (const device of discoveryInfo.devices) {
          var name;
          var host;

          if (discoveryMethod === "ssdp") {
            host = device.ip;
          }
          else {
            host = device.service;
          }

          switch (ledType) {
            case "nanoleaf":
              if (discoveryMethod === "ssdp") {
                name = device.other["nl-devicename"] + " (" + host + ")";
              }
              else {
                name = device.name;
              }
              break;
            default:
              if (discoveryMethod === "ssdp") {
                name = device.hostname + " (" + host + ")";
              }
              else {
                name = device.name;
              }
              break;
          }

          enumVals.push(host);
          enumTitleVals.push(name);
        }

        //Always allow to add custom configuration
        addCustom = true;
        // Select configured device
        var configuredDeviceType = window.serverConfig.device.type;
        var configuredHost = window.serverConfig.device.hostList;
        if (ledType === configuredDeviceType) {
          if ($.inArray(configuredHost, enumVals) != -1) {
            enumDefaultVal = configuredHost;
          } else if (configuredHost === "CUSTOM") {
            enumDefaultVal = "CUSTOM";
          } else {
            addSelect = true;
          }
        }
        else {
          addSelect = true;
        }
      }
      break;

    case "devSerial":
      key = "output";

      if (discoveryInfo.devices.length == 0) {
        enumVals.push("NONE");
        enumTitleVals.push($.i18n('edt_dev_spec_devices_discovered_none'));
        $('#btn_submit_controller').prop('disabled', true);
        showAllDeviceInputOptions(key, false);
      }
      else {
        switch (ledType) {
          case "adalight":
          case "atmo":
          case "dmx":
          case "karate":
          case "sedu":
          case "tpm2":
            for (const device of discoveryInfo.devices) {
              if (device.udev) {
                enumVals.push(device.systemLocation);
              } else {
                enumVals.push(device.portName);
              }
              enumTitleVals.push(device.portName + " (" + device.vendorIdentifier + "|" + device.productIdentifier + ") - " + device.manufacturer);
            }

            // Select configured device
            var configuredDeviceType = window.serverConfig.device.type;
            var configuredOutput = window.serverConfig.device.output;
            if (ledType === configuredDeviceType) {
              if ($.inArray(configuredOutput, enumVals) != -1) {
                enumDefaultVal = configuredOutput;
              } else {
                enumVals.push(window.serverConfig.device.output);
                enumDefaultVal = configuredOutput;
              }
            }
            else {
              addSelect = true;
            }

            break;
          default:
        }
      }
      break;
    case "devRPiSPI":
    case "devRPiGPIO":
      key = "output";

      if (discoveryInfo.devices.length == 0) {
        enumVals.push("NONE");
        enumTitleVals.push($.i18n('edt_dev_spec_devices_discovered_none'));
        $('#btn_submit_controller').prop('disabled', true);
        showAllDeviceInputOptions(key, false);
      }
      else {
        switch (ledType) {
          case "apa102":
          case "apa104":
          case "ws2801":
          case "lpd6803":
          case "lpd8806":
          case "p9813":
          case "sk6812spi":
          case "sk6822spi":
          case "sk9822":
          case "ws2812spi":
          case "piblaster":
            for (const device of discoveryInfo.devices) {
              enumVals.push(device.systemLocation);
              enumTitleVals.push(device.deviceName + " (" + device.systemLocation + ")");
            }

            // Select configured device
            var configuredDeviceType = window.serverConfig.device.type;
            var configuredOutput = window.serverConfig.device.output;
            if (ledType === configuredDeviceType && $.inArray(configuredOutput, enumVals) != -1) {
              enumDefaultVal = configuredOutput;
            }
            else {
              addSelect = true;
            }

            break;
          default:
        }
      }
      break;
    case "devRPiPWM":
      key = ledType;

      if (!discoveryInfo.isUserAdmin) {
        enumVals.push("NONE");
        enumTitleVals.push($.i18n('edt_dev_spec_devices_discovered_none'));
        $('#btn_submit_controller').prop('disabled', true);
        showAllDeviceInputOptions(key, false);

        $("#info_container_text").html($.i18n("conf_leds_info_ws281x"));
      }
      break;
    default:
  }

  if (enumVals.length > 0) {
    updateJsonEditorSelection(conf_editor, 'root.specificOptions', key, addSchemaElements, enumVals, enumTitleVals, enumDefaultVal, addSelect, addCustom);
  }
};

async function discover_device(ledType, params) {

  const result = await requestLedDeviceDiscovery(ledType, params);

  var discoveryResult = {};
  if (result) {
    if (result.error) {
      throw (result.error);
    }
    discoveryResult = result.info;
  }
  else {
    discoveryResult = {
      devices: [],
      ledDevicetype: ledType
    }
  }
  return discoveryResult;
}

async function getProperties_device(ledType, key, params) {
  var disabled = $('#btn_submit_controller').is(':disabled');
  // Take care that connfig cannot be saved during background processing
  $('#btn_submit_controller').prop('disabled', true);

  //Create ledType cache entry
  if (!devicesProperties[ledType]) {
    devicesProperties[ledType] = {};
  }

  // get device's properties, if properties not available in chache
  if (!devicesProperties[ledType][key]) {
    const res = await requestLedDeviceProperties(ledType, params);
    if (res && !res.error) {
      var ledDeviceProperties = res.info.properties;

      if (!jQuery.isEmptyObject(ledDeviceProperties)) {
        devicesProperties[ledType][key] = ledDeviceProperties;

        if (!window.readOnlyMode) {
          $('#btn_submit_controller').prop('disabled', disabled);
        }
      }
      else {
        showNotification('warning', $.i18n('conf_leds_error_get_properties_text'), $.i18n('conf_leds_error_get_properties_title'));
        $('#btn_submit_controller').prop('disabled', true);
        $('#btn_test_controller').prop('disabled', true);
      }
    }
  }

  updateElements(ledType, key);
}

async function identify_device(type, params) {
  var disabled = $('#btn_submit_controller').is(':disabled');
  // Take care that connfig cannot be saved and identification cannot be retriggerred during background processing
  $('#btn_submit_controller').prop('disabled', true);
  $('#btn_test_controller').prop('disabled', true);

  await requestLedDeviceIdentification(type, params);

  $('#btn_test_controller').prop('disabled', false);
  if (!window.readOnlyMode) {
    $('#btn_submit_controller').prop('disabled', disabled);
  }
}

function updateElements(ledType, key) {
  if (devicesProperties[ledType][key]) {
    var hardwareLedCount = 1;
    switch (ledType) {
      case "cololight":
        var ledProperties = devicesProperties[ledType][key];

        if (ledProperties) {
          hardwareLedCount = ledProperties.ledCount;
        }
        conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(hardwareLedCount);
        break;
      case "wled":
        updateElementsWled(ledType, key);
        break;

      case "nanoleaf":
        var ledProperties = devicesProperties[ledType][key];

        if (ledProperties && ledProperties.panelLayout.layout) {
          //Identify non-LED type panels, e.g. Rhythm (1) and Shapes Controller (12)
          var nonLedNum = 0;
          for (const panel of ledProperties.panelLayout.layout.positionData) {
            if (panel.shapeType === 1 || panel.shapeType === 12) {
              nonLedNum++;
            }
          }
          hardwareLedCount = ledProperties.panelLayout.layout.numPanels - nonLedNum;
        }
        conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(hardwareLedCount);

        break;

      case "udpraw":
        var ledProperties = devicesProperties[ledType][key];

        if (ledProperties && ledProperties.maxLedCount) {
          hardwareLedCount = conf_editor.getEditor("root.generalOptions.hardwareLedCount").getValue();
          var maxLedCount = ledProperties.maxLedCount;
          if (hardwareLedCount > maxLedCount) {
            showInfoDialog('warning', $.i18n("conf_leds_config_warning"), $.i18n('conf_leds_error_hwled_gt_maxled', hardwareLedCount, maxLedCount, maxLedCount));
            hardwareLedCount = maxLedCount;
          }
          updateJsonEditorRange(conf_editor, "root.generalOptions", "hardwareLedCount", 1, maxLedCount, hardwareLedCount);
        }
        break;

      case "atmo":
      case "karate":
        var ledProperties = devicesProperties[ledType][key];

        if (ledProperties && ledProperties.ledCount) {
          if (ledProperties.ledCount.length > 0) {
            var configuredLedCount = window.serverConfig.device.hardwareLedCount;
            showInputOptionForItem(conf_editor, 'generalOptions', "hardwareLedCount", false);
            updateJsonEditorSelection(conf_editor, 'root.generalOptions', "hardwareLedCountList", { "title": "edt_dev_general_hardwareLedCount_title" },
              ledProperties.ledCount.map(String), [], configuredLedCount);
          }
        }
        break;

      case "razer":
        var ledProperties = devicesProperties[ledType][key];
        if (ledProperties) {
          conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(ledProperties.maxLedCount);
          $("#ip_ma_ledshoriz").val(ledProperties.maxColumn);
          $("#ip_ma_ledsvert").val(ledProperties.maxRow);
          $("#ip_ma_cabling").val("parallel");
          $("#ip_ma_direction").val("horizontal");
          $("#ip_ma_start").val("top-left");
          createMatrixLeds();
        }
        break;

      default:
    }
  }

  if (!conf_editor.validate().length) {
    if (!window.readOnlyMode) {
      $('#btn_submit_controller').attr('disabled', false);
    }
  }
  else {
    $('#btn_submit_controller').attr('disabled', true);
  }
}

function showAllDeviceInputOptions(showForKey, state) {
  showInputOptionsForKey(conf_editor, "generalOptions", showForKey, state);
  showInputOptionsForKey(conf_editor, "specificOptions", showForKey, state);
}

function disableAutoResolvedGeneralOptions() {
  conf_editor.getEditor("root.generalOptions.hardwareLedCount").disable();
  conf_editor.getEditor("root.generalOptions.colorOrder").disable();
}

function validateWledSegmentConfig(streamSegmentId) {
  var overlapSegNames = [];
  if (streamSegmentId > -1) {
    if (!jQuery.isEmptyObject(devicesProperties)) {
      var host = conf_editor.getEditor("root.specificOptions.host").getValue();
      var ledProperties = devicesProperties['wled'][host];
      if (ledProperties && ledProperties.state) {
        var segments = ledProperties.state.seg;
        var segmentConfig = segments.filter(seg => seg.id == streamSegmentId)[0];

        var overlappingSegments = segments.filter((seg) => {
          if (seg.id != streamSegmentId) {
            if ((segmentConfig.start >= seg.stop) || (segmentConfig.start < seg.start && segmentConfig.stop <= seg.start)) {
              return false;
            }
            return true;
          }
        });

        if (overlappingSegments.length > 0) {
          var overlapSegNames = [];
          for (const segment of overlappingSegments) {
            if (segment.n) {
              overlapSegNames.push(segment.n);
            } else {
              overlapSegNames.push("Segment " + segment.id);
            }
          }
        }
      }
    }
  }
  return overlapSegNames;
}

function validateWledLedCount(hardwareLedCount) {

  if (!jQuery.isEmptyObject(devicesProperties)) {
    var host = conf_editor.getEditor("root.specificOptions.host").getValue();
    var ledDeviceProperties = devicesProperties["wled"][host];

    if (ledDeviceProperties) {

      var streamProtocol = conf_editor.getEditor("root.specificOptions.streamProtocol").getValue();
      if (streamProtocol === "RAW") {
        var maxLedCount = 490;
        if (ledDeviceProperties.maxLedCount) {
          //WLED not DDP ready
          maxLedCount = ledDeviceProperties.maxLedCount;
          if (hardwareLedCount > maxLedCount) {
            showInfoDialog('warning', $.i18n("conf_leds_config_warning"), $.i18n('conf_leds_error_hwled_gt_maxled', hardwareLedCount, maxLedCount, maxLedCount));
            hardwareLedCount = maxLedCount;
            conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(hardwareLedCount);
            conf_editor.getEditor("root.specificOptions.streamProtocol").setValue("RAW");
          }
        } else {
          //WLED is DDP ready
          if (hardwareLedCount > maxLedCount) {
            var newStreamingProtocol = "DDP";
            showInfoDialog('warning', $.i18n("conf_leds_config_warning"), $.i18n('conf_leds_error_hwled_gt_maxled_protocol', hardwareLedCount, maxLedCount, newStreamingProtocol));
            conf_editor.getEditor("root.specificOptions.streamProtocol").setValue(newStreamingProtocol);
          }
        }
      }
    }
  }
}

function updateElementsWled(ledType, key) {

  // Get configured device's details
  var configuredDeviceType = window.serverConfig.device.type;
  var configuredHost = window.serverConfig.device.host;
  var host = conf_editor.getEditor("root.specificOptions.host").getValue();

  //New segment selection list values
  var enumSegSelectVals = [];
  var enumSegSelectTitleVals = [];
  var enumSegSelectDefaultVal = "";
  var defaultSegmentId = "-1";

  if (devicesProperties[ledType] && devicesProperties[ledType][key]) {
    var ledDeviceProperties = devicesProperties[ledType][key];

    if (!jQuery.isEmptyObject(ledDeviceProperties)) {

      if (ledDeviceProperties.info) {
        if (!ledDeviceProperties.info.hasOwnProperty("liveseg") || ledDeviceProperties.info.liveseg < 0) {
          // "Use main segment only" is disabled
          enumSegSelectVals.push(defaultSegmentId);
          enumSegSelectTitleVals.push($.i18n('edt_dev_spec_segments_disabled_title'));
          enumSegSelectDefaultVal = defaultSegmentId;

        } else {
          if (ledDeviceProperties.state) {
            //Prepare new segment selection list
            var segments = ledDeviceProperties.state.seg;
            for (const segment of segments) {
              enumSegSelectVals.push(segment.id.toString());
              if (segment.n) {
                enumSegSelectTitleVals.push(segment.n);
              } else {
                enumSegSelectTitleVals.push("Segment " + segment.id);
              }
            }
            var currentSegmentId = conf_editor.getEditor("root.specificOptions.segments.streamSegmentId").getValue().toString();
            enumSegSelectDefaultVal = currentSegmentId;
          }
        }

        // Check if currently configured segment is available at WLED
        var configuredDeviceType = window.serverConfig.device.type;
        var configuredHost = window.serverConfig.device.host;

        var host = conf_editor.getEditor("root.specificOptions.host").getValue();
        if (configuredDeviceType == ledType && configuredHost == host) {
          var configuredStreamSegmentId = window.serverConfig.device.segments.streamSegmentId.toString();
          var segmentIdFound = enumSegSelectVals.filter(segId => segId == configuredStreamSegmentId).length;
          if (!segmentIdFound) {
            showInfoDialog('warning', $.i18n("conf_leds_config_warning"), $.i18n('conf_leds_error_wled_segment_missing', configuredStreamSegmentId));
          }
        }
      }
    }
  } else {
    //If failed to get properties
    var hardwareLedCount;
    var segmentConfig = false;

    if (configuredDeviceType == ledType && configuredHost == host) {
      // Populate elements from existing configuration
      if (window.serverConfig.device.segments) {
        segmentConfig = true;
      }
      hardwareLedCount = window.serverConfig.device.hardwareLedCount;
    } else {
      // Populate elements with default values
      hardwareLedCount = 1;
    }

    if (segmentConfig && segmentConfig.streamSegmentId > defaultSegmentId) {
      var configuredstreamSegmentId = window.serverConfig.device.segments.streamSegmentId.toString();
      enumSegSelectVals = [configuredstreamSegmentId];
      enumSegSelectTitleVals = ["Segment " + configuredstreamSegmentId];
      enumSegSelectDefaultVal = configuredstreamSegmentId;
    } else {
      enumSegSelectVals.push(defaultSegmentId);
      enumSegSelectTitleVals.push($.i18n('edt_dev_spec_segments_disabled_title'));
      enumSegSelectDefaultVal = defaultSegmentId;
    }
    conf_editor.getEditor("root.generalOptions.hardwareLedCount").setValue(hardwareLedCount);
  }

  updateJsonEditorSelection(conf_editor, 'root.specificOptions.segments',
    'segmentList', {}, enumSegSelectVals, enumSegSelectTitleVals, enumSegSelectDefaultVal, false, false);

  //Show additional configuration options, if more than one segment is available
  var showAdditionalOptions = false;
  if (enumSegSelectVals.length > 1) {
    showAdditionalOptions = true;
  }
  showInputOptionForItem(conf_editor, "root.specificOptions.segments", "switchOffOtherSegments", showAdditionalOptions);
}

