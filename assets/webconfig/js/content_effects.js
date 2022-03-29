$(document).ready(function () {
  performTranslation();

  var EFFECTENGINE_ENABLED = (jQuery.inArray("effectengine", window.serverInfo.services) !== -1);

  // update instance listing
  updateHyperionInstanceListing();

  var oldEffects = [];
  var effects_editor = null;
  var confFgEff = window.serverConfig.foregroundEffect.effect;
  var confBgEff = window.serverConfig.backgroundEffect.effect;
  var foregroundEffect_editor = null;
  var backgroundEffect_editor = null;

  if (!EFFECTENGINE_ENABLED) {
    window.schema.foregroundEffect.properties.type.enum.splice(1, 1);
    window.schema.foregroundEffect.properties.type.options.enum_titles.splice(1, 1);
    window.schema.foregroundEffect.properties.type.default = "color";
    delete window.schema.foregroundEffect.properties.effect;

    window.schema.backgroundEffect.properties.type.enum.splice(1, 1);
    window.schema.backgroundEffect.properties.type.options.enum_titles.splice(1, 1);
    window.schema.backgroundEffect.properties.type.default = "color";
    delete window.schema.backgroundEffect.properties.effect;
  }

  if (window.showOptHelp) {
    //foreground effect
    $('#conf_cont').append(createRow('conf_cont_fge'));
    $('#conf_cont_fge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
    $('#conf_cont_fge').append(createHelpTable(window.schema.foregroundEffect.properties, $.i18n("edt_conf_fge_heading_title"), "foregroundEffectHelpPanelId"));

    //background effect
    $('#conf_cont').append(createRow('conf_cont_bge'));
    $('#conf_cont_bge').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
    $('#conf_cont_bge').append(createHelpTable(window.schema.backgroundEffect.properties, $.i18n("edt_conf_bge_heading_title"), "backgroundEffectHelpPanelId"));

    if (EFFECTENGINE_ENABLED) {
      //effect path
      if (storedAccess != 'default') {
        $('#conf_cont').append(createRow('conf_cont_ef'));
        $('#conf_cont_ef').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
        $('#conf_cont_ef').append(createHelpTable(window.schema.effects.properties, $.i18n("edt_conf_effp_heading_title")));
      }
    }
  }
  else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_fge_heading_title"), 'editor_container_foregroundEffect', 'btn_submit_foregroundEffect'));
    $('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_bge_heading_title"), 'editor_container_backgroundEffect', 'btn_submit_backgroundEffect'));
    if (EFFECTENGINE_ENABLED) {
      if (storedAccess != 'default')
        $('#conf_cont').append(createOptPanel('fa-spinner', $.i18n("edt_conf_effp_heading_title"), 'editor_container_effects', 'btn_submit_effects'));
    }
  }

  if (EFFECTENGINE_ENABLED) {
    if (storedAccess != 'default') {
      effects_editor = createJsonEditor('editor_container_effects', {
        effects: window.schema.effects
      }, true, true);

      effects_editor.on('change', function () {
        effects_editor.validate().length || window.readOnlyMode ? $('#btn_submit_effects').prop('disabled', true) : $('#btn_submit_effects').prop('disabled', false);
      });

      $('#btn_submit_effects').off().on('click', function () {
        requestWriteConfig(effects_editor.getValue());
      });
    }
  }

  foregroundEffect_editor = createJsonEditor('editor_container_foregroundEffect', {
    foregroundEffect: window.schema.foregroundEffect
  }, true, true);

  backgroundEffect_editor = createJsonEditor('editor_container_backgroundEffect', {
    backgroundEffect: window.schema.backgroundEffect
  }, true, true);

  foregroundEffect_editor.on('ready', function () {
    if (EFFECTENGINE_ENABLED) {
      updateEffectlist();
    }
  });

  foregroundEffect_editor.on('change', function () {
    var foregroundEffectEnable = foregroundEffect_editor.getEditor("root.foregroundEffect.enable").getValue();
    if (foregroundEffectEnable) {
      showInputOptionsForKey(foregroundEffect_editor, "foregroundEffect", "enable", true);
      $('#foregroundEffectHelpPanelId').show();

    } else {
      showInputOptionsForKey(foregroundEffect_editor, "foregroundEffect", "enable", false);
      $('#foregroundEffectHelpPanelId').hide();
    }

    foregroundEffect_editor.validate().length || window.readOnlyMode ? $('#btn_submit_foregroundEffect').prop('disabled', true) : $('#btn_submit_foregroundEffect').prop('disabled', false);
  });

  backgroundEffect_editor.on('change', function () {
    var backgroundEffectEnable = backgroundEffect_editor.getEditor("root.backgroundEffect.enable").getValue();
    if (backgroundEffectEnable) {
      showInputOptionsForKey(backgroundEffect_editor, "backgroundEffect", "enable", true);
      $('#backgroundEffectHelpPanelId').show();

    } else {
      showInputOptionsForKey(backgroundEffect_editor, "backgroundEffect", "enable", false);
      $('#backgroundEffectHelpPanelId').hide();
    }

    backgroundEffect_editor.validate().length || window.readOnlyMode ? $('#btn_submit_backgroundEffect').prop('disabled', true) : $('#btn_submit_backgroundEffect').prop('disabled', false);
  });

  $('#btn_submit_foregroundEffect').off().on('click', function () {
    var value = foregroundEffect_editor.getValue();
    if (typeof value.foregroundEffect.effect == 'undefined')
      value.foregroundEffect.effect = window.serverConfig.foregroundEffect.effect;
    requestWriteConfig(value);
  });

  $('#btn_submit_backgroundEffect').off().on('click', function () {
    var value = backgroundEffect_editor.getValue();
    if (typeof value.backgroundEffect.effect == 'undefined')
      value.backgroundEffect.effect = window.serverConfig.backgroundEffect.effect;
    requestWriteConfig(value);
  });

  //create introduction
  if (window.showOptHelp) {
    if (EFFECTENGINE_ENABLED) {
      createHint("intro", $.i18n('conf_effect_path_intro'), "editor_container_effects");
    }
    createHint("intro", $.i18n('conf_effect_fgeff_intro'), "editor_container_foregroundEffect");
    createHint("intro", $.i18n('conf_effect_bgeff_intro'), "editor_container_backgroundEffect");
  }

  function updateEffectlist() {
    var newEffects = window.serverInfo.effects;
    if (newEffects.length != oldEffects.length) {
      $('#root_foregroundEffect_effect').html('');
      var usrEffArr = [];
      var sysEffArr = [];

      for (var i = 0; i < newEffects.length; i++) {
        var effectName = newEffects[i].name;
        if (!/^\:/.test(newEffects[i].file))
          usrEffArr.push(effectName);
        else
          sysEffArr.push(effectName);
      }
      $('#root_foregroundEffect_effect').append(createSel(usrEffArr, $.i18n('remote_optgroup_usreffets')));
      $('#root_foregroundEffect_effect').append(createSel(sysEffArr, $.i18n('remote_optgroup_syseffets')));
      $('#root_backgroundEffect_effect').html($('#root_foregroundEffect_effect').html());
      oldEffects = newEffects;

      $('#root_foregroundEffect_effect').val(confFgEff);
      $('#root_backgroundEffect_effect').val(confBgEff);
    }
  }

  //interval update
  $(window.hyperion).on("cmd-effects-update", function (event) {
    window.serverInfo.effects = event.response.data.effects
    updateEffectlist();
  });

  removeOverlay();
});

