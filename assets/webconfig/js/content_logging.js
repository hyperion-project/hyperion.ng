var conf_editor = null;
var createdCont = false;
var isScroll = true;

performTranslation();
requestLoggingStop();

$(document).ready(function () {

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
    var info = "";

    info += 'Hyperion System Summary Report (' + window.serverConfig.general.name + ')\n';
    info += 'Reported instance: [' + window.currentHyperionInstance + '] - ' + window.currentHyperionInstanceName + '\n';

    info += "\n< ----- System information -------------------- >\n";
    info += getSystemInfo() + '\n';

    info += "\n< ----- Configured Instances ------------------ >\n";
    var instances = window.serverInfo.instance;
    for (var i = 0; i < instances.length; i++) {
      info += instances[i].instance + ': ' + instances[i].friendly_name + ', Running: ' + instances[i].running + '\n';
    }

    info += "\n< ----- This instance's priorities ------------ >\n";
    var prios = window.serverInfo.priorities;

    if (prios.length > 0) {

      for (var i = 0; i < prios.length; i++) {

        var prio = prios[i].priority.toString().padStart(3, '0');

        info += prio + ': ';
        if (prios[i].visible) {
          info += ' VISIBLE   -';
        }
        else {
          info += ' INVISIBLE -';
        }
        info += ' (' + prios[i].componentId + ')';
        if (prios[i].owner) {
          info += ' (Owner: ' + prios[i].owner + ')';
        }
        info += '\n';

      }
    } else {
      info += 'The current priority list is empty!\n';
    }
    info += 'Autoselect: ' + window.serverInfo.priorities_autoselect + '\n';

    info += "\n< ----- This instance components' status ------->\n";
    var comps = window.serverInfo.components;
    for (var i = 0; i < comps.length; i++) {
      info += comps[i].name + ' - ' + comps[i].enabled + '\n';
    }

    info += "\n< ----- This instance's configuration --------- >\n";
    info += JSON.stringify(window.serverConfig, null, 2) + '\n';

    info += "\n< ----- Current Log --------------------------- >\n";
    var logMsgs = document.getElementById("logmessages").textContent;
    if (logMsgs.length !== 0) {
      info += logMsgs;
    } else {
      info += "Log is empty!";
    }

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

    $(window.hyperion).on("cmd-logging-update", function (event) {

      var messages = (event.response.result.messages);

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

