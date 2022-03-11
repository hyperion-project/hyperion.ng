$(document).ready(function () {
  performTranslation();

  var BORDERDETECT_ENABLED = (jQuery.inArray("borderdetection", window.serverInfo.services) !== -1);

  // update instance listing
  updateHyperionInstanceListing();

  var editor_color = null;
  var editor_smoothing = null;
  var editor_blackborder = null;

  if (window.showOptHelp) {
    //color
    $('#conf_cont').append(createRow('conf_cont_color'));
    $('#conf_cont_color').append(createOptPanel('fa-photo', $.i18n("edt_conf_color_heading_title"), 'editor_container_color', 'btn_submit_color'));
    $('#conf_cont_color').append(createHelpTable(window.schema.color.properties, $.i18n("edt_conf_color_heading_title")));

    //smoothing
    $('#conf_cont').append(createRow('conf_cont_smoothing'));
    $('#conf_cont_smoothing').append(createOptPanel('fa-photo', $.i18n("edt_conf_smooth_heading_title"), 'editor_container_smoothing', 'btn_submit_smoothing'));
    $('#conf_cont_smoothing').append(createHelpTable(window.schema.smoothing.properties, $.i18n("edt_conf_smooth_heading_title"), "smoothingHelpPanelId"));

    //blackborder
    if (BORDERDETECT_ENABLED) {
      $('#conf_cont').append(createRow('conf_cont_blackborder'));
      $('#conf_cont_blackborder').append(createOptPanel('fa-photo', $.i18n("edt_conf_bb_heading_title"), 'editor_container_blackborder', 'btn_submit_blackborder'));
      $('#conf_cont_blackborder').append(createHelpTable(window.schema.blackborderdetector.properties, $.i18n("edt_conf_bb_heading_title"), "blackborderHelpPanelId"));
    }
  }
  else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_color_heading_title"), 'editor_container_color', 'btn_submit_color'));
    $('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_smooth_heading_title"), 'editor_container_smoothing', 'btn_submit_smoothing'));
    if (BORDERDETECT_ENABLED) {
      $('#conf_cont').append(createOptPanel('fa-photo', $.i18n("edt_conf_bb_heading_title"), 'editor_container_blackborder', 'btn_submit_blackborder'));
    }
  }

  //color
  editor_color = createJsonEditor('editor_container_color', {
    color: window.schema.color
  }, true, true);

  editor_color.on('change', function () {
    editor_color.validate().length || window.readOnlyMode ? $('#btn_submit_color').prop('disabled', true) : $('#btn_submit_color').prop('disabled', false);
  });

  $('#btn_submit_color').off().on('click', function () {
    requestWriteConfig(editor_color.getValue());
  });

  //smoothing
  editor_smoothing = createJsonEditor('editor_container_smoothing', {
    smoothing: window.schema.smoothing
  }, true, true);

  editor_smoothing.on('change', function () {
    var smoothingEnable = editor_smoothing.getEditor("root.smoothing.enable").getValue();
    if (smoothingEnable) {
      showInputOptionsForKey(editor_smoothing, "smoothing", "enable", true);
      $('#smoothingHelpPanelId').show();
    } else {
      showInputOptionsForKey(editor_smoothing, "smoothing", "enable", false);
      $('#smoothingHelpPanelId').hide();
    }
    editor_smoothing.validate().length || window.readOnlyMode ? $('#btn_submit_smoothing').prop('disabled', true) : $('#btn_submit_smoothing').prop('disabled', false);
  });

  $('#btn_submit_smoothing').off().on('click', function () {
    requestWriteConfig(editor_smoothing.getValue());
  });

  //blackborder
  if (BORDERDETECT_ENABLED) {
    editor_blackborder = createJsonEditor('editor_container_blackborder', {
      blackborderdetector: window.schema.blackborderdetector
    }, true, true);

    editor_blackborder.on('change', function () {
      var blackborderEnable = editor_blackborder.getEditor("root.blackborderdetector.enable").getValue();
      if (blackborderEnable) {
        showInputOptionsForKey(editor_blackborder, "blackborderdetector", "enable", true);
        $('#blackborderHelpPanelId').show();
        $('#blackborderWikiLinkId').show();

      } else {
        showInputOptionsForKey(editor_blackborder, "blackborderdetector", "enable", false);
        $('#blackborderHelpPanelId').hide();
        $('#blackborderWikiLinkId').hide();
      }
      editor_blackborder.validate().length || window.readOnlyMode ? $('#btn_submit_blackborder').prop('disabled', true) : $('#btn_submit_blackborder').prop('disabled', false);
    });

    $('#btn_submit_blackborder').off().on('click', function () {
      requestWriteConfig(editor_blackborder.getValue());
    });
  }

  //wiki links
  var wikiElement = $(buildWL("user/advanced/Advanced.html#blackbar-detection", "edt_conf_bb_mode_title", true));
  wikiElement.attr('id', 'blackborderWikiLinkId');
  $('#editor_container_blackborder').append(wikiElement);

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_colors_color_intro'), "editor_container_color");
    createHint("intro", $.i18n('conf_colors_smoothing_intro'), "editor_container_smoothing");
    if (BORDERDETECT_ENABLED) {
      createHint("intro", $.i18n('conf_colors_blackborder_intro'), "editor_container_blackborder");
    }
  }

  removeOverlay();
});
