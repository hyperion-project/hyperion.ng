var prevTag;

function removeOverlay() {
  $("#loading_overlay").removeClass("overlay");
}

function reload() {
  location.reload();
}

function storageComp() {
  if (typeof (Storage) !== "undefined")
    return true;
  return false;
}

function getStorage(item) {
  if (storageComp()) {
    return localStorage.getItem(item);
  }
  return null;
}

function setStorage(item, value) {
  if (storageComp()) {
    localStorage.setItem(item, value);
  }
}

function removeStorage(item) {
  if (storageComp()) {
    localStorage.removeItem(item);
  }
}

function debugMessage(msg) {
  if (window.debugMessagesActive) {
    console.log(msg);
  }
}

function validateDuration(d) {
  if (typeof d === "undefined" || d < 0)
    return ENDLESS;
  else
    return d *= 1000;
}

function getHashtag() {
  if (getStorage('lasthashtag') != null)
    return getStorage('lasthashtag');
  else {
    var tag = document.URL;
    tag = tag.substr(tag.indexOf("#") + 1);
    if (tag == "" || typeof tag === "undefined" || tag.startsWith("http"))
      tag = "dashboard"
    return tag;
  }
}

function loadContent(event, forceRefresh) {
  var tag;

  var lastSelectedInstance = getStorage('lastSelectedInstance');

  if (lastSelectedInstance && (lastSelectedInstance != window.currentHyperionInstance)) {
    if (window.serverInfo.instance[lastSelectedInstance] && window.serverInfo.instance[lastSelectedInstance].running) {
      instanceSwitch(lastSelectedInstance);
    } else {
      removeStorage('lastSelectedInstance');
    }
  }

  if (typeof event != "undefined") {
    tag = event.currentTarget.hash;
    tag = tag.substr(tag.indexOf("#") + 1);
    setStorage('lasthashtag', tag);
  }
  else
    tag = getHashtag();

  if (forceRefresh || prevTag != tag) {
    prevTag = tag;
    $("#page-content").off();
    $("#page-content").load("/content/" + tag + ".html", function (response, status, xhr) {
      if (status == "error") {
        tag = 'dashboard';
        console.log("Could not find page:", prevTag, ", Redirecting to:", tag);
        setStorage('lasthashtag', tag);

        $("#page-content").load("/content/" + tag + ".html", function (response, status, xhr) {
          if (status == "error") {
            $("#page-content").html('<h3>' + encode_utf8(tag) + '<br/>' + $.i18n('info_404') + '</h3>');
            removeOverlay();
          }
        });
      }
      updateUiOnInstance(window.currentHyperionInstance);
    });
  }
}

function getInstanceNameByIndex(index) {
  var instData = window.serverInfo.instance
  for (var key in instData) {
    if (instData[key].instance == index)
      return instData[key].friendly_name;
  }
  return "unknown"
}

function updateHyperionInstanceListing() {
  if (window.serverInfo.instance) {
    var data = window.serverInfo.instance.filter(entry => entry.running);
    $('#hyp_inst_listing').html("");
    for (var key in data) {
      var currInstMarker = (data[key].instance == window.currentHyperionInstance) ? "component-on" : "";

      var html = '<li id="hyperioninstance_' + data[key].instance + '"> \
      <a>  \
        <div>  \
          <i class="fa fa-circle fa-fw '+ currInstMarker + '"></i> \
          <span>'+ data[key].friendly_name + '</span> \
        </div> \
      </a> \
    </li> '

      if (data.length - 1 > key)
        html += '<li class="divider"></li>'

      $('#hyp_inst_listing').append(html);

      $('#hyperioninstance_' + data[key].instance).off().on("click", function (e) {
        var inst = e.currentTarget.id.split("_")[1]
        instanceSwitch(inst)
      });
    }
  }
}

function initLanguageSelection() {
  // Initialise language selection list with languages supported
  for (var i = 0; i < availLang.length; i++) {
    $("#language-select").append('<option value="' + i + '" selected="">' + availLangText[i] + '</option>');
  }

  var langLocale = storedLang;

  //Test, if language is supported by hyperion
  var langIdx = availLang.indexOf(langLocale);
  if (langIdx > -1) {
    langText = availLangText[langIdx];
  } else {
    // If language is not supported by hyperion, try fallback language
    langLocale = $.i18n().options.fallbackLocale.substring(0, 2);
    langIdx = availLang.indexOf(langLocale);
    if (langIdx > -1) {
      langText = availLangText[langIdx];
    } else {
      langLocale = 'en';
      langIdx = availLang.indexOf(langLocale);
      if (langIdx > -1) {
        langText = availLangText[langIdx];
      }
    }
  }

  $('#language-select').prop('title', langText);
  $("#language-select").val(langIdx);
  $("#language-select").selectpicker("refresh");
}

function updateUiOnInstance(inst) {
  $("#active_instance_friendly_name").text(getInstanceNameByIndex(inst));
  if (window.serverInfo.instance.filter(entry => entry.running).length > 1) {
    $('#btn_hypinstanceswitch').toggle(true);
    $('#active_instance_dropdown').prop('disabled', false);
    $('#active_instance_dropdown').css('cursor', 'pointer');
    $("#active_instance_dropdown").css("pointer-events", "auto");
  } else {
    $('#btn_hypinstanceswitch').toggle(false);
    $('#active_instance_dropdown').prop('disabled', true);
    $("#active_instance_dropdown").css('cursor', 'default');
    $("#active_instance_dropdown").css("pointer-events", "none");
  }
}

function instanceSwitch(inst) {
  requestInstanceSwitch(inst)
  window.currentHyperionInstance = inst;
  window.currentHyperionInstanceName = getInstanceNameByIndex(inst);
  setStorage('lastSelectedInstance', inst)
}

function loadContentTo(containerId, fileName) {
  $(containerId).load("/content/" + fileName + ".html");
}

function toggleClass(obj, class1, class2) {
  if ($(obj).hasClass(class1)) {
    $(obj).removeClass(class1);
    $(obj).addClass(class2);
  }
  else {
    $(obj).removeClass(class2);
    $(obj).addClass(class1);
  }
}

function setClassByBool(obj, enable, class1, class2) {
  if (enable) {
    $(obj).removeClass(class1);
    $(obj).addClass(class2);
  }
  else {
    $(obj).removeClass(class2);
    $(obj).addClass(class1);
  }
}

function showInfoDialog(type, header, message) {
  if (type == "success") {
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-check modal-icon-check">');
    if (header == "")
      $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('infoDialog_general_success_title') + '</h4>');
    $('#id_footer').html('<button type="button" class="btn btn-success" data-dismiss="modal">' + $.i18n('general_btn_ok') + '</button>');
  }
  else if (type == "warning") {
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
    if (header == "")
      $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('infoDialog_general_warning_title') + '</h4>');
    $('#id_footer').html('<button type="button" class="btn btn-warning" data-dismiss="modal">' + $.i18n('general_btn_ok') + '</button>');
  }
  else if (type == "error") {
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-error">');
    if (header == "")
      $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('infoDialog_general_error_title') + '</h4>');
    $('#id_footer').html('<button type="button" class="btn btn-danger" data-dismiss="modal">' + $.i18n('general_btn_ok') + '</button>');
  }
  else if (type == "select") {
    $('#id_body').html('<img style="margin-bottom:20px" id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_saveandreload') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "iswitch") {
    $('#id_body').html('<img style="margin-bottom:20px" id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-exchange"></i>' + $.i18n('general_btn_iswitch') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "uilock") {
    $('#id_body').html('<img id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_footer').html('<b>' + $.i18n('InfoDialog_nowrite_foottext') + '</b>');
  }
  else if (type == "import") {
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
    $('#id_footer').html('<button type="button" id="id_btn_import" class="btn btn-warning" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_saverestart') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "delInst") {
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-remove modal-icon-warning">');
    $('#id_footer').html('<button type="button" id="id_btn_yes" class="btn btn-warning" data-dismiss="modal"><i class="fa fa-fw fa-trash"></i>' + $.i18n('general_btn_yes') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "renInst") {
    $('#id_body_rename').html('<i style="margin-bottom:20px" class="fa fa-pencil modal-icon-edit"><br>');
    $('#id_body_rename').append('<h4>' + header + '</h4>');
    $('#id_body_rename').append('<input class="form-control" id="renInst_name" type="text" value="' + message + '">');
    $('#id_footer_rename').html('<button type="button" id="id_btn_ok" class="btn btn-success" data-dismiss-modal="#modal_dialog_rename" disabled><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_ok') + '</button>');
    $('#id_footer_rename').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "changePassword") {
    $('#id_body_rename').html('<i style="margin-bottom:20px" class="fa fa-key modal-icon-edit"><br>');
    $('#id_body_rename').append('<h4>' + header + '</h4><br>');
    $('#id_body_rename').append('<div class="row"><div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_username_text') +
      '</p></div><div class="col-md-8"><input class="form-control" id="username" type="text" value="Hyperion" disabled></div></div><br>');
    $('#id_body_rename').append('<div class="row"><div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_password_current_text') +
      '</p></div><div class="col-md-8"><input class="form-control" id="current-password" placeholder="Old" type="password" autocomplete="current-password"></div></div><br>');
    $('#id_body_rename').append('<div class="row"><div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_password_new_text') +
      '</p></div><div class="col-md-8"><input class="form-control" id="new-password" placeholder="New" type="password" autocomplete="new-password"></div></div>');
    $('#id_body_rename').append('<div class="bs-callout bs-callout-info"><span>' + $.i18n('infoDialog_password_minimum_length') + '</span></div>');
    $('#id_footer_rename').html('<button type="button" id="id_btn_ok" class="btn btn-success" data-dismiss-modal="#modal_dialog_rename" disabled><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_ok') + '</button></div>');
    $('#id_footer_rename').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
  }
  else if (type == "checklist") {
    $('#id_body').html('<img style="margin-bottom:20px" id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('infoDialog_checklist_title') + '</h4>');
    $('#id_body').append(header);
    $('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal">' + $.i18n('general_btn_ok') + '</button>');
  }
  else if (type == "newToken") {
    $('#id_body').html('<img style="margin-bottom:20px" id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal">' + $.i18n('general_btn_ok') + '</button>');
  }
  else if (type == "grantToken") {
    $('#id_body').html('<img style="margin-bottom:20px" id="id_logo" src="img/hyperion/logo_positiv.png" alt="Redefine ambient light!">');
    $('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal" id="tok_grant_acc">' + $.i18n('general_btn_grantAccess') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal" id="tok_deny_acc">' + $.i18n('general_btn_denyAccess') + '</button>');
  }

  if (type != "renInst") {
    $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + header + '</h4>');
    $('#id_body').append(message);
  }

  if (type == "select" || type == "iswitch")
    $('#id_body').append('<select id="id_select" class="form-control" style="margin-top:10px;width:auto;"></select>');

  if (getStorage("darkMode") == "on")
    $('#id_logo').attr("src", 'img/hyperion/logo_negativ.png');

  $(type == "renInst" || type == "changePassword" ? "#modal_dialog_rename" : "#modal_dialog").modal({
    backdrop: "static",
    keyboard: false,
    show: true
  });

  $(document).on('click', '[data-dismiss-modal]', function () {
    var target = $(this).attr('data-dismiss-modal');
    $(target).modal('hide'); // lgtm [js/xss-through-dom]
  });
}

function createHintH(type, text, container) {
  type = String(type);
  if (type == "intro")
    tclass = "introd";

  $('#' + container).prepend('<div class="' + tclass + '"><h4 style="font-size:16px">' + text + '</h4><hr/></div>');
}

function createHint(type, text, container, buttonid, buttontxt) {
  var fe, tclass;

  if (type == "intro") {
    fe = '';
    tclass = "intro-hint";
  }
  else if (type == "info") {
    fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-info"></i></div><div style="text-align:center;font-size:13px">Information</div>';
    tclass = "info-hint";
  }
  else if (type == "wizard") {
    fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-magic"></i></div><div style="text-align:center;font-size:13px">Information</div>';
    tclass = "wizard-hint";
  }
  else if (type == "warning") {
    fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-info"></i></div><div style="text-align:center;font-size:13px">Information</div>';
    tclass = "warning-hint";
  }

  if (buttonid)
    buttonid = '<p><button id="' + buttonid + '" class="btn btn-wizard" style="margin-top:15px;">' + text + '</button></p>';
  else
    buttonid = "";

  if (type == "intro")
    $('#' + container).prepend('<div class="bs-callout bs-callout-primary" style="margin-top:0px"><h4>' + $.i18n("conf_helptable_expl") + '</h4>' + text + '</div>');
  else if (type == "wizard")
    $('#' + container).prepend('<div class="bs-callout bs-callout-wizard" style="margin-top:0px"><h4>' + $.i18n("wiz_wizavail") + '</h4>' + $.i18n('wiz_guideyou', text) + buttonid + '</div>');
  else {
    createTable('', 'htb', container, true, tclass);
    $('#' + container + ' .htb').append(createTableRow([fe, text], false, true));
  }
}

function createEffHint(title, text) {
  return '<div class="bs-callout bs-callout-primary" style="margin-top:0px"><h4>' + title + '</h4>' + text + '</div>';
}

function valValue(id, value, min, max) {
  if (typeof max === 'undefined' || max == "")
    max = 999999;

  if (Number(value) > Number(max)) {
    $('#' + id).val(max);
    showInfoDialog("warning", "", $.i18n('edt_msg_error_maximum_incl', max));
    return max;
  }
  else if (Number(value) < Number(min)) {
    $('#' + id).val(min);
    showInfoDialog("warning", "", $.i18n('edt_msg_error_minimum_incl', min));
    return min;
  }
  return value;
}

function readImg(input, cb) {
  if (input.files && input.files[0]) {
    var reader = new FileReader();
    // inject fileName property
    reader.fileName = input.files[0].name

    reader.onload = function (e) {
      cb(e.target.result, e.target.fileName);
    }
    reader.readAsDataURL(input.files[0]);
  }
}

function isJsonString(str) {
  try {
    JSON.parse(str);
  }
  catch (e) {
    return e;
  }
  return "";
}

function createJsonEditor(container, schema, setconfig, usePanel, arrayre) {
  $('#' + container).off();
  $('#' + container).html("");

  if (typeof arrayre === 'undefined')
    arrayre = true;

  var editor = new JSONEditor(document.getElementById(container),
    {
      theme: 'bootstrap3',
      iconlib: "fontawesome4",
      disable_collapse: 'true',
      form_name_root: 'sa',
      disable_edit_json: true,
      disable_properties: true,
      disable_array_reorder: arrayre,
      no_additional_properties: true,
      disable_array_delete_all_rows: true,
      disable_array_delete_last_row: true,
      access: storedAccess,
      schema: {
        title: '',
        properties: schema
      }
    });

  if (usePanel) {
    $('#' + container + ' .well').first().removeClass('well well-sm');
    $('#' + container + ' h4').first().remove();
    $('#' + container + ' .well').first().removeClass('well well-sm');
  }

  if (setconfig) {
    for (var key in editor.root.editors) {
      editor.getEditor("root." + key).setValue(Object.assign({}, editor.getEditor("root." + key).value, window.serverConfig[key]));
    }
  }

  return editor;
}

function updateJsonEditorSelection(rootEditor, path, key, addElements, newEnumVals, newTitelVals, newDefaultVal, addSelect, addCustom, addCustomAsFirst, customText) {
  var editor = rootEditor.getEditor(path);
  var orginalProperties = editor.schema.properties[key];

  var orginalWatchFunctions = rootEditor.watchlist[path + "." + key];
  rootEditor.unwatch(path + "." + key);

  var newSchema = [];
  newSchema[key] =
  {
    "type": "string",
    "enum": [],
    "required": true,
    "options": { "enum_titles": [], "infoText": "" },
    "propertyOrder": 1
  };

  //Add additional elements to overwrite defaults
  for (var item in addElements) {
    newSchema[key][item] = addElements[item];
  }

  if (orginalProperties) {
    if (orginalProperties["title"]) {
      newSchema[key]["title"] = orginalProperties["title"];
    }

    if (orginalProperties["options"] && orginalProperties["options"]["infoText"]) {
      newSchema[key]["options"]["infoText"] = orginalProperties["options"]["infoText"];
    }

    if (orginalProperties["propertyOrder"]) {
      newSchema[key]["propertyOrder"] = orginalProperties["propertyOrder"];
    }
  }

  if (addCustom) {

    if (newTitelVals.length === 0) {
      newTitelVals = [...newEnumVals];
    }

    if (!!!customText) {
      customText = "edt_conf_enum_custom";
    }

    if (addCustomAsFirst) {
      newEnumVals.unshift("CUSTOM");
      newTitelVals.unshift(customText);
    } else {
      newEnumVals.push("CUSTOM");
      newTitelVals.push(customText);
    }

    if (newSchema[key].options.infoText) {
      var customInfoText = newSchema[key].options.infoText + "_custom";
      newSchema[key].options.infoText = customInfoText;
    }
  }

  if (addSelect) {
    newEnumVals.unshift("SELECT");
    newTitelVals.unshift("edt_conf_enum_please_select");
    newDefaultVal = "SELECT";
  }

  if (newEnumVals) {
    newSchema[key]["enum"] = newEnumVals;
  }

  if (newTitelVals) {
    newSchema[key]["options"]["enum_titles"] = newTitelVals;
  }
  if (newDefaultVal) {
    newSchema[key]["default"] = newDefaultVal;
  }

  editor.original_schema.properties[key] = orginalProperties;
  editor.schema.properties[key] = newSchema[key];
  rootEditor.validator.schema.properties[editor.key].properties[key] = newSchema[key];

  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  if (orginalWatchFunctions) {
    for (var i = 0; i < orginalWatchFunctions.length; i++) {
      rootEditor.watch(path + "." + key, orginalWatchFunctions[i]);
    }
  }
  rootEditor.notifyWatchers(path + "." + key);
}

function updateJsonEditorMultiSelection(rootEditor, path, key, addElements, newEnumVals, newTitelVals, newDefaultVal) {
  var editor = rootEditor.getEditor(path);
  var orginalProperties = editor.schema.properties[key];

  var orginalWatchFunctions = rootEditor.watchlist[path + "." + key];
  rootEditor.unwatch(path + "." + key);

  var newSchema = [];
  newSchema[key] =
  {
    "type": "array",
    "format": "select",
    "items": {
      "type": "string",
      "enum": [],
      "options": { "enum_titles": [] },
    },
    "options": { "infoText": "" },
    "default": [],
    "propertyOrder": 1
  };

  //Add additional elements to overwrite defaults
  for (var item in addElements) {
    newSchema[key][item] = addElements[item];
  }

  if (orginalProperties) {
    if (orginalProperties["title"]) {
      newSchema[key]["title"] = orginalProperties["title"];
    }

    if (orginalProperties["options"] && orginalProperties["options"]["infoText"]) {
      newSchema[key]["options"]["infoText"] = orginalProperties["options"]["infoText"];
    }

    if (orginalProperties["propertyOrder"]) {
      newSchema[key]["propertyOrder"] = orginalProperties["propertyOrder"];
    }
  }

  if (newEnumVals) {
    newSchema[key]["items"]["enum"] = newEnumVals;
  }

  if (newTitelVals) {
    newSchema[key]["items"]["options"]["enum_titles"] = newTitelVals;
  }

  if (newDefaultVal) {
    newSchema[key]["default"] = newDefaultVal;
  }

  editor.original_schema.properties[key] = orginalProperties;
  editor.schema.properties[key] = newSchema[key];
  rootEditor.validator.schema.properties[editor.key].properties[key] = newSchema[key];

  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  if (orginalWatchFunctions) {
    for (var i = 0; i < orginalWatchFunctions.length; i++) {
      rootEditor.watch(path + "." + key, orginalWatchFunctions[i]);
    }
  }
  rootEditor.notifyWatchers(path + "." + key);
}

function updateJsonEditorRange(rootEditor, path, key, minimum, maximum, defaultValue, step, clear) {
  var editor = rootEditor.getEditor(path);

  //Preserve current value when updating range
  var currentValue = rootEditor.getEditor(path + "." + key).getValue();

  var orginalProperties = editor.schema.properties[key];
  var newSchema = [];
  newSchema[key] = orginalProperties;

  if (clear) {
    delete newSchema[key]["minimum"];
    delete newSchema[key]["maximum"];
    delete newSchema[key]["default"];
    delete newSchema[key]["step"];
  }

  if (typeof minimum !== "undefined") {
    newSchema[key]["minimum"] = minimum;
  }
  if (typeof maximum !== "undefined") {
    newSchema[key]["maximum"] = maximum;
  }
  if (typeof defaultValue !== "undefined") {
    newSchema[key]["default"] = defaultValue;
    currentValue = defaultValue;
  }

  if (typeof step !== "undefined") {
    newSchema[key]["step"] = step;
  }

  editor.original_schema.properties[key] = orginalProperties;
  editor.schema.properties[key] = newSchema[key];
  rootEditor.validator.schema.properties[editor.key].properties[key] = newSchema[key];

  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  // Restore current (new default) value for new range
  rootEditor.getEditor(path + "." + key).setValue(currentValue);
}

function addJsonEditorHostValidation() {

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    var errors = [];

    if (!jQuery.isEmptyObject(value)) {
      switch (schema.format) {
        case "hostname_or_ip":
          if (!isValidHostnameOrIP(value)) {
            errors.push({
              path: path,
              property: 'format',
              message: $.i18n('edt_msgcust_error_hostname_ip')
            });
          }
          break;
        case "hostname_or_ip4":
          if (!isValidHostnameOrIP4(value)) {
            errors.push({
              path: path,
              property: 'format',
              message: $.i18n('edt_msgcust_error_hostname_ip4')
            });
          }
          break;

        //Remove, when new json-editor 2.x is used
        case "ipv4":
          if (!isValidIPv4(value)) {
            errors.push({
              path: path,
              property: 'format',
              message: $.i18n('edt_msg_error_ipv4')
            });
          }
          break;
        case "ipv6":
          if (!isValidIPv6(value)) {
            errors.push({
              path: path,
              property: 'format',
              message: $.i18n('edt_msg_error_ipv6')
            });
          }
          break;
        case "hostname":
          if (!isValidHostname(value)) {
            errors.push({
              path: path,
              property: 'format',
              message: $.i18n('edt_msg_error_hostname')
            });
          }
          break;

        default:
      }
    }
    return errors;
  });
}

function buildWL(link, linkt, cl) {
  var baseLink = "https://docs.hyperion-project.org/";
  var lang;

  if (typeof linkt == "undefined")
    linkt = "Placeholder";

  if (storedLang == "de" || navigator.locale == "de")
    lang = "de";
  else
    lang = "en";

  if (cl === true) {
    linkt = $.i18n(linkt);
    return '<div class="bs-callout bs-callout-primary"><h4>' + linkt + '</h4>' + $.i18n('general_wiki_moreto', linkt) + ': <a href="' + baseLink + lang + '/' + link + '" target="_blank">' + linkt + '<a></div>'
  }
  else
    return ': <a href="' + baseLink + lang + '/' + link + '" target="_blank">' + linkt + '<a>';
}

function rgbToHex(rgb) {
  if (rgb.length == 3) {
    return "#" +
      ("0" + parseInt(rgb[0], 10).toString(16)).slice(-2) +
      ("0" + parseInt(rgb[1], 10).toString(16)).slice(-2) +
      ("0" + parseInt(rgb[2], 10).toString(16)).slice(-2);
  }
  else
    debugMessage('rgbToHex: Given rgb is no array or has wrong length');
}

function hexToRgb(hex) {
  var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : {
    r: 0,
    g: 0,
    b: 0
  };
}

/*
  Show a notification
  @param type     Valid types are "info","success","warning","danger"
  @param message  The message to show
  @param title     A title (optional)
  @param addhtml   Add custom html to the notification end
 */
function showNotification(type, message, title = "", addhtml = "") {
  if (title == "") {
    switch (type) {
      case "info":
        title = $.i18n('infoDialog_general_info_title');
        break;
      case "success":
        title = $.i18n('infoDialog_general_success_title');
        break;
      case "warning":
        title = $.i18n('infoDialog_general_warning_title');
        break;
      case "danger":
        title = $.i18n('infoDialog_general_error_title');
        break;
    }
  }

  $.notify({
    // options
    title: title,
    message: message
  }, {
    // settings
    type: type,
    animate: {
      enter: 'animate__animated animate__fadeInDown',
      exit: 'animate__animated animate__fadeOutUp'
    },
    placement: {
      align: 'center'
    },
    mouse_over: 'pause',
    template: '<div data-notify="container" class="bg-w col-md-6 bs-callout bs-callout-{0}" role="alert">' +
      '<button type="button" aria-hidden="true" class="close" data-notify="dismiss">×</button>' +
      '<span data-notify="icon"></span> ' +
      '<h4 data-notify="title">{1}</h4> ' +
      '<span data-notify="message">{2}</span>' +
      addhtml +
      '<div class="progress" data-notify="progressbar">' +
      '<div class="progress-bar progress-bar-{0}" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100" style="width: 0%;"></div>' +
      '</div>' +
      '<a href="{3}" target="{4}" data-notify="url"></a>' +
      '</div>'
  });
}

function createCP(id, color, cb) {
  if (Array.isArray(color))
    color = rgbToHex(color);
  else if (color == "undefined")
    color = "#AA3399";

  if (color.startsWith("#")) {
    $('#' + id).colorpicker({
      format: 'rgb',
      customClass: 'colorpicker-2x',
      color: color,
      sliders: {
        saturation: {
          maxLeft: 200,
          maxTop: 200
        },
        hue: {
          maxTop: 200
        },
      }
    });
    $('#' + id).colorpicker().on('changeColor', function (e) {
      var rgb = e.color.toRGB();
      var hex = e.color.toHex();
      cb(rgb, hex, e);
    });
  }
  else
    debugMessage('createCP: Given color is not legit');
}

// Creates a table with thead and tbody ids
// @param string hid  : a class for thead
// @param string bid  : a class for tbody
// @param string cont : a container id to html() the table
// @param string bless: if true the table is borderless
function createTable(hid, bid, cont, bless, tclass) {
  var table = document.createElement('table');
  var thead = document.createElement('thead');
  var tbody = document.createElement('tbody');

  table.className = "table";
  if (bless === true)
    table.className += " borderless";
  if (typeof tclass !== "undefined")
    table.className += " " + tclass;
  table.style.marginBottom = "0px";
  if (hid != "")
    thead.className = hid;
  tbody.className = bid;
  if (hid != "")
    table.appendChild(thead);
  table.appendChild(tbody);

  $('#' + cont).append(table);
}

// Creates a table row <tr>
// @param array list :innerHTML content for <td>/<th>
// @param bool head  :if null or false it's body
// @param bool align :if null or false no alignment
//
// @return : <tr> with <td> or <th> as child(s)
function createTableRow(list, head, align) {
  var row = document.createElement('tr');

  for (var i = 0; i < list.length; i++) {
    if (head === true)
      var el = document.createElement('th');
    else
      var el = document.createElement('td');

    if (align)
      el.style.verticalAlign = "middle";

    el.innerHTML = list[i];
    row.appendChild(el);
  }
  return row;
}

function createRow(id) {
  var el = document.createElement('div');
  el.className = "row";
  el.setAttribute('id', id);
  return el;
}

function createOptPanel(phicon, phead, bodyid, footerid, css, panelId) {
  phead = '<i class="fa ' + phicon + ' fa-fw"></i>' + phead;

  var pfooter = document.createElement('button');
  pfooter.className = "btn btn-primary";
  pfooter.setAttribute("id", footerid);
  pfooter.innerHTML = '<i class="fa fa-fw fa-save"></i>' + $.i18n('general_button_savesettings');

  return createPanel(phead, "", pfooter, "panel-default", bodyid, css, panelId);
}

function compareTwoValues(key1, key2, order = 'asc') {
  return function innerSort(a, b) {
    if (!a.hasOwnProperty(key1) || !b.hasOwnProperty(key1)) {
      // property key1 doesn't exist on either object
      return 0;
    }

    const varA1 = (typeof a[key1] === 'string')
      ? a[key1].toUpperCase() : a[key1];
    const varB1 = (typeof b[key1] === 'string')
      ? b[key1].toUpperCase() : b[key1];

    let comparison = 0;
    if (varA1 > varB1) {
      comparison = 1;
    } else {
      if (varA1 < varB1) {
        comparison = -1;
      } else {
        if (!a.hasOwnProperty(key2) || !b.hasOwnProperty(key2)) {
          // property key2 doesn't exist on either object
          return 0;
        }

        const varA2 = (typeof a[key2] === 'string')
          ? a[key2].toUpperCase() : a[key2];
        const varB2 = (typeof b[key1] === 'string')
          ? b[key2].toUpperCase() : b[key2];

        if (varA2 > varB2) {
          comparison = 1;
        } else {
          comparison = -1;
        }
      }
    }
    return (
      (order === 'desc') ? (comparison * -1) : comparison
    );
  };
}

function sortProperties(list) {
  for (var key in list) {
    list[key].key = key;
  }
  list = $.map(list, function (value, index) {
    return [value];
  });
  return list.sort(function (a, b) {
    return a.propertyOrder - b.propertyOrder;
  });
}

function createHelpTable(list, phead, panelId) {
  var table = document.createElement('table');
  var thead = document.createElement('thead');
  var tbody = document.createElement('tbody');
  list = sortProperties(list);

  phead = '<i class="fa fa-fw fa-info-circle"></i>' + phead + ' ' + $.i18n("conf_helptable_expl");

  table.className = 'table table-hover borderless';

  thead.appendChild(createTableRow([$.i18n('conf_helptable_option'), $.i18n('conf_helptable_expl')], true, false));

  for (var key in list) {
    if (list[key].access != 'system') {
      // break one iteration (in the loop), if the schema has the entry hidden=true
      if ("options" in list[key] && "hidden" in list[key].options && (list[key].options.hidden))
        continue;
      if ("access" in list[key] && ((list[key].access == "advanced" && storedAccess == "default") || (list[key].access == "expert" && storedAccess != "expert")))
        continue;
      var text = list[key].title.replace('title', 'expl');
      tbody.appendChild(createTableRow([$.i18n(list[key].title), $.i18n(text)], false, false));

      if (list[key].items && list[key].items.properties) {
        var ilist = sortProperties(list[key].items.properties);
        for (var ikey in ilist) {
          // break one iteration (in the loop), if the schema has the entry hidden=true
          if ("options" in ilist[ikey] && "hidden" in ilist[ikey].options && (ilist[ikey].options.hidden))
            continue;
          if ("access" in ilist[ikey] && ((ilist[ikey].access == "advanced" && storedAccess == "default") || (ilist[ikey].access == "expert" && storedAccess != "expert")))
            continue;
          var itext = ilist[ikey].title.replace('title', 'expl');
          tbody.appendChild(createTableRow([$.i18n(ilist[ikey].title), $.i18n(itext)], false, false));
        }
      }
    }
  }
  table.appendChild(thead);
  table.appendChild(tbody);

  return createPanel(phead, table, undefined, undefined, undefined, undefined, panelId);
}

function createPanel(head, body, footer, type, bodyid, css, panelId) {
  var cont = document.createElement('div');
  var p = document.createElement('div');
  var phead = document.createElement('div');
  var pbody = document.createElement('div');
  var pfooter = document.createElement('div');

  cont.className = "col-lg-6";

  if (typeof type == 'undefined')
    type = 'panel-default';

  p.className = 'panel ' + type;
  if (typeof panelId != 'undefined') {
    p.setAttribute("id", panelId);
  }

  phead.className = 'panel-heading ' + css;
  pbody.className = 'panel-body';
  pfooter.className = 'panel-footer';

  phead.innerHTML = head;

  if (typeof bodyid != 'undefined') {
    pfooter.style.textAlign = 'right';
    pbody.setAttribute("id", bodyid);
  }

  if (typeof body != 'undefined' && body != "")
    pbody.appendChild(body);

  if (typeof footer != 'undefined')
    pfooter.appendChild(footer);

  p.appendChild(phead);
  p.appendChild(pbody);

  if (typeof footer != 'undefined') {
    pfooter.style.textAlign = "right";
    p.appendChild(pfooter);
  }

  cont.appendChild(p);

  return cont;
}

function createSelGroup(group) {
  var el = document.createElement('optgroup');
  el.setAttribute('label', group);
  return el;
}

function createSelOpt(opt, title) {
  var el = document.createElement('option');
  el.setAttribute('value', opt);
  if (typeof title == 'undefined')
    el.innerHTML = opt;
  else
    el.innerHTML = title;
  return el;
}

function createSel(array, group, split) {
  if (array.length != 0) {
    var el = createSelGroup(group);
    for (var i = 0; i < array.length; i++) {
      var opt;
      if (split) {
        opt = array[i].split(":")
        opt = createSelOpt(opt[0], opt[1])
      }
      else
        opt = createSelOpt(array[i])
      el.appendChild(opt);
    }
    return el;
  }
}

function performTranslation() {
  $('[data-i18n]').i18n();
}

function encode_utf8(s) {
  return unescape(encodeURIComponent(s));
}

function getReleases(callback) {
  $.ajax({
    url: window.gitHubReleaseApiUrl,
    method: 'get',
    error: function (XMLHttpRequest, textStatus, errorThrown) {
      callback(false);
    },
    success: function (releases) {
      window.gitHubVersionList = releases;
      var highestRelease = {
        tag_name: '0.0.0'
      };
      var highestAlphaRelease = {
        tag_name: '0.0.0'
      };
      var highestBetaRelease = {
        tag_name: '0.0.0'
      };
      var highestRcRelease = {
        tag_name: '0.0.0'
      };

      for (var i in releases) {
        //drafts will be ignored
        if (releases[i].draft)
          continue;

        if (releases[i].tag_name.includes('alpha')) {
          if (sem = semverLite.gt(releases[i].tag_name, highestAlphaRelease.tag_name))
            highestAlphaRelease = releases[i];
        }
        else if (releases[i].tag_name.includes('beta')) {
          if (sem = semverLite.gt(releases[i].tag_name, highestBetaRelease.tag_name))
            highestBetaRelease = releases[i];
        }
        else if (releases[i].tag_name.includes('rc')) {
          if (semverLite.gt(releases[i].tag_name, highestRcRelease.tag_name))
            highestRcRelease = releases[i];
        }
        else {
          if (semverLite.gt(releases[i].tag_name, highestRelease.tag_name))
            highestRelease = releases[i];
        }
      }
      window.latestStableVersion = highestRelease;
      window.latestBetaVersion = highestBetaRelease;
      window.latestAlphaVersion = highestAlphaRelease;
      window.latestRcVersion = highestRcRelease;

      if (window.serverConfig.general.watchedVersionBranch == "Beta" && semverLite.gt(highestBetaRelease.tag_name, highestRelease.tag_name))
        window.latestVersion = highestBetaRelease;
      else
        window.latestVersion = highestRelease;

      if (window.serverConfig.general.watchedVersionBranch == "Alpha" && semverLite.gt(highestAlphaRelease.tag_name, highestBetaRelease.tag_name))
        window.latestVersion = highestAlphaRelease;

      if (window.serverConfig.general.watchedVersionBranch == "Alpha" && semverLite.lt(highestAlphaRelease.tag_name, highestBetaRelease.tag_name))
        window.latestVersion = highestBetaRelease;

      //next two if statements are only necessary if we don't have a beta or stable release. We need one alpha release at least
      if (window.latestVersion.tag_name == '0.0.0' && highestBetaRelease.tag_name != '0.0.0')
        window.latestVersion = highestBetaRelease;

      if (window.latestVersion.tag_name == '0.0.0' && highestAlphaRelease.tag_name != '0.0.0')
        window.latestVersion = highestAlphaRelease;

      callback(true);
    }
  });
}

function getSystemInfo() {
  var sys = window.sysInfo.system;
  var shy = window.sysInfo.hyperion;

  var info = "Hyperion Server:\n";
  info += '- Build:             ' + shy.build + '\n';
  info += '- Build time:        ' + shy.time + '\n';
  info += '- Git Remote:        ' + shy.gitremote + '\n';
  info += '- Version:           ' + shy.version + '\n';
  info += '- UI Lang:           ' + storedLang + ' (BrowserLang: ' + navigator.language + ')\n';
  info += '- UI Access:         ' + storedAccess + '\n';
  //info += '- Log lvl:           ' + window.serverConfig.logger.level + '\n';
  info += '- Avail Screen Cap.: ' + window.serverInfo.grabbers.screen.available + '\n';
  info += '- Avail Video  Cap.: ' + window.serverInfo.grabbers.video.available + '\n';
  info += '- Avail Services:    ' + window.serverInfo.services + '\n';
  info += '- Config path:       ' + shy.rootPath + '\n';
  info += '- Database:          ' + (shy.readOnlyMode ? "ready-only" : "read/write") + '\n';

  info += '\n';

  info += 'Hyperion Server OS:\n';
  info += '- Distribution:      ' + sys.prettyName + '\n';
  info += '- Architecture:      ' + sys.architecture + '\n';

  if (sys.cpuModelName)
    info += '- CPU Model:         ' + sys.cpuModelName + '\n';
  if (sys.cpuModelType)
    info += '- CPU Type:          ' + sys.cpuModelType + '\n';
  if (sys.cpuRevision)
    info += '- CPU Revision:      ' + sys.cpuRevision + '\n';
  if (sys.cpuHardware)
    info += '- CPU Hardware:      ' + sys.cpuHardware + '\n';

  info += '- Kernel:            ' + sys.kernelType + ' (' + sys.kernelVersion + ' (WS: ' + sys.wordSize + '))\n';
  info += '- Root/Admin:        ' + sys.isUserAdmin + '\n';
  info += '- Qt Version:        ' + sys.qtVersion + '\n';
  if (jQuery.inArray("effectengine", window.serverInfo.services) !== -1) {
    info += '- Python Version:    ' + sys.pyVersion + '\n';
  }
  info += '- Browser:           ' + navigator.userAgent;
  return info;
}

function handleDarkMode() {
  $("<link/>", {
    rel: "stylesheet",
    type: "text/css",
    href: "../css/darkMode.css"
  }).appendTo("head");

  setStorage("darkMode", "on");
  $('#btn_darkmode_icon').removeClass('fa fa-moon-o');
  $('#btn_darkmode_icon').addClass('mdi mdi-white-balance-sunny');
  $('#navbar_brand_logo').attr("src", 'img/hyperion/logo_negativ.png');
}

function isAccessLevelCompliant(accessLevel) {
  var isOK = true;
  if (accessLevel) {
    if (accessLevel === 'system') {
      isOK = false;
    }
    else if (accessLevel === 'advanced' && storedAccess === 'default') {
      isOK = false;
    }
    else if (accessLevel === 'expert' && storedAccess !== 'expert') {
      isOK = false;
    }
  }
  return isOK
}

function showInputOptions(path, elements, state) {
  for (var i = 0; i < elements.length; i++) {
    $('[data-schemapath="root.' + path + '.' + elements[i] + '"]').toggle(state);
  }
}

function showInputOptionForItem(editor, path, item, state) {
  var accessLevel = editor.schema.properties[path].properties[item].access;
  // Enable element only, if access level compliant
  if (!state || isAccessLevelCompliant(accessLevel)) {
    showInputOptions(path, [item], state);
  }
}

function showInputOptionsForKey(editor, item, showForKeys, state) {
  var elements = [];
  var keysToshow = [];

  if (Array.isArray(showForKeys)) {
    keysToshow = showForKeys;
  } else {
    if (typeof showForKeys === 'string') {
      keysToshow.push(showForKeys);
    } else {
      return
    }
  }

  for (var key in editor.schema.properties[item].properties) {
    if ($.inArray(key, keysToshow) === -1) {
      var accessLevel = editor.schema.properties[item].properties[key].access;

      //Always disable all elements, but only enable elements, if access level compliant
      if (!state || isAccessLevelCompliant(accessLevel)) {
        elements.push(key);
      }
    }
  }
  showInputOptions(item, elements, state);
}

function encodeHTML(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/"/g, '&quot;');
}

function isValidIPv4(value) {
  const parts = value.split('.')
  if (parts.length !== 4) {
    return false;
  }
  for (let part of parts) {
    if (isNaN(part) || part < 0 || part > 255) {
      return false;
    }
  }
  return true;
}

function isValidIPv6(value) {
  if (value.match(
    '^(?:(?:(?:[a-fA-F0-9]{1,4}:){6}|(?=(?:[a-fA-F0-9]{0,4}:){2,6}(?:[0-9]{1,3}.){3}[0-9]{1,3}$)(([0-9a-fA-F]{1,4}:){1,5}|:)((:[0-9a-fA-F]{1,4}){1,5}:|:)|::(?:[a-fA-F0-9]{1,4}:){5})(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9]).){3}(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])|(?:[a-fA-F0-9]{1,4}:){7}[a-fA-F0-9]{1,4}|(?=(?:[a-fA-F0-9]{0,4}:){0,7}[a-fA-F0-9]{0,4}$)(([0-9a-fA-F]{1,4}:){1,7}|:)((:[0-9a-fA-F]{1,4}){1,7}|:)|(?:[a-fA-F0-9]{1,4}:){7}:|:(:[a-fA-F0-9]{1,4}){7})$'
  ))
    return true;
  else
    return false;
}

function isValidHostname(value) {
  if (value.match(
    '^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[_a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$'
  ))
    return true;
  else
    return false;
}

function isValidServicename(value) {
  if (value.match(
    '^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9 \-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[_a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*$'
  ))
    return true;
  else
    return false;
}

function isValidHostnameOrIP4(value) {
  return (isValidHostname(value) || isValidIPv4(value));
}

function isValidHostnameOrIP(value) {
  return (isValidHostnameOrIP4(value) || isValidIPv6(value) || isValidServicename(value));
}

