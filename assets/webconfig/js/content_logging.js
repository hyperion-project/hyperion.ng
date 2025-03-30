var conf_editor = null;
var createdCont = false;
var isScroll = true;

performTranslation();

$(document).ready(function () {

  window.addEventListener('hashchange', function (event) {
    requestLoggingStop();
  });

  requestLoggingStart();

  $('#conf_cont').append(createOptPanel('fa-reorder', $.i18n("edt_conf_log_heading_title"), 'editor_container', 'btn_submit'));
  if (window.showOptHelp) {
    $('#conf_cont').append(createHelpTable(window.schema.logger.properties, $.i18n("edt_conf_log_heading_title")));
    createHintH("intro", $.i18n('conf_logging_label_intro'), "log_head");
  }

  conf_editor = createJsonEditor('editor_container', {
    logger: window.schema.logger
  }, true, true);

  conf_editor.on('change', function () {
    conf_editor.validate().length || window.readOnlyMode ? $('#btn_submit').prop('disabled', true) : $('#btn_submit').prop('disabled', false);
  });

  $('#btn_submit').off().on('click', function () {

    var displayedLogLevel = conf_editor.getEditor("root.logger.level").getValue();
    var newLogLevel = { logger: {} };
    newLogLevel.logger.level = displayedLogLevel;

    requestWriteConfig(newLogLevel);
  });

  function infoSummary() {
    let info = '';

    const serverConfig = window.serverConfig ?? {};
    const currentInstance = window.currentHyperionInstance;
    const currentInstanceName = window.currentHyperionInstanceName ?? 'Unknown';

    info += `Hyperion System Summary Report (${serverConfig.general?.name ?? 'Unknown'})\n`;

    if (currentInstance !== null) {
      info += `Reported instance: [${currentInstance}] - ${currentInstanceName}\n`;
    }

    info += `\n< ----- System information -------------------- >\n`;
    info += `${getSystemInfo()}\n`;

    // Configured Instances
    info += `\n< ----- Configured Instances ------------------ >\n`;
    const instances = serverInfo.instance ?? [];

    if (instances.length > 0) {
      info += instances.map(inst =>
        `${inst.instance ?? 'Unknown Instance'}: ${inst.friendly_name ?? 'Unnamed'}, Running: ${inst.running ?? 'Unknown'}`
      ).join('\n') + '\n';

      // Priorities (Only shown if instances exist)
      info += `\n< ----- This instance's priorities ------------ >\n`;
      const priorities = serverInfo.priorities ?? [];

      if (priorities.length > 0) {
        info += priorities.map(prio => {
          const priorityStr = prio.priority?.toString().padStart(3, '0') ?? 'N/A';
          return `${priorityStr}: ${prio.visible ? 'VISIBLE   -' : 'INVISIBLE -'} (${prio.componentId ?? 'Unknown Component'})` +
            (prio.owner ? ` (Owner: ${prio.owner})` : '');
        }).join('\n') + '\n';
      } else {
        info += `The current priority list is empty or unavailable!\n`;
      }

      info += `Autoselect: ${serverInfo.priorities_autoselect ?? 'N/A'}\n`;

      // Components Status (Only shown if instances exist)
      info += `\n< ----- This instance components' status ------->\n`;
      const components = serverInfo.components ?? [];

      if (components.length > 0) {
        info += components.map(comp =>
          `${comp.name} - ${comp.enabled ?? 'Unknown'}`
        ).join('\n') + '\n';
      } else {
        info += `No components found or unavailable!\n`;
      }
    } else {
      info += `No instances are configured!\n`;
    }

    // Configuration
    const config = transformConfig(serverConfig, currentInstance);
    info += `\n< ----- Global configuration items------------- >\n`;
    info += `${JSON.stringify(config.global, null, 2)}\n`;

    if (instances.length > 0) {
      info += `\n< ----- Selected Instance configuration items-- >\n`;
      info += `${JSON.stringify(config.instances, null, 2)}\n`;
    }

    // Log Messages
    info += `\n< ----- Current Log --------------------------- >\n`;
    const logMessages = document.getElementById("logmessages")?.textContent.trim() ?? '';

    info += logMessages.length > 0 ? logMessages : "Log is empty!";

    return info;
  }

  function createLogContainer() {

    const isScrollEnableStyle = (isScroll ? "checked" : "");

    $('#log_content').html('<pre><div id="logmessages" style="overflow:scroll;max-height:400px"></div></pre>');
    $('#log_footer').append('<label class="checkbox-inline">'
      + '<input id = "btn_scroll"' + isScrollEnableStyle + ' type = "checkbox"'
      + 'data-toggle="toggle" data-onstyle="success" data-on="' + $.i18n('general_btn_on') + '" data-off="' + $.i18n('general_btn_off') + '">'
      + $.i18n('conf_logging_btn_autoscroll') + '</label>'
    );

    $(`#btn_scroll`).bootstrapToggle();
    $(`#btn_scroll`).on("change", e => {
      if (e.currentTarget.checked) {
        //Scroll to end of log
        isScroll = true;
        if ($("#logmessages").length > 0) {
          $('#logmessages')[0].scrollTop = $('#logmessages')[0].scrollHeight;
        }
      } else {
        isScroll = false;
      }
    });

    $('#log_footer').append('<button class="btn btn-primary pull-right" id="btn_clipboard"><i class="fa fa-fw fa-clipboard"></i>' + $.i18n("conf_logging_btn_clipboard") + '</button>');

    $('#btn_clipboard').off().on('click', function () {
      const temp = document.createElement('textarea');
      temp.textContent = infoSummary();
      document.body.append(temp);
      temp.select();
      document.execCommand("copy");
      temp.remove();
    });
  }

  function updateLogOutput(messages) {

    if (messages.length != 0) {

      for (var idx = 0; idx < messages.length; idx++) {
        var logger_name = messages[idx].loggerName;
        var logger_subname = messages[idx].loggerSubName;
        var function_ = messages[idx].function;
        var line = messages[idx].line;
        var file_name = messages[idx].fileName;
        var msg = encodeHTML(messages[idx].message);
        var level_string = messages[idx].levelString;
        var utime = messages[idx].utime;

        var debug = "";
        if (level_string == "DEBUG") {
          debug = "(" + file_name + ":" + line + ":" + function_ + "()) ";
        }

        var date = new Date(parseInt(utime));
        var subComponent = "";
        if (window.serverInfo.instance.length >= 1) {
          if (logger_subname.startsWith("I")) {
            var instanceNum = logger_subname.substring(1);
            if (window.serverInfo.instance[instanceNum]) {
              subComponent = window.serverInfo.instance[instanceNum].friendly_name;
            } else {
              subComponent = instanceNum;
            }
          }
        }
        var newLogLine = date.toISOString() + " [" + logger_name + (subComponent ? "|" + subComponent : "") + "] (" + level_string + ") " + debug + msg;

        $("#logmessages").append("<code>" + newLogLine + "</code>\n");
      }

      if (isScroll && $("#logmessages").length > 0) {
        $('#logmessages').stop().animate({
          scrollTop: $('#logmessages')[0].scrollHeight
        }, 800);
      }
    }
  }

  if (!window.loggingHandlerInstalled) {
    window.loggingHandlerInstalled = true;

    $(window.hyperion).on("cmd-logmsg-update", function (event) {

      var messages = (event.response.data.messages);

      if (messages.length != 0) {
        if (!createdCont) {
          createLogContainer();
          createdCont = true;
        }
        updateLogOutput(messages)
      }
    });
  }

  $(window.hyperion).on("cmd-settings-update", function (event) {

    var obj = event.response.data
    if (obj.logger) {
      Object.getOwnPropertyNames(obj).forEach(function (val, idx, array) {
        window.serverConfig[val] = obj[val];
      });

      var currentlogLevel = window.serverConfig.logger.level;
      conf_editor.getEditor("root.logger.level").setValue(currentlogLevel);
      location.reload();
    }

  });

  // toggle fullscreen button in log output
  $(".fullscreen-btn").mousedown(function (e) {
    e.preventDefault();
  });

  $(".fullscreen-btn").click(function (e) {
    e.preventDefault();
    $(this).children('i')
      .toggleClass('fa-expand')
      .toggleClass('fa-compress');
    $('#conf_cont').toggle();
    $('#logmessages').css('max-height', $('#logmessages').css('max-height') !== 'none' ? 'none' : '400px');
  });

  removeOverlay();
});

