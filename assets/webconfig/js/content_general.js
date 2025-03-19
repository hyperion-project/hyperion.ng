$(document).ready(function () {
  // Perform translation on page load
  performTranslation();

  let importedConf;
  let confName;
  let conf_editor = null;

  // Initialize the configuration panel
  $('#conf_cont').append(createOptPanel('fa-wrench', $.i18n("edt_conf_gen_heading_title"), 'editor_container', 'btn_submit', 'panel-system'));

  // Show help if needed
  if (window.showOptHelp) {
    $('#conf_cont').append(createHelpTable(window.schema.general.properties, $.i18n("edt_conf_gen_heading_title")));
  } else {
    $('#conf_imp').appendTo('#conf_cont');
  }

  // Create JSON editor
  conf_editor = createJsonEditor('editor_container', { general: window.schema.general }, true, true);

  // Handle editor change
  conf_editor.on('change', function () {
    const isValid = !conf_editor.validate().length && !window.readOnlyMode;
    $('#btn_submit').prop('disabled', !isValid);
  });

  // Submit button click handler
  $('#btn_submit').off().on('click', function () {
    window.showOptHelp = conf_editor.getEditor("root.general.showOptHelp").getValue();
    requestWriteConfig(conf_editor.getValue());
  });

  // Instance handling functions
  function handleInstanceRename(instance) {
    showInfoDialog('renInst', $.i18n('conf_general_inst_renreq_t'), getInstanceName(instance));

    // Rename button click handler
    $("#id_btn_ok").off().on('click', function () {
      requestInstanceRename(instance, encodeHTML($('#renInst_name').val()));
    });

    // Input handler for rename field
    $('#renInst_name').off().on('input', function (e) {
      const isValid = e.currentTarget.value.length >= 5 && e.currentTarget.value !== getInstanceName(instance);
      $('#id_btn_ok').prop('disabled', !isValid);
    });
  }

  function handleInstanceDelete(instance) {
    showInfoDialog('delInst', $.i18n('conf_general_inst_delreq_h'), $.i18n('conf_general_inst_delreq_t', getInstanceName(instance)));

    // Delete button click handler
    $("#id_btn_yes").off().on('click', function () {
      requestInstanceDelete(instance);
    });
  }

  // Build the instance list
  function buildInstanceList() {

    const instances = serverInfo.instance;

    // Ensure .itbody exists and clear it
    const $itbody = $('.itbody');
    if ($itbody.length === 0) {
      console.warn("Element '.itbody' does not exist. Aborting instance list build.");
      return;
    }
    $itbody.empty(); // Explicitly clear the content before adding new rows

    // Collect rows in a document fragment for efficient DOM updates
    const $rows = $(document.createDocumentFragment());

    for (const instance in instances) {
      const instanceID = instances[instance].instance;
      const enableStyle = instances[instance].running ? "checked" : "";
      const renameBtn = `<button id="instren_${instanceID}" type="button" class="btn btn-primary">
                               <i class="mdi mdi-lead-pencil"></i>
                           </button>`;
      const delBtn = `<button id="instdel_${instanceID}" type="button" class="btn btn-danger">
                            <i class="mdi mdi-delete-forever"></i>
                        </button>`;
      const startBtn = `<input id="inst_${instanceID}" ${enableStyle} type="checkbox" 
                             data-toggle="toggle" data-onstyle="success font-weight-bold" 
                             data-on="${$.i18n('general_btn_on')}" data-offstyle="default font-weight-bold" 
                             data-off="${$.i18n('general_btn_off')}">`;

      // Generate a table row and add it to the document fragment
      const $row = createTableRow(
        [
          instances[instance].friendly_name,
          startBtn,
          renameBtn,
          delBtn
        ],
        false,
        true
      );

      $rows.append($row);
    }

    // Append all rows to .itbody in one operation
    $itbody.append($rows);

    // Reapply event listeners
    for (const instance in instances) {
      const instanceID = instances[instance].instance;

      const readOnly = window.readOnlyMode;
      $('#instren_' + instanceID).prop('disabled', readOnly);
      $('#inst_' + instanceID).prop('disabled', readOnly);
      $('#instdel_' + instanceID).prop('disabled', readOnly);

      $('#instren_' + instanceID).off().on('click', function () {
        handleInstanceRename(instanceID);
      });

      $('#instdel_' + instanceID).off().on('click', function () {
        handleInstanceDelete(instanceID);
      });

      $('#inst_' + instanceID).bootstrapToggle();
      $('#inst_' + instanceID).change(function () {
        const isChecked = $(this).prop('checked');
        requestInstanceStartStop(instanceID, isChecked);
      });
    }
  }

  // Initialize table
  createTable('ithead', 'itbody', 'itable');
  $('.ithead').html(createTableRow([$.i18n('conf_general_inst_namehead'), "", $.i18n('conf_general_inst_actionhead'), ""], true, true));
  buildInstanceList();

  // Instance name input validation
  $('#inst_name').off().on('input', function (e) {
    const isValid = e.currentTarget.value.length >= 5 && !window.readOnlyMode;
    $('#btn_create_inst').prop('disabled', !isValid);

    const charsNeeded = 5 - e.currentTarget.value.length;
    $('#inst_chars_needed').html(charsNeeded >= 1 && charsNeeded <= 4 ? `${charsNeeded} ${$.i18n('general_chars_needed')}` : "<br />");
  });

  // Instance creation button click handler
  $('#btn_create_inst').off().on('click', function (e) {
    requestInstanceCreate(encodeHTML($('#inst_name').val()));
    $('#inst_name').val("");
    $('#btn_create_inst').prop('disabled', true);
  });

  // Instance updated event listener
  $(hyperion).off("instance-updated").on("instance-updated", function (event) {
    buildInstanceList();
  });

  // Import handling functions
  function dis_imp_btn(state) {
    $('#btn_import_conf').prop('disabled', state || window.readOnlyMode);
  }

  function readFile(evt) {
    const f = evt.target.files[0];
    if (f) {
      const r = new FileReader();
      r.onload = function (e) {
        let content = e.target.result.replace(/[^:]?\/\/.*/g, ''); // Remove comments

        // Check if the content is valid JSON
        const check = isJsonString(content);
        if (check.length !== 0) {
          showInfoDialog('error', "", $.i18n('infoDialog_import_jsonerror_text', f.name, JSON.stringify(check.message)));
          dis_imp_btn(true);
        } else {
          content = JSON.parse(content);
          if (typeof content.global === 'undefined' || typeof content.instances === 'undefined') {
            showInfoDialog('error', "", $.i18n('infoDialog_import_version_error_text', f.name));
            dis_imp_btn(true);
          } else {
            dis_imp_btn(false);
            importedConf = content;
            confName = f.name;
          }
        }
      };
      r.readAsText(f);
    }
  }

  // Import button click handler
  $('#btn_import_conf').off().on('click', function () {
    showInfoDialog('import', $.i18n('infoDialog_import_confirm_title'), $.i18n('infoDialog_import_confirm_text', confName));

    $('#id_btn_import').off().on('click', function () {
      requestRestoreConfig(importedConf);
    });
  });

  // Import file selection change handler
  $('#select_import_conf').off().on('change', function (e) {
    if (window.File && window.FileReader && window.FileList && window.Blob) {
      readFile(e);
    } else {
      showInfoDialog('error', "", $.i18n('infoDialog_import_comperror_text'));
    }
  });

  // Export configuration
  $('#btn_export_conf').off().on('click', async () => {
    const d = new Date();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    const timestamp = `${d.getFullYear()}-${month}-${day}`;

    const configBackup = await requestServerConfig.async();
    if (configBackup.success) {
      download(JSON.stringify(configBackup.info, null, "\t"), `HyperionBackup-${timestamp}_v${window.currentVersion}.json`, "application/json");
    }
  });

  // Create introduction hints if help is shown
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_general_intro'), "editor_container");
    createHint("intro", $.i18n('conf_general_tok_desc'), "tok_desc_cont");
    createHint("intro", $.i18n('conf_general_inst_desc'), "inst_desc_cont");
  }

  removeOverlay();
});

// Command for restoring config
$(window.hyperion).on("cmd-config-restoreconfig", function () {
  setTimeout(initRestart, 100);
});

