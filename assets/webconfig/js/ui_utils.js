const DURATION_ENDLESS = -1;

let prevTag;

function removeOverlay() {
  $("#loading_overlay").removeClass("overlay");
}

function reload() {
  location.reload();
}

function storageComp() {
  return typeof (Storage) !== "undefined";
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
  if (typeof d === "undefined" || d <= 0) {
    return DURATION_ENDLESS;
  } else {
    return d * 1000;
  }
}

function getHashtag() {
  const lastHashtag = getStorage('lasthashtag');
  if (lastHashtag !== null) {
    return lastHashtag;
  } else {
    let tag = document.URL;
    const hashIndex = tag.indexOf("#");
    if (hashIndex !== -1) {
      tag = tag.slice(hashIndex + 1);
    } else {
      tag = ""; // No hashtag found
    }

    if (tag === "" || typeof tag === "undefined" || tag.startsWith("http")) {
      tag = "dashboard";
    }

    return tag;
  }
}

function isInstanceRunning(instanceId) {
  return window.serverInfo?.instance?.some(
    (instance) => instance.instance === Number(instanceId) && instance.running
  );
}

function isCurrentInstanceRunning() {
  return isInstanceRunning(window.currentHyperionInstance);
}

function getFirstRunningInstance() {

  const runningInstance = window.serverInfo?.instance?.find(
    (instance) => instance.running
  );

  return runningInstance ? runningInstance.instance : null; // Return instance number or null if none is running
}

function getFirstConfiguredInstance() {
  const configuredInstance = window.serverInfo?.instance?.find(
    (instance) => instance.instance !== undefined
  );

  return configuredInstance ? configuredInstance.instance : null; // Return instance number or null if none exists
}

function doesInstanceExist(instanceId) {

  if (instanceId == null) {
    return false; // Return false if instanceId is null or undefined
  }

  return window.serverInfo?.instance?.some(
    (instance) => instance.instance === Number(instanceId)
  ) || false; // Return false if serverInfo or instance is undefined
}

function getInstanceName(instanceId) {
  const instance = window.serverInfo?.instance?.find(
    (instance) => instance.instance === Number(instanceId)
  );

  return instance?.friendly_name || "unknown";
}

function getCurrentInstanceName() {

  const instanceId = window.currentHyperionInstance;
  return getInstanceName(instanceId);
}

function loadContent(event, forceRefresh) {

  let tag;

  if (typeof event !== "undefined") {
    tag = event.currentTarget.hash;
    tag = tag.substr(tag.indexOf("#") + 1);
    setStorage('lasthashtag', tag);
  } else {
    tag = getHashtag();
  }

  // Only load content if the tag is different or forced
  if (forceRefresh || prevTag !== tag) {
    prevTag = tag;

    $("#page-content").off().empty(); // Off all events and clear the content

    $("#page-content").load("/content/" + tag + ".html", function (response, status, xhr) {
      if (status === "error") {
        tag = 'dashboard';
        console.log("Could not find page:", prevTag, ", Redirecting to:", tag);
        setStorage('lasthashtag', tag);
        $("#page-content").html('<h3>' + encode_utf8(tag) + '<br/>' + $.i18n('info_404') + '</h3>');
        removeOverlay();
      } else {
        updateUiOnInstance(window.currentHyperionInstance);
      }
    });
  }
}

function updateHyperionInstanceListing() {

  const data = window.serverInfo.instance;
  if (data) {
    const instances = Object.values(data);
    // Sort instances by friendly_name (case-insensitive)
    instances.sort((a, b) => a.friendly_name.toLowerCase().localeCompare(b.friendly_name.toLowerCase()));

    $('#hyp_inst_listing').html("");

    instances.forEach((instance, index) => {
      const isRunningMarker = isInstanceRunning(instance.instance) ? "component-on" : "";

      const html = `
        <li id="hyperioninstance_${instance.instance}">
          <a>
            <div>
              <i class="fa fa-circle fa-fw ${isRunningMarker}"></i>
              <span>${instance.friendly_name}</span>
            </div>
          </a>
        </li>
        ${index < instances.length - 1 ? '<li class="divider"></li>' : ''}
      `;

      $('#hyp_inst_listing').append(html);

      $(`#hyperioninstance_${instance.instance}`).off().on("click", (e) => {
        const inst = e.currentTarget.id.split("_")[1];
        instanceSwitch(inst);
      });
    });
  }
}

function initLanguageSelection() {
  const $select = $('#language-select');
  $select.empty(); // clear existing options

  for (let i = 0; i < availLang.length; i++) {
    $select.append('<option value="' + availLang[i] + '">' + availLangText[i] + '</option>');
  }

  let langLocale = storedLang;
  if (!langLocale) {
    langLocale = navigator.language?.substring(0, 2) || 'en';
  }

  let langIdx = availLang.indexOf(langLocale);
  if (langIdx === -1) {
    // Try fallback
    langLocale = $.i18n().options.fallbackLocale.substring(0, 2);
    langIdx = availLang.indexOf(langLocale);
  }

  if (langIdx === -1) {
    // Default to English
    langLocale = 'en';
  }

  // Update the language select dropdown
  $select.val(langLocale); 
  $select.selectpicker({
    container: 'body',
    width: 'fit',
    style: 'btn-transparent'
  });
  $select.selectpicker('refresh');
}

function updateUiOnInstance(inst) {

  window.currentHyperionInstance = inst;
  if (inst === null) {
    //No instance defined, hide all instance related menue items
    $("#MenuItemLedInstances").closest("li").hide();
    $("#MenuItemRemoteControl, #MenuItemEffectsConfig, #NavMenuWizards, #btn_open_ledsim, #btn_streamer").hide();
  } else {
    window.currentHyperionInstanceName = getInstanceName(inst);

    $("#active_instance_friendly_name").text(getInstanceName(inst));
    $('#btn_hypinstanceswitch').toggle(true);
    $('#active_instance_dropdown').prop('disabled', false);
    $('#active_instance_dropdown').css('cursor', 'pointer');
    $("#active_instance_dropdown").css("pointer-events", "auto");

    //Allow to configure an existing instance
    $("#MenuItemLedInstances").show().closest("li").show();

    // Show menue items according to instance's running state
    if (isInstanceRunning(window.currentHyperionInstance)) {
      $("#MenuItemRemoteControl, #NavMenuWizards, #btn_open_ledsim").show();

      //Show effectsconfigurator menu entry, only if effectengine is available
      if (jQuery.inArray("effectengine", window.serverInfo.services) !== -1) {
        $("#MenuItemEffectsConfig").show();
      }

      const isMediaStreamingSupported = getStorage('mediaStreamingSupported');
      if (isMediaStreamingSupported) {
        $('#btn_streamer').show();
      }

    } else {
      $("#MenuItemRemoteControl, #MenuItemEffectsConfig, #NavMenuWizards, #btn_open_ledsim, #btn_streamer").hide();
    }
  }
}

function instanceSwitch(inst) {
  const instanceID = Number(inst);
  requestInstanceSwitch(instanceID)
  window.currentHyperionInstance = instanceID;
  window.currentHyperionInstanceName = getInstanceName(instanceID);
  setStorage('lastSelectedInstance', instanceID)
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

function showInfoDialog(type, header = "", message = "", details = []) {
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
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-error"></i>');
    if (header == "") {
      $('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">' + $.i18n('infoDialog_general_error_title') + '</h4>');
    }
    $('#id_footer').html('<button type="button" class="btn btn-danger" data-dismiss-modal="#modal_dialog">' + $.i18n('general_btn_ok') + '</button>');
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
    $('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning"></i>');
    $('#id_footer').html('<button type="button" id="id_btn_import" class="btn btn-warning"><i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_saverestart') + '</button>');
    $('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss-modal="#modal_dialog"><i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>');
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

    // Start form wrapper
    $('#id_body_rename').append('<form id="changePasswordForm"; return false;">');
    $('#changePasswordForm').append(
      '<div class="row">' +
      '<div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_username_text') + '</p></div>' +
      '<div class="col-md-8"><input class="form-control" id="username" type="text" value="Hyperion" disabled autocomplete="username"></div>' +
      '</div><br>'
    );
    $('#changePasswordForm').append(
      '<div class="row">' +
      '<div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_password_current_text') + '</p></div>' +
      '<div class="col-md-8"><input class="form-control" id="current-password" placeholder="Old" type="password" autocomplete="current-password"></div>' +
      '</div><br>'
    );
    $('#changePasswordForm').append(
      '<div class="row">' +
      '<div class="col-md-4"><p class="text-left">' + $.i18n('infoDialog_password_new_text') + '</p></div>' +
      '<div class="col-md-8"><input class="form-control" id="new-password" placeholder="New" type="password" autocomplete="new-password"></div>' +
      '</div>'
    );
    $('#changePasswordForm').append(
      '<div class="bs-callout bs-callout-info"><span>' + $.i18n('infoDialog_password_minimum_length') + '</span></div>'
    );
    $('#changePasswordForm').append('</form>');

    // Footer buttons
    $('#id_footer_rename').html(
      '<button type="submit" form="changePasswordForm" id="id_btn_ok" class="btn btn-success" data-dismiss-modal="#modal_dialog_rename" disabled>' +
      '<i class="fa fa-fw fa-save"></i>' + $.i18n('general_btn_ok') + '</button>'
    );
    $('#id_footer_rename').append(
      '<button type="button" class="btn btn-danger" data-dismiss="modal">' +
      '<i class="fa fa-fw fa-close"></i>' + $.i18n('general_btn_cancel') + '</button>'
    );
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

  // Append details if available
  if (Array.isArray(details) && details.length > 0) {

    // Create a container div for additional details with proper styles
    const detailsContent = $('<div></div>').css({
      'text-align': 'left',
      'white-space': 'pre-wrap',     // Ensures newlines are respected
      'word-wrap': 'break-word',     // Prevents long words from overflowing
      'margin-top': '15px'
    });

    detailsContent.append('<hr>');
    details.forEach((desc, index) => {
      detailsContent.append(document.createTextNode(`${index + 1}. ${desc}\n`));
    });
    $('#id_body').append(detailsContent);
  }

  if (getStorage("darkMode") == "on")
    $('#id_logo').attr("src", 'img/hyperion/logo_negativ.png');

  $(type == "renInst" || type == "changePassword" ? "#modal_dialog_rename" : "#modal_dialog").modal({
    backdrop: "static",
    keyboard: false,
    show: true
  });

  $(document).on('click', '[data-dismiss-modal]', function () {
    const target = $(this).data('dismiss-modal');
    $($.find(target)).modal('hide');
  });
}

function createHintH(type, text, container) {
  type = String(type);

  let tclass;
  if (type == "intro") {
    tclass = "introd";
  }

  // Prepend the formatted hint to the container
  $('#' + container).prepend(
    '<div class="' + tclass + '">' +
    '<h4 style="font-size:16px">' + text + '</h4>' +
    '<hr/>' +
    '</div>'
  );
}

function createHint(type, text, container, buttonid) {
  let fe = '';
  let tclass = '';
  let buttonHtml = '';

  // Set up icon HTML and hint class based on type
  switch (type) {
    case 'intro':
      tclass = 'intro-hint';
      break;
    case 'info':
      fe = `
        <div style="font-size:25px;text-align:center">
          <i class="fa fa-info"></i>
        </div>
        <div style="text-align:center;font-size:13px">Information</div>`;
      tclass = 'info-hint';
      break;
    case 'wizard':
      fe = `
        <div style="font-size:25px;text-align:center">
          <i class="fa fa-magic"></i>
        </div>
        <div style="text-align:center;font-size:13px">Information</div>`;
      tclass = 'wizard-hint';
      break;
    case 'warning':
      fe = `
        <div style="font-size:25px;text-align:center">
          <i class="fa fa-info"></i>
        </div>
        <div style="text-align:center;font-size:13px">Information</div>`;
      tclass = 'warning-hint';
      break;
    default:
      tclass = 'info-hint'; // Default to info-hint if no match
  }

  // Create button HTML if buttonid is provided
  if (buttonid) {
    buttonHtml = `
      <p>
        <button id="${buttonid}" class="btn btn-wizard" style="margin-top:15px;">
          ${text}
        </button>
      </p>`;
  }

  // Add hint to the container based on type
  switch (type) {
    case 'intro':
      $('#' + container).prepend(`
        <div class="bs-callout bs-callout-primary" style="margin-top:0px">
          <h4>${$.i18n("conf_helptable_expl")}</h4>
          ${text}
        </div>`);
      break;
    case 'wizard':
      $('#' + container).prepend(`
        <div class="bs-callout bs-callout-wizard" style="margin-top:0px">
          <h4>${$.i18n("wiz_wizavail")}</h4>
          ${$.i18n('wiz_guideyou', text)}
          ${buttonHtml}
        </div>`);
      break;
    default:
      createTable('', 'htb', container, true, tclass);
      $('#' + container + ' .htb').append(createTableRow([fe, text], false, true));
  }
}


function createEffHint(title, text) {
  return `
    <div class="bs-callout bs-callout-primary" style="margin-top:0px">
      <h4>${title}</h4>
      ${text}
    </div>
  `;
}

function valValue(id, value, min, max) {
  // Default max to 999999 if it's undefined or an empty string
  max = (max === undefined || max === "") ? 999999 : Number(max);

  const numericValue = Number(value);
  const numericMin = Number(min);

  if (numericValue > max) {
    $('#' + id).val(max);
    showInfoDialog("warning", "", $.i18n('edt_msg_error_maximum_incl', max));
    return max;
  }

  if (numericValue < numericMin) {
    $('#' + id).val(numericMin);
    showInfoDialog("warning", "", $.i18n('edt_msg_error_minimum_incl', numericMin));
    return numericMin;
  }

  return numericValue;
}

function readImg(input, callback) {
  const file = input.files?.[0];

  if (file) {
    const reader = new FileReader();
    reader.fileName = file.name;

    // Handle file load
    reader.onload = (e) => {
      callback(e.target.result, e.target.fileName);
    };

    reader.readAsDataURL(file);
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

const getObjectProperty = (obj, path) => path.split(".").reduce((o, key) => o && typeof o[key] !== 'undefined' ? o[key] : undefined, obj);

const setObjectProperty = (object, path, value) => {
  const parts = path.split('.');
  const limit = parts.length - 1;
  for (let i = 0; i < limit; ++i) {
    const key = parts[i];
    if (key === "__proto__" || key === "constructor") continue;
    object = object[key] ?? (object[key] = {});
  }
  const key = parts[limit];
  object[key] = value;
};

function getLongPropertiesPath(path) {
  if (path) {
    // Remove 'root.' from the start of the path
    path = path.replace('root.', '');

    // Split the path into parts and append ".properties" to each part
    const parts = path.split('.');
    parts.forEach(function (part, index) {
      parts[index] += ".properties";
    });

    // Join the parts back together and append a final '.'
    path = parts.join('.') + '.';
  }
  return path;
}

function createJsonEditor(container, schema, setconfig, usePanel, arrayre) {
  $('#' + container).off();
  $('#' + container).html("");

  if (typeof arrayre === 'undefined')
    arrayre = true;

  let editor = new JSONEditor(document.getElementById(container),
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
    for (let key in editor.root.editors) {
      editor.getEditor("root." + key).setValue({ ...editor.getEditor("root." + key).value, ...window.serverConfig[key] });
    }
  }

  return editor;
}

// Update the selection for JSON Editor
function updateJsonEditorSelection(
  rootEditor, path, key, addElements, newEnumVals, newTitleVals, newDefaultVal,
  addSelect, addCustom, addCustomAsFirst, customText = "edt_conf_enum_custom"
) {
  const editor = rootEditor.getEditor(path);
  const originalProperties = editor.schema.properties[key];
  const originalWatchFunctions = rootEditor.watchlist[path + "." + key];

  // Unwatch the existing path
  rootEditor.unwatch(path + "." + key);

  const newSchema = {
    [key]: {
      type: "string",
      enum: [],
      required: true,
      options: { enum_titles: [], infoText: "" },
      propertyOrder: 1,
      ...addElements, // Merge custom elements directly into schema
    }
  };

  // Retain original properties if available
  if (originalProperties) {
    const { title, options, propertyOrder } = originalProperties;
    newSchema[key].title = title || newSchema[key].title;
    newSchema[key].options.infoText = options?.infoText || newSchema[key].options.infoText;
    newSchema[key].propertyOrder = propertyOrder || newSchema[key].propertyOrder;
  }

  // Handle custom values
  if (addCustom) {
    if (newTitleVals.length === 0) newTitleVals = [...newEnumVals];

    const customPosition = addCustomAsFirst ? "unshift" : "push";
    newEnumVals[customPosition]("CUSTOM");
    newTitleVals[customPosition](customText);

    // Append custom infoText if exists
    if (newSchema[key].options.infoText) {
      newSchema[key].options.infoText += "_custom";
    }
  }

  // Handle Select options
  if (addSelect) {
    newEnumVals.unshift("SELECT");
    newTitleVals.unshift("edt_conf_enum_please_select");
    newDefaultVal = "SELECT";
  }

  // Set new values
  if (newEnumVals) newSchema[key].enum = newEnumVals;
  if (newTitleVals) newSchema[key].options.enum_titles = newTitleVals;
  if (newDefaultVal) newSchema[key].default = newDefaultVal;

  // Update the editor schema
  editor.original_schema.properties[key] = originalProperties;
  editor.schema.properties[key] = newSchema[key];

  // Update schema for validation
  setObjectProperty(rootEditor.validator.schema.properties, getLongPropertiesPath(path) + key, newSchema[key]);

  // Re-apply changes to the editor
  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  // Reapply original watch functions
  if (originalWatchFunctions) {
    originalWatchFunctions.forEach(element => rootEditor.watch(path + "." + key, element));
  }

  // Notify watchers
  rootEditor.notifyWatchers(path + "." + key);
}


// Handle custom values logic for enum and title values
function handleCustomValues(newEnumVals, newTitleVals, customText, addCustomAsFirst) {
  if (newTitleVals.length === 0) {
    newTitleVals = [...newEnumVals];
  }

  if (!customText) {
    customText = "edt_conf_enum_custom";
  }

  if (addCustomAsFirst) {
    newEnumVals.unshift("CUSTOM");
    newTitleVals.unshift(customText);
  } else {
    newEnumVals.push("CUSTOM");
    newTitleVals.push(customText);
  }

  // Add infoText for custom options
  if (newSchema[key].options.infoText) {
    const customInfoText = newSchema[key].options.infoText + "_custom";
    newSchema[key].options.infoText = customInfoText;
  }
}

// Update the JSON Editor for multi-selection fields
function updateJsonEditorMultiSelection(rootEditor, path, key, addElements, newEnumVals, newTitleVals, newDefaultVal) {
  const editor = rootEditor.getEditor(path);
  const originalProperties = editor.schema.properties[key];
  const originalWatchFunctions = rootEditor.watchlist[path + "." + key];

  // Unwatch the existing path
  rootEditor.unwatch(path + "." + key);

  const newSchema = {
    [key]: {
      type: "array",
      format: "select",
      items: {
        type: "string",
        enum: [],
        options: { enum_titles: [] }
      },
      options: { infoText: "" },
      default: [],
      propertyOrder: 1
    }
  };

  // Overwrite default properties with additional elements
  Object.assign(newSchema[key], addElements);

  // Retain original properties
  if (originalProperties) {
    if (originalProperties.title) newSchema[key].title = originalProperties.title;
    if (originalProperties?.options?.infoText) newSchema[key].options.infoText = originalProperties.options.infoText;
    if (originalProperties.propertyOrder) newSchema[key].propertyOrder = originalProperties.propertyOrder;
  }

  // Set new enum and title values
  if (newEnumVals) newSchema[key].items.enum = newEnumVals;
  if (newTitleVals) newSchema[key].items.options.enum_titles = newTitleVals;
  if (newDefaultVal) newSchema[key].default = newDefaultVal;

  // Update the editor schema
  editor.original_schema.properties[key] = originalProperties;
  editor.schema.properties[key] = newSchema[key];

  // Update schema for validation
  setObjectProperty(rootEditor.validator.schema.properties, getLongPropertiesPath(path) + key, newSchema[key]);

  // Re-apply changes to the editor
  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  // Reapply original watch functions
  if (originalWatchFunctions) {
    originalWatchFunctions.forEach((element) => {
      rootEditor.watch(path + "." + key, element);
    });
  }

  // Notify watchers
  rootEditor.notifyWatchers(path + "." + key);
}

// Update JSON Editor Range with min, max, and step values
function updateJsonEditorRange(rootEditor, path, key, minimum, maximum, defaultValue, step, clear) {
  const editor = rootEditor.getEditor(path);
  const currentValue = rootEditor.getEditor(path + "." + key).getValue();
  const originalProperties = editor.schema.properties[key];

  // Initialize the new schema with original properties
  const newSchema = { [key]: { ...originalProperties } };

  // Clear range-related properties if needed
  if (clear) {
    delete newSchema[key].minimum;
    delete newSchema[key].maximum;
    delete newSchema[key].default;
    delete newSchema[key].step;
  }

  // Set the range values
  if (typeof minimum !== "undefined") newSchema[key].minimum = minimum;
  if (typeof maximum !== "undefined") newSchema[key].maximum = maximum;
  if (typeof defaultValue !== "undefined") {
    newSchema[key].default = defaultValue;
  }
  if (typeof step !== "undefined") newSchema[key].step = step;

  // Update the editor schema
  editor.original_schema.properties[key] = originalProperties;
  editor.schema.properties[key] = newSchema[key];

  // Update schema for validation
  setObjectProperty(rootEditor.validator.schema.properties, getLongPropertiesPath(path) + key, newSchema[key]);

  // Re-apply changes to the editor
  editor.removeObjectProperty(key);
  delete editor.cached_editors[key];
  editor.addObjectProperty(key);

  // restore the current value, if no default value given
  if (typeof defaultValue === "undefined") {
    rootEditor.getEditor(path + "." + key).setValue(currentValue);
  } else {
    rootEditor.getEditor(path + "." + key).setValue(defaultValue);
  }
}

// Add custom host validation to JSON Editor
function addJsonEditorHostValidation() {
  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    const errors = [];

    if (!jQuery.isEmptyObject(value)) {
      switch (schema.format) {
        case "hostname_or_ip":
          if (!isValidHostnameOrIP(value)) {
            errors.push({ path, property: 'format', message: $.i18n('edt_msgcust_error_hostname_ip') });
          }
          break;
        case "hostname_or_ip4":
          if (!isValidHostnameOrIP4(value)) {
            errors.push({ path, property: 'format', message: $.i18n('edt_msgcust_error_hostname_ip4') });
          }
          break;
        case "ipv4":
          if (!isValidIPv4(value)) {
            errors.push({ path, property: 'format', message: $.i18n('edt_msg_error_ipv4') });
          }
          break;
        case "ipv6":
          if (!isValidIPv6(value)) {
            errors.push({ path, property: 'format', message: $.i18n('edt_msg_error_ipv6') });
          }
          break;
        case "hostname":
          if (!isValidHostname(value)) {
            errors.push({ path, property: 'format', message: $.i18n('edt_msg_error_hostname') });
          }
          break;
        default:
          break;
      }
    }

    return errors;
  });
}

// Build a link with localization
function buildWL(link, linkt, cl) {
  const baseLink = "https://docs.hyperion-project.org/";
  const lang = (storedLang === "de" || navigator.locale === "de") ? "de/" : "";

  if (cl) {
    linkt = $.i18n(linkt);
    return `<div class="bs-callout bs-callout-primary"><h4>${linkt}</h4>${$.i18n('general_wiki_moreto', linkt)}: <a href="${baseLink}${lang}${link}" target="_blank">${linkt}</a></div>`;
  } else {
    return `: <a href="${baseLink}${lang}${link}" target="_blank">${linkt}</a>`;
  }
}

// Convert RGB values to Hex color
function rgbToHex(rgb) {
  if (rgb.length === 3) {
    return `#${("0" + parseInt(rgb[0], 10).toString(16)).slice(-2)}${("0" + parseInt(rgb[1], 10).toString(16)).slice(-2)}${("0" + parseInt(rgb[2], 10).toString(16)).slice(-2)}`;
  } else {
    debugMessage('rgbToHex: Given rgb is no array or has wrong length');
  }
}

// Convert Hex color to RGB
function hexToRgb(hex) {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : { r: 0, g: 0, b: 0 };
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
  // Ensure color is valid and handle cases where it's an array or undefined
  if (Array.isArray(color)) {
    color = rgbToHex(color);  // Convert array to hex
  } else if (color === "undefined") {
    color = "#AA3399";  // Default color
  }

  // Only proceed if the color is a valid hex color
  if (color.startsWith("#")) {
    // Initialize colorpicker with the given color
    $(`#${id}`).colorpicker({
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

    // Handle color change events
    $(`#${id}`).colorpicker().on('changeColor', (e) => {
      const rgb = e.color.toRGB();
      const hex = e.color.toHex();
      cb(rgb, hex, e);  // Callback with updated color values
    });
  } else {
    debugMessage('createCP: Given color is not legit');
  }
}

// Function to create a table with thead and tbody elements
// @param {string} hid - Class name for thead
// @param {string} bid - Class name for tbody
// @param {string} cont - Container ID to append the table
// @param {boolean} bless - If true, the table is borderless
// @param {string} tclass - Additional class for the table (optional)
function createTable(hid, bid, cont, bless, tclass) {
  const table = document.createElement('table');
  const thead = document.createElement('thead');
  const tbody = document.createElement('tbody');

  // Set the base class for the table
  table.className = "table";

  // If 'bless' is true, add the borderless class
  if (bless) {
    table.classList.add("borderless");
  }

  // If 'tclass' is provided, add it as a class
  if (tclass) {
    table.classList.add(tclass);
  }

  // Remove bottom margin for the table
  table.style.marginBottom = "0px";

  // Set class for thead and tbody if provided
  if (hid !== "") {
    thead.className = hid;
  }
  tbody.className = bid;

  // Append thead and tbody to the table
  if (hid !== "") {
    table.appendChild(thead);
  }
  table.appendChild(tbody);

  // Append the table to the specified container
  $(`#${cont}`).append(table);
}



// Creates a table row <tr>
// @param array list :innerHTML content for <td>/<th>
// @param bool head  :if null or false it's body
// @param bool align :if null or false no alignment
//
// @return : <tr> with <td> or <th> as child(s)
function createTableRow(list, head, align) {
  const row = document.createElement('tr');

  for (const element of list) {
    let el = head === true ? document.createElement('th') : document.createElement('td');
    if (align) {
      el.style.verticalAlign = "middle";
    }
    const purifyConfig = {
      ADD_TAGS: ['button'],
      ADD_ATTR: ['onclick']
    };
    el.innerHTML = DOMPurify.sanitize(element, purifyConfig);
    row.appendChild(el);
  }

  // Return the constructed table row
  return row;
}

function createRow(id) {
  let el = document.createElement('div');
  el.className = "row";
  el.setAttribute('id', id);
  return el;
}

function createOptPanel(phicon, phead, bodyid, footerid, css, panelId) {
  phead = '<i class="fa ' + phicon + ' fa-fw"></i>' + phead;

  let pfooter = document.createElement('button');
  pfooter.className = "btn btn-primary";
  pfooter.setAttribute("id", footerid);
  pfooter.innerHTML = '<i class="fa fa-fw fa-save"></i>' + $.i18n('general_button_savesettings');

  return createPanel(phead, "", pfooter, bodyid, css, panelId);
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
    } else if (varA1 < varB1) {
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
    return (
      (order === 'desc') ? (comparison * -1) : comparison
    );
  };
}

function sortProperties(list) {
  // Assign the key as a property for each item in the list
  for (const key in list) {
    if (Object.hasOwn(list, key)) {
      list[key].key = key;
    }
  }

  // Convert the object to an array
  const mappedList = $.map(list, function (value) {
    return [value];
  });

  // Sort the array based on the propertyOrder
  return mappedList.sort((a, b) => a.propertyOrder - b.propertyOrder);
}

function createHelpTable(list, phead, panelId) {
  const table = document.createElement('table');
  const thead = document.createElement('thead');
  const tbody = document.createElement('tbody');

  list = sortProperties(list);

  // Update the heading with an icon and the translation
  phead = '<i class="fa fa-fw fa-info-circle"></i>' + phead + ' ' + $.i18n("conf_helptable_expl");

  table.className = 'table table-hover borderless';

  // Create the table header
  thead.appendChild(createTableRow([$.i18n('conf_helptable_option'), $.i18n('conf_helptable_expl')], true, false));

  // Iterate over the list and populate the table
  for (const key in list) {
    if (list[key].access !== 'system') {
      // Skip entries if they are hidden or have access restrictions
      if ("options" in list[key] && "hidden" in list[key].options && list[key].options.hidden) {
        continue;
      }

      if ("access" in list[key] && ((list[key].access === "advanced" && storedAccess === "default") || (list[key].access === "expert" && storedAccess !== "expert"))) {
        continue;
      }

      const text = list[key].title.replace('title', 'expl');
      tbody.appendChild(createTableRow([$.i18n(list[key].title), $.i18n(text)], false, false));

      if (list[key].items?.properties) {
        const ilist = sortProperties(list[key].items.properties);
        for (const ikey in ilist) {
          // Skip items based on hidden or access restrictions
          if ("options" in ilist[ikey] && "hidden" in ilist[ikey].options && ilist[ikey].options.hidden) {
            continue;
          }

          if ("access" in ilist[ikey] && ((ilist[ikey].access === "advanced" && storedAccess === "default") || (ilist[ikey].access === "expert" && storedAccess !== "expert"))) {
            continue;
          }

          const itext = ilist[ikey].title.replace('title', 'expl');
          tbody.appendChild(createTableRow([$.i18n(ilist[ikey].title), $.i18n(itext)], false, false));
        }
      }
    }
  }

  table.appendChild(thead);
  table.appendChild(tbody);

  return createPanel(phead, table, undefined, undefined, undefined, panelId, undefined);
}

function createPanel(head, body, footer, bodyid, css, panelId, type = 'panel-default') {
  const cont = document.createElement('div');
  const p = document.createElement('div');
  const phead = document.createElement('div');
  const pbody = document.createElement('div');
  const pfooter = document.createElement('div');

  cont.className = "col-lg-6";

  p.className = `panel ${type}`;
  if (panelId) {
    p.setAttribute("id", panelId);
  }

  phead.className = `panel-heading ${css}`;
  pbody.className = 'panel-body';
  pfooter.className = 'panel-footer';

  phead.innerHTML = head;

  if (bodyid) {
    pfooter.style.textAlign = 'right';
    pbody.setAttribute("id", bodyid);
  }

  if (body) {
    pbody.appendChild(body);
  }

  if (footer) {
    pfooter.appendChild(footer);
  }

  p.appendChild(phead);
  p.appendChild(pbody);

  if (footer) {
    pfooter.style.textAlign = "right";
    p.appendChild(pfooter);
  }

  cont.appendChild(p);

  return cont;
}

function createSelGroup(group) {
  const el = document.createElement('optgroup');
  el.setAttribute('label', group);
  return el;
}

function createSelOpt(opt, title = opt) {
  const el = document.createElement('option');
  el.setAttribute('value', opt);
  el.textContent = title;
  return el;
}

function createSel(array, group, split) {
  if (array.length !== 0) {
    const el = createSelGroup(group);
    for (const element of array) {
      let opt;
      if (split) {
        const [value, label] = element.split(":");
        opt = createSelOpt(value, label);
      } else {
        opt = createSelOpt(element);
      }
      el.appendChild(opt);
    }
    return el;
  }
}


function performTranslation() {
  $('[data-i18n]').i18n();
}

function encode_utf8(s) {
  return btoa(new TextEncoder().encode(s).reduce((data, byte) => data + String.fromCharCode(byte), ''));
}

function getReleases(callback) {
  $.ajax({
    url: window.gitHubReleaseApiUrl,
    method: 'GET',
    error: function () {
      callback(false);
    },
    success: function (releases) {
      window.gitHubVersionList = releases;

      // Initialize release categories
      const defaultRelease = { tag_name: '0.0.0' };
      let highestRelease = { ...defaultRelease };
      let highestAlphaRelease = { ...defaultRelease };
      let highestBetaRelease = { ...defaultRelease };
      let highestRcRelease = { ...defaultRelease };

      // Iterate through releases
      releases.forEach((release) => {

        if (release.tag_name === "nightly") return;
        if (release.draft) return;

        if (release.tag_name.includes('alpha') && semverLite.gt(release.tag_name, highestAlphaRelease.tag_name)) {
          highestAlphaRelease = release;
        } else if (release.tag_name.includes('beta') && semverLite.gt(release.tag_name, highestBetaRelease.tag_name)) {
          highestBetaRelease = release;
        } else if (release.tag_name.includes('rc') && semverLite.gt(release.tag_name, highestRcRelease.tag_name)) {
          highestRcRelease = release;
        } else if (semverLite.gt(release.tag_name, highestRelease.tag_name)) {
          highestRelease = release;
        }
      });

      // Update global variables with the latest releases
      window.latestStableVersion = highestRelease;
      window.latestBetaVersion = highestBetaRelease;
      window.latestAlphaVersion = highestAlphaRelease;
      window.latestRcVersion = highestRcRelease;

      // Determine the latest version based on the watched branch
      const { watchedVersionBranch } = window.serverConfig.general;

      if (watchedVersionBranch === "Beta" && semverLite.gt(highestBetaRelease.tag_name, highestRelease.tag_name)) {
        window.latestVersion = highestBetaRelease;
      } else if (watchedVersionBranch === "Alpha") {
        window.latestVersion = semverLite.gt(highestAlphaRelease.tag_name, highestBetaRelease.tag_name)
          ? highestAlphaRelease
          : highestBetaRelease;
      } else {
        window.latestVersion = highestRelease;
      }

      // Fallback handling if no stable or beta release exists
      if (window.latestVersion.tag_name === '0.0.0') {
        if (highestBetaRelease.tag_name !== '0.0.0') {
          window.latestVersion = highestBetaRelease;
        } else if (highestAlphaRelease.tag_name !== '0.0.0') {
          window.latestVersion = highestAlphaRelease;
        }
      }

      // Execute the callback with success
      callback(true);
    }
  });
}

function getSystemInfo() {
  const { system: sys, hyperion: shy } = window.sysInfo;

  let info = `Hyperion Server:
- Build:             ${shy.build}
- Build time:        ${shy.time}
- Build type:        ${shy.buildType}
- Git Remote:        ${shy.gitremote}
- Version:           ${shy.version}
- UI Lang:           ${storedLang} (BrowserLang: ${navigator.language})
- UI Access:         ${storedAccess}
- Avail Screen Cap.: ${window.serverInfo.grabbers.screen.available}
- Avail Video  Cap.: ${window.serverInfo.grabbers.video.available}
- Avail Audio  Cap.: ${window.serverInfo.grabbers.audio.available}
- Avail Services:    ${window.serverInfo.services}
- Config database:   ${shy.configDatabaseFile}
- Database:          ${shy.readOnlyMode ? "read-only" : "read/write"}
- Mode:              ${shy.isGuiMode ? "GUI" : "Non-GUI"}

Hyperion Server OS:
- Distribution:      ${sys.prettyName}
- Architecture:      ${sys.architecture}`;

  if (sys.cpuModelName) info += `\n- CPU Model:         ${sys.cpuModelName}`;
  if (sys.cpuModelType) info += `\n- CPU Type:          ${sys.cpuModelType}`;
  if (sys.cpuRevision) info += `\n- CPU Revision:      ${sys.cpuRevision}`;
  if (sys.cpuHardware) info += `\n- CPU Hardware:      ${sys.cpuHardware}`;

  info += `\n- Kernel:            ${sys.kernelType} (${sys.kernelVersion} (WS: ${sys.wordSize}))
- Root/Admin:        ${sys.isUserAdmin}
- Qt Version:        ${sys.qtVersion}`;

  if (window.serverInfo.services.includes("effectengine")) {
    info += `\n- Python Version:    ${sys.pyVersion}`;
  }

  info += `\n- Browser:           ${navigator.userAgent}`;

  return info;
}

function handleDarkMode() {
  $("<link/>", {
    rel: "stylesheet",
    type: "text/css",
    href: "../css/darkMode.css"
  }).appendTo("head");

  setStorage("darkMode", "on");
  $('#btn_darkmode_icon').removeClass('fa fa-moon-o').addClass('mdi mdi-white-balance-sunny');
  $('#navbar_brand_logo').attr("src", 'img/hyperion/logo_negativ.png');
}

function isAccessLevelCompliant(accessLevel) {
  if (!accessLevel) return true;

  switch (accessLevel) {
    case 'system':
      return false;
    case 'advanced':
      return storedAccess !== 'default';
    case 'expert':
      return storedAccess === 'expert';
    default:
      return true;
  }
}

function showInputOptions(path, elements, state) {

  if (!path.startsWith("root.")) {
    path = ["root", path].join('.');
  }

  for (const element of elements) {
    $('[data-schemapath="' + path + '.' + element + '"]').toggle(state);
  }
}

function showInputOptionForItem(editor, path, item, state) {
  // Get access level for the full path and item
  const accessLevel = getObjectProperty(editor.schema.properties, `${getLongPropertiesPath(path)}${item}.access`);

  // Enable the element only if access level is compliant
  if (!state || isAccessLevelCompliant(accessLevel)) {
    // If path is not provided, use the editor's path
    if (!path) {
      path = editor.path;
    }
    showInputOptions(path, [item], state);
  }
}

function showInputOptionsForKey(editor, item, showForKeys, state) {
  const elements = [];
  let keysToShow = [];

  // Determine keys to show based on input type
  if (Array.isArray(showForKeys)) {
    keysToShow = showForKeys;
  } else if (typeof showForKeys === 'string') {
    keysToShow.push(showForKeys);
  } else {
    return;
  }

  const itemProperties = editor.schema.properties[item].properties;

  for (const key in itemProperties) {
    // Skip the key if it is not in the list of keys to show
    if (!keysToShow.includes(key)) {
      const { access, options } = itemProperties[key];
      const hidden = options?.hidden || false;

      // Always disable all elements, but enable only if access level is compliant and not hidden
      if ((!state || isAccessLevelCompliant(access)) && !hidden) {
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
  return !!(value.match(
    '^(?:(?:(?:[a-fA-F0-9]{1,4}:){6}|(?=(?:[a-fA-F0-9]{0,4}:){2,6}(?:[0-9]{1,3}.){3}[0-9]{1,3}$)(([0-9a-fA-F]{1,4}:){1,5}|:)((:[0-9a-fA-F]{1,4}){1,5}:|:)|::(?:[a-fA-F0-9]{1,4}:){5})(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9]).){3}(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])|(?:[a-fA-F0-9]{1,4}:){7}[a-fA-F0-9]{1,4}|(?=(?:[a-fA-F0-9]{0,4}:){0,7}[a-fA-F0-9]{0,4}$)(([0-9a-fA-F]{1,4}:){1,7}|:)((:[0-9a-fA-F]{1,4}){1,7}|:)|(?:[a-fA-F0-9]{1,4}:){7}:|:(:[a-fA-F0-9]{1,4}){7})$'
  ));
}

function isValidHostname(value) {
  return !!(value.match(
    '^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]{0,61}[a-zA-Z0-9])(.([a-zA-Z0-9]|[_a-zA-Z0-9][a-zA-Z0-9-]{0,61}[a-zA-Z0-9]))*$'
  ));
}

function isValidServicename(value) {
  return !!(value.match(
    '^([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9 -]{0,61}[a-zA-Z0-9])(.([a-zA-Z0-9]|[_a-zA-Z0-9][a-zA-Z0-9-]{0,61}[a-zA-Z0-9]))*$'
  ));
}

function isValidHostnameOrIP4(value) {
  return (isValidHostname(value) || isValidIPv4(value));
}

function isValidHostnameOrIP(value) {
  return (isValidHostnameOrIP4(value) || isValidIPv6(value) || isValidServicename(value));
}

const loadedScripts = [];

function isScriptLoaded(src) {
  return loadedScripts.indexOf(src) > -1;
}

function loadScript(src, callback, ...params) {
  if (isScriptLoaded(src)) {
    debugMessage('Script ' + src + ' already loaded');
    if (callback && typeof callback === 'function') {
      callback(...params);
    }
    return;
  }

  const script = document.createElement('script');
  script.src = src;

  script.onload = function () {
    debugMessage('Script ' + src + ' loaded successfully');
    loadedScripts.push(src);

    if (callback && typeof callback === 'function') {
      callback(...params);
    }
  };

  document.head.appendChild(script);
}

// Function to reverse the transformed config into the legacy format
function reverseTransformConfig(serverConfig, instanceId) {
  const { global, instances } = serverConfig;

  // Initialize the resulting legacy config
  const legacyConfig = {};

  // Add global settings to the legacy config
  if (global?.settings) {
    Object.assign(legacyConfig, global.settings);
  }

  // Find the instance with the matching id and add its settings
  const instance = instances?.find(inst => inst.id === instanceId);
  if (instance?.settings) {
    Object.assign(legacyConfig, instance.settings);
  }

  return legacyConfig;
}

