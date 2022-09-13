$(document).ready(function () {
  performTranslation();

  // update instance listing
  updateHyperionInstanceListing();

  var oldDelList = [];
  var effectName = "";
  var imageData = "";
  var effects_editor = null;
  var effectPyScript = "";
  var testrun;

  if (window.showOptHelp)
    createHintH("intro", $.i18n('effectsconfigurator_label_intro'), "intro_effc");

  function updateDelEffectlist() {
    var newDelList = window.serverInfo.effects;
    if (newDelList.length != oldDelList.length) {
      $('#effectsdellist').html("");
      var usrEffArr = [];
      var sysEffArr = [];
      for (var idx = 0; idx < newDelList.length; idx++) {
        if (newDelList[idx].file.startsWith(":")) {
          sysEffArr.push(idx);
        }
        else {
          usrEffArr.push(idx);
        }
      }

      if (usrEffArr.length > 0) {
        var usrEffGrp = createSelGroup($.i18n('remote_optgroup_usreffets'));
        for (var idx = 0; idx < usrEffArr.length; idx++)
        {
          usrEffGrp.appendChild(createSelOpt('ext_' + newDelList[usrEffArr[idx]].name, newDelList[usrEffArr[idx]].name));
        }
        $('#effectsdellist').append(usrEffGrp);
      }

      var sysEffGrp = createSelGroup($.i18n('remote_optgroup_syseffets'));
      for (var idx = 0; idx < sysEffArr.length; idx++)
      {
        sysEffGrp.appendChild(createSelOpt('int_' + newDelList[sysEffArr[idx]].name, newDelList[sysEffArr[idx]].name));
      }
      $('#effectsdellist').append(sysEffGrp);

      $("#effectsdellist").trigger("change");

      oldDelList = newDelList;
    }
  }

  function triggerTestEffect() {
    testrun = true;

    var args = effects_editor.getEditor('root.args');
    if ($('input[type=radio][value=url]').is(':checked')) {
      requestTestEffect(effectName, effectPyScript, JSON.stringify(args.getValue()), "");
    } else {
      requestTestEffect(effectName, effectPyScript, JSON.stringify(args.getValue()), imageData);
    }
  };

  // Specify upload handler for image files
  JSONEditor.defaults.options.upload = function (type, file, cbs) {
    var fileReader = new FileReader();

    //check file
    if (!file.type.startsWith('image')) {
      imageData = "";
      cbs.failure('File upload error');
      // TODO clear file dialog.
      showInfoDialog('error', "", $.i18n('infoDialog_writeimage_error_text', file.name));
      return;
    }

    fileReader.onload = function () {
      imageData = this.result.split(',')[1];
      cbs.success(file.name);
    };

    fileReader.readAsDataURL(file);
  };

  $("#effectslist").off().on("change", function (event) {
    if (effects_editor != null)
      effects_editor.destroy();

    for (var idx = 0; idx < effects.length; idx++) {
      if (effects[idx].script == this.value) {
        effects_editor = createJsonEditor('editor_container', {
          args: effects[idx].schemaContent,
        }, false, true, false);

        effectPyScript = effects[idx].script;

        imageData = "";
        $("#name-input").trigger("change");

        var desc = $.i18n(effects[idx].schemaContent.title + '_desc');
        if (desc === effects[idx].schemaContent.title + '_desc') {
          desc = "";
        }

        $("#eff_desc").html(createEffHint($.i18n(effects[idx].schemaContent.title), desc));
        break;
      }
    }

    effects_editor.on('change', function () {
      if ($("#btn_cont_test").hasClass("btn-success") && effects_editor.validate().length == 0 && effectName != "") {
        triggerTestEffect();
      }
      if (effects_editor.validate().length == 0 && effectName != "") {
        $('#btn_start_test').prop('disabled', false);
        !window.readOnlyMode ? $('#btn_write').prop('disabled', false) : $('#btn_write').prop('disabled', true);
      }
      else {
        $('#btn_start_test, #btn_write').prop('disabled', true);
      }
    });
  });

  // disable or enable control elements
  $("#name-input").on('change keyup', function (event) {
    effectName = encodeHTML($(this).val());
    if ($(this).val() == '') {
      effects_editor.disable();
      $("#eff_footer").children().prop('disabled', true);
    } else {
      effects_editor.enable();
      $("#eff_footer").children().prop('disabled', false);
      !window.readOnlyMode ? $('#btn_write').prop('disabled', false) : $('#btn_write').prop('disabled', true);
    }
  });

  // Save Effect
  $('#btn_write').off().on('click', function () {
    if ($('input[type=radio][value=url]').is(':checked')) {
      requestWriteEffect(effectName, effectPyScript, JSON.stringify(effects_editor.getValue()), "");
    } else {
      requestWriteEffect(effectName, effectPyScript, JSON.stringify(effects_editor.getValue()), imageData);
    }

    $(window.hyperion).one("cmd-create-effect", function (event) {
      if (event.response.success)
        showInfoDialog('success', "", $.i18n('infoDialog_effconf_created_text', effectName));
    });

    if (testrun)
      setTimeout(requestPriorityClear, 100);
  });

  // Start test
  $('#btn_start_test').off().on('click', function () {
    $('#btn_start_test').prop('disabled', true);
    triggerTestEffect();
  });

  // Stop test
  $('#btn_stop_test').off().on('click', function () {
    requestPriorityClear();
    testrun = false;
    $('#btn_start_test').prop('disabled', false);
  });

  // Continuous test
  $('#btn_cont_test').off().on('click', function () {
    toggleClass('#btn_cont_test', "btn-success", "btn-danger");
  });

  // Delete Effect
  $('#btn_delete').off().on('click', function () {
    var name = $("#effectsdellist").val().split("_")[1];
    requestDeleteEffect(name);
    $(window.hyperion).one("cmd-delete-effect", function (event) {
      if (event.response.success)
        showInfoDialog('success', "", $.i18n('infoDialog_effconf_deleted_text', name));
    });
  });

  // Disable or enable Delete Effect Button
  $('#effectsdellist').off().on('change', function () {
    var value = $(this).val();
    value == null ? $('#btn_edit').prop('disabled', true) : $('#btn_edit').prop('disabled', false);
    value.startsWith("int_") ? $('#btn_delete').prop('disabled', true) : $('#btn_delete').prop('disabled', false);
  });

  // Load Effect
  $('#btn_edit').off().on('click', function () {

    var name = $("#effectsdellist").val().replace("ext_", "");
    if (name.startsWith("int_")) {
      name = name.split("_").pop();
      $("#name-input").val("My Modded Effect");
    }
    else {
      name = name.split("_").pop();
      $("#name-input").val(name);
    }

    var efx = window.serverInfo.effects;
    for (var i = 0; i < efx.length; i++) {
      if (efx[i].name == name) {
        var py = efx[i].script;
        $("#effectslist").val(py).trigger("change");

        for (var key in efx[i].args) {
          var ed = effects_editor.getEditor('root.args.' + [key]);
          if (ed)
            ed.setValue(efx[i].args[key]);
        }
        break;
      }
    }
  });

  //Create effect template list
  var effects = window.serverSchema.properties.effectSchemas;

  $('#effectslist').html("");
  var custTemplatesIDs = [];
  var sysTemplatesIDs = [];

  for (var idx = 0; idx < effects.length; idx++) {
    if (effects[idx].type === "custom")
      custTemplatesIDs.push(idx);
    else
      sysTemplatesIDs.push(idx);
  }

  //Cannot use createSel(), as Windows filenames include colons ":"
  if (custTemplatesIDs.length > 0) {
    var custTmplGrp = createSelGroup($.i18n('remote_optgroup_templates_custom'));
    for (var idx = 0; idx < custTemplatesIDs.length; idx++)
    {
      custTmplGrp.appendChild(createSelOpt(effects[custTemplatesIDs[idx]].script, $.i18n(effects[custTemplatesIDs[idx]].schemaContent.title)));
    }
    $('#effectslist').append(custTmplGrp);
  }

  var sysTmplGrp = createSelGroup($.i18n('remote_optgroup_templates_system'));
  for (var idx = 0; idx < sysTemplatesIDs.length; idx++)
  {
    sysTmplGrp.appendChild(createSelOpt(effects[sysTemplatesIDs[idx]].script, $.i18n(effects[sysTemplatesIDs[idx]].schemaContent.title)));
  }
  $('#effectslist').append(sysTmplGrp);

  $("#effectslist").trigger("change");

  updateDelEffectlist();

  //interval update
  $(window.hyperion).on("cmd-effects-update", function (event) {
    window.serverInfo.effects = event.response.data.effects
    updateDelEffectlist();
  });

  removeOverlay();
});

