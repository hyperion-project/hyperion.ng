$(document).ready(function () {
  performTranslation();
  var conf_editor_v4l2 = null;
  var conf_editor_fg = null;
  var conf_editor_instCapt = null;
  var VIDEOGRABBER_AVAIL = window.serverInfo.grabbers.available.includes("v4l2");

  if (window.showOptHelp) {
    // Instance Capture
    $('#conf_cont').append(createRow('conf_cont_instCapt'));
    $('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));

    // Framegrabber
    $('#conf_cont').append(createRow('conf_cont_fg'));
    $('#conf_cont_fg').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
    $('#conf_cont_fg').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title")));

    // V4L2 - hide if not available
    if (VIDEOGRABBER_AVAIL) {
      $('#conf_cont').append(createRow('conf_cont_v4l'));
      $('#conf_cont_v4l').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
      $('#conf_cont_v4l').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title")));
    }
  } else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
    if (VIDEOGRABBER_AVAIL) {
      $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
    }
  }

  // Instance Capture
  conf_editor_instCapt = createJsonEditor('editor_container_instCapt', {
    instCapture: window.schema.instCapture
  }, true, true);

  // Hide V4L2 elements, if not available
  if (!VIDEOGRABBER_AVAIL) {
    $('[data-schemapath*="root.instCapture.v4lEnable' + '"]').hide();
    $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').hide();
  }

  conf_editor_instCapt.on('change', function () {
    var systemEnable = conf_editor_instCapt.getEditor("root.instCapture.systemEnable").getValue();
    if (systemEnable) {
      $('[data-schemapath*="root.instCapture.systemPriority' + '"]').show();
      $('#conf_cont_fg').show();
    } else {
      $('#conf_cont_fg').hide();
      $('[data-schemapath*="root.instCapture.systemPriority' + '"]').hide();
    }

    if (VIDEOGRABBER_AVAIL) {
      var v4lEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (v4lEnable) {
        $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').show();
        $('#conf_cont_v4l').show();
      }
      else {
        $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').hide();
        $('#conf_cont_v4l').hide();
      }
    }

    conf_editor_instCapt.validate().length || window.readOnlyMode ? $('#btn_submit_instCapt').attr('disabled', true) : $('#btn_submit_instCapt').attr('disabled', false);
  });

  conf_editor_instCapt.watch('root.instCapture.v4lEnable', () => {
    if (VIDEOGRABBER_AVAIL) {
      var v4lEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (v4lEnable) {
        discoverInputSources("video");
      }
    }
  });

  $('#btn_submit_instCapt').off().on('click', function () {
    requestWriteConfig(conf_editor_instCapt.getValue());
  });

  // Framegrabber
  conf_editor_fg = createJsonEditor('editor_container_fg', {
    framegrabber: window.schema.framegrabber
  }, true, true);

  conf_editor_fg.on('ready', function () {
    var availableGrabbers = window.serverInfo.grabbers.available;
    var fgOptions = conf_editor_fg.getEditor('root.framegrabber');
    var orginalGrabberTypes = fgOptions.schema.properties.type.enum;
    var orginalGrabberTitles = fgOptions.schema.properties.type.options.enum_titles;

    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";

    for (var i = 0; i < orginalGrabberTypes.length; i++) {
      var grabberType = orginalGrabberTypes[i];
      if ($.inArray(grabberType, availableGrabbers) != -1) {
        enumVals.push(grabberType);
        enumTitelVals.push(orginalGrabberTitles[i]);
      }
    }

    var activeGrabbers = window.serverInfo.grabbers.active.map(v => v.toLowerCase());

    // Select first active platform grabber
    for (var i = 0; i < enumVals.length; i++) {
      var grabberType = enumVals[i];
      if ($.inArray(grabberType, activeGrabbers) != -1) {
        enumDefaultVal = grabberType;
        break;
      }
    }
    updateJsonEditorSelection(fgOptions, "type", {}, enumVals, enumTitelVals, enumDefaultVal);
  });

  conf_editor_fg.on('change', function () {
    var selectedType = conf_editor_fg.getEditor("root.framegrabber.type").getValue();
    filerFgGrabberOptions(selectedType);
    conf_editor_fg.validate().length || window.readOnlyMode ? $('#btn_submit_fg').attr('disabled', true) : $('#btn_submit_fg').attr('disabled', false);
  });

  function toggleFgOptions(el, state) {
    for (var i = 0; i < el.length; i++) {
      $('[data-schemapath*="root.framegrabber.' + el[i] + '"]').toggle(state);
    }
  }

  function filerFgGrabberOptions(type) {
    //hide specific options for grabbers found
    var grabbers = window.serverInfo.grabbers.available;
    if (grabbers.indexOf(type) > -1) {
      toggleFgOptions(["width", "height", "pixelDecimation", "display"], true);

      switch (type) {
        case "dispmanx":
          toggleFgOptions(["pixelDecimation", "display"], false);
          break;
        case "x11":
        case "xcb":
          toggleFgOptions(["width", "height", "display"], false);
          break;
        case "framebuffer":
          toggleFgOptions(["display"], false);
          break;
        case "amlogic":
          toggleFgOptions(["pixelDecimation", "display"], false);
          break;
        case "qt":
          break;
        case "dx":
          break;
        case "osx":
          break;
        default:
      }
    }
  };

  $('#btn_submit_fg').off().on('click', function () {
    requestWriteConfig(conf_editor_fg.getValue());
  });

  // External Input Sources (Video-Grabbers)

  var configuredDevice = "";
  var discoveredInputSources = {};
  var deviceProperties = {};

  if (VIDEOGRABBER_AVAIL) {
    conf_editor_v4l2 = createJsonEditor('editor_container_v4l2', {
      grabberV4L2: window.schema.grabberV4L2
    }, true, true);

    conf_editor_v4l2.on('ready', function () {
      var v4lEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (v4lEnable) {
        discoverInputSources("video");
      }
    });

    conf_editor_v4l2.on('change', function () {
      var deviceSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.available_devices").getValue();
      if (!conf_editor_v4l2.validate().length) {
        if (deviceSelected !== "NONE") {
          window.readOnlyMode ? $('#btn_submit_v4l2').attr('disabled', true) : $('#btn_submit_v4l2').attr('disabled', false);
        }
      }
    });

    conf_editor_v4l2.watch('root.grabberV4L2.available_devices', () => {
      var deviceSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.available_devices").getValue();
      if (deviceSelected === "NONE" || deviceSelected === "") {
        $('#btn_submit_v4l2').attr('disabled', true);
      }
      else {
        var addSchemaElements = {};
        var enumVals = [];
        var enumTitelVals = [];
        var enumDefaultVal = "";

        var deviceProperties = getPropertiesOfDevice(deviceSelected);

        //Update hidden input element
        conf_editor_v4l2.getEditor("root.grabberV4L2.device").setValue(deviceProperties.device);

        var video_inputs = deviceProperties.video_inputs;
        if (video_inputs.length <= 1) {
          addSchemaElements.access = "expert";
        }

        for (const video_input of video_inputs) {
          enumVals.push(video_input.inputIdx);
          enumTitelVals.push(video_input.name);
        }

        if (enumVals.length > 0) {
          if (deviceSelected === configuredDevice) {
            var configuredVideoInput = window.serverConfig.grabberV4L2.input;
            if ($.inArray(configuredVideoInput, enumVals) != -1) {
              enumDefaultVal = configuredVideoInput;
            }
          }
          updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
            'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
        }

        if (!window.readOnlyMode) {
          $('#btn_submit_v4l2').attr('disabled', false);
        }
      }
    });

    conf_editor_v4l2.watch('root.grabberV4L2.device_inputs', () => {
      var deviceSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.device_inputs").getValue();

      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice(deviceSelected);
      var formats = deviceProperties.video_inputs[videoInputSelected].formats;
      //Hide, if only one record available for selection
      if (formats.length <= 1) {
        addSchemaElements.access = "expert";
      }

      for (var i = 0; i < formats.length; i++) {
        if (formats[i].format) {
          enumVals.push(formats[i].format);
          enumTitelVals.push(formats[i].format.toUpperCase());
        }
        else {
          enumVals.push("NONE");
        }
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredEncoding = window.serverConfig.grabberV4L2.encoding;
          if ($.inArray(configuredEncoding, enumVals) != -1) {
            enumDefaultVal = configuredEncoding;
          }
        }
        updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
          'encoding', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      var enumVals = [];
      var enumDefaultVal = "";

      var standards = deviceProperties.video_inputs[videoInputSelected].standards;
      if (!standards) {
        enumVals.push("NONE");
        addSchemaElements.options = { "hidden": true };
      } else {
        enumVals = standards;
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredStandard = window.serverConfig.grabberV4L2.standard;
          if ($.inArray(configuredStandard, enumVals) != -1) {
            enumDefaultVal = configuredStandard;
          }
        }

        updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
          'standard', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_v4l2').attr('disabled', false);
      }
    });

    conf_editor_v4l2.watch('root.grabberV4L2.encoding', () => {
      var deviceSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.device_inputs").getValue();
      var formatSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.encoding").getValue();

      //Update hidden input element
      conf_editor_v4l2.getEditor("root.grabberV4L2.input").setValue(parseInt(videoInputSelected));

      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice(deviceSelected);

      var formats = deviceProperties.video_inputs[videoInputSelected].formats;
      var formatIdx = 0;
      if (formatSelected !== "NONE") {
        formatIdx = formats.findIndex(x => x.format === formatSelected);
      }

      var resolutions = formats[formatIdx].resolutions;
      if (resolutions.length <= 1) {
        addSchemaElements.access = "expert";
      } else {
        resolutions.sort(compareTwoValues('width', 'height', 'asc'));
      }

      for (var i = 0; i < resolutions.length; i++) {
        enumVals.push(i);
        var resolutionText = resolutions[i].width + "x" + resolutions[i].height;
        enumTitelVals.push(resolutionText);
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredResolutionText = window.serverConfig.grabberV4L2.width + "x" + window.serverConfig.grabberV4L2.height;
          var idx = $.inArray(configuredResolutionText, enumTitelVals)
          if (idx != -1) {
            enumDefaultVal = idx;
          }
        }

        updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_v4l2').attr('disabled', false);
      }
    });

    conf_editor_v4l2.watch('root.grabberV4L2.resolutions', () => {
      var deviceSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.device_inputs").getValue();
      var formatSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.encoding").getValue();
      var resolutionSelected = conf_editor_v4l2.getEditor("root.grabberV4L2.resolutions").getValue();

      var addSchemaElements = {};
      var enumVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice(deviceSelected);

      var formats = deviceProperties.video_inputs[videoInputSelected].formats;
      var formatIdx = 0;
      if (formatSelected !== "NONE") {
        formatIdx = formats.findIndex(x => x.format === formatSelected);
      }

      //Update hidden resolution related elements
      var width = parseInt(formats[formatIdx].resolutions[resolutionSelected].width);
      conf_editor_v4l2.getEditor("root.grabberV4L2.width").setValue(width);

      var height = parseInt(formats[formatIdx].resolutions[resolutionSelected].height);
      conf_editor_v4l2.getEditor("root.grabberV4L2.height").setValue(height);

      var fps = formats[formatIdx].resolutions[resolutionSelected].fps;
      if (!fps) {
        enumVals.push("NONE");
        addSchemaElements.options = { "hidden": true };
      } else {
        fps.sort((a, b) => a - b);
        for (var i = 0; i < fps.length; i++) {
          enumVals.push(fps[i]);
        }
      }

      if (enumVals.length <= 1) {
        addSchemaElements.access = "expert";
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredFps = window.serverConfig.grabberV4L2.fps;
          if ($.inArray(configuredFps, enumVals) != -1) {
            enumDefaultVal = configuredFps;
          }
        }
        updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
          'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_v4l2').attr('disabled', false);
      }
    });

    conf_editor_v4l2.watch('root.grabberV4L2.framerates', () => {
      //Update hidden fps element
      var fps = 0;
      var framerates = conf_editor_v4l2.getEditor("root.grabberV4L2.framerates").getValue();
      if (framerates !== "NONE") {
        fps = parseInt(framerates);
      }

      //Show Frameskipping only when more than 2 fps
      if (fps > 2) {
        $('[data-schemapath*="root.grabberV4L2.fpsSoftwareDecimation').toggle(true);
      }
      else {
        $('[data-schemapath*="root.grabberV4L2.fpsSoftwareDecimation').toggle(false);
      }
      conf_editor_v4l2.getEditor("root.grabberV4L2.fps").setValue(fps);
    });

    $('#btn_submit_v4l2').off().on('click', function () {
      var v4l2Options = conf_editor_v4l2.getValue();
      requestWriteConfig(v4l2Options);
    });
  }

  //////////////////////////////////////////////////

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_fg");
    if (VIDEOGRABBER_AVAIL) {
      createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_v4l2");
    }
  }

  removeOverlay();

  // build dynamic enum
  var updateVideoSourcesList = function (type, discoveryInfo) {
    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";

    if (jQuery.isEmptyObject(discoveryInfo)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_grabber_discovered_none'));

      conf_editor_v4l2.getEditor('root.grabberV4L2').disable();
    }
    else {
      for (const device of discoveryInfo) {
        enumVals.push(device.device_name);
      }
      conf_editor_v4l2.getEditor('root.grabberV4L2').enable();
    }

    if (enumVals.length > 0) {
      configuredDevice = window.serverConfig.grabberV4L2.available_devices;
      if ($.inArray(configuredDevice, enumVals) != -1) {
        enumDefaultVal = configuredDevice;
      }

      updateJsonEditorSelection(conf_editor_v4l2.getEditor('root.grabberV4L2'),
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, false);
    }
  }

  async function discoverInputSources(type, params) {
    const result = await requestInputSourcesDiscovery(type, params);

    var discoveryResult;
    if (result && !result.error) {
      discoveryResult = result.info;
    }
    else {
      discoveryResult = {
        "video_sources": []
      }
    }
    discoveredInputSources = discoveryResult.video_sources;

    updateVideoSourcesList(type, discoveredInputSources);
  }

  function getPropertiesOfDevice(deviceName) {
    deviceProperties = {};
    for (const deviceRecord of discoveredInputSources) {
      if (deviceRecord.device_name === deviceName) {
        deviceProperties = deviceRecord;
        break;
      }
    }
    return deviceProperties;
  }
});
