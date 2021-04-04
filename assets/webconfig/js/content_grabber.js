$(document).ready(function () {
  performTranslation();
  var conf_editor_video = null;
  var conf_editor_screen = null;
  var conf_editor_instCapt = null;
  var VIDEOGRABBER_AVAIL = window.serverInfo.grabbers.available.includes("v4l2");

  if (window.showOptHelp) {
    // Instance Capture
    $('#conf_cont').append(createRow('conf_cont_instCapt'));
    $('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));

    // Screen-Grabber
    $('#conf_cont').append(createRow('conf_cont_screen'));
    $('#conf_cont_screen').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_screengrabber', 'btn_submit_screengrabber'));
    $('#conf_cont_screen').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title")));

    // Video-Grabber - hide if not available
    if (VIDEOGRABBER_AVAIL) {
      $('#conf_cont').append(createRow('conf_cont_video'));
      $('#conf_cont_video').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_videograbber', 'btn_submit_videograbber'));
      $('#conf_cont_video').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title")));
    }
  } else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_screengrabber', 'btn_submit_screengrabber'));
    if (VIDEOGRABBER_AVAIL) {
      $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_videograbber', 'btn_submit_videograbber'));
    }
  }

  // Instance Capture
  conf_editor_instCapt = createJsonEditor('editor_container_instCapt', {
    instCapture: window.schema.instCapture
  }, true, true);

  // Hide Video-Grabber elements, if not available
  if (!VIDEOGRABBER_AVAIL) {
    $('[data-schemapath*="root.instCapture.v4lEnable' + '"]').hide();
    $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').hide();
  }

  conf_editor_instCapt.on('change', function () {
    var screenEnable = conf_editor_instCapt.getEditor("root.instCapture.systemEnable").getValue();
    if (screenEnable) {
      $('[data-schemapath*="root.instCapture.systemPriority' + '"]').show();
      $('#conf_cont_screen').show();
    } else {
      $('#conf_cont_screen').hide();
      $('[data-schemapath*="root.instCapture.systemPriority' + '"]').hide();
    }

    if (VIDEOGRABBER_AVAIL) {
      var videoEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (videoEnable) {
        $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').show();
        $('#conf_cont_video').show();
      }
      else {
        $('[data-schemapath*="root.instCapture.v4lPriority' + '"]').hide();
        $('#conf_cont_video').hide();
      }
    }

    conf_editor_instCapt.validate().length || window.readOnlyMode ? $('#btn_submit_instCapt').attr('disabled', true) : $('#btn_submit_instCapt').attr('disabled', false);
  });


  conf_editor_instCapt.watch('root.instCapture.systemEnable', () => {
    var systemEnable = conf_editor_instCapt.getEditor("root.instCapture.systemEnable").getValue();
    if (systemEnable) {
        discoverInputSources("screen");
    }

  });

  conf_editor_instCapt.watch('root.instCapture.v4lEnable', () => {
    if (VIDEOGRABBER_AVAIL) {
      var videoEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (videoEnable) {
        discoverInputSources("video");
      }
    }
  });

  $('#btn_submit_instCapt').off().on('click', function () {
    requestWriteConfig(conf_editor_instCapt.getValue());
  });

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    var errors = [];

    if (path === "root.grabberV4L2" || path === "root.framegrabber") {
      var editor;
      switch (path) {
        case "root.framegrabber":
          editor = conf_editor_screen;
          break;
        case "root.grabberV4L2":
          editor = conf_editor_video;
          break;
      }

      if (value.cropLeft || value.cropRight) {
        var width = editor.getEditor(path + ".width").getValue();
        if (value.cropLeft + value.cropRight > width) {
          errors.push({
            path: path,
            property: 'maximum',
            message: $.i18n('edt_conf_v4l2_cropWidthValidation_error', width)
          });
        }
      }

      if (value.cropTop || value.cropBottom) {
        var height = editor.getEditor(path + ".height").getValue();
        if (value.cropTop + value.cropBottom > height) {
          errors.push({
            path: path,
            property: 'maximum',
            message: $.i18n('edt_conf_v4l2_cropHeightValidation_error', height)
          });
        }
      }
    }
    return errors;
  });

  function updateCropForWidth(editor, path) {
    var width = editor.getEditor(path+".width").getValue();
    updateJsonEditorRange(editor.getEditor(path), 'cropLeft', 0, width);
    updateJsonEditorRange(editor.getEditor(path), 'cropRight', 0, width);
  }

  function updateCropForHeight(editor, path) {
    var height = editor.getEditor(path + ".height").getValue();
    updateJsonEditorRange(editor.getEditor(path), 'cropTop', 0, height);
    updateJsonEditorRange(editor.getEditor(path), 'cropBottom', 0, height);
  }

  // Screen-Grabber
  conf_editor_screen = createJsonEditor('editor_container_screengrabber', {
    framegrabber: window.schema.framegrabber
  }, true, true);

  conf_editor_screen.on('ready', function () {

    var screenEnable = conf_editor_instCapt.getEditor("root.instCapture.systemEnable").getValue();
    if (screenEnable) {
      discoverInputSources("screen");
    }

    updateCropForWidth(conf_editor_screen, "root.framegrabber");
    updateCropForHeight(conf_editor_screen, "root.framegrabber");
  });

  conf_editor_screen.on('change', function () {
    conf_editor_screen.validate().length || window.readOnlyMode ? $('#btn_submit_screengrabber').attr('disabled', true) : $('#btn_submit_screengrabber').attr('disabled', false);
  });

  conf_editor_screen.watch('root.framegrabber.type', () => {
    var selectedType = conf_editor_screen.getEditor("root.framegrabber.type").getValue();
    filterScreenInputOptions(selectedType);
  });

  conf_editor_screen.watch('root.framegrabber.available_devices', () => {
    var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
    if (deviceSelected === "NONE" || deviceSelected === "") {
      $('#btn_submit_screengrabber').attr('disabled', true);
    }
    else {
      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice(deviceSelected);

      //Update hidden input element
      conf_editor_screen.getEditor("root.framegrabber.device").setValue(deviceProperties.device);

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
          var configuredVideoInput = window.serverConfig.framegrabber.input;
          if ($.inArray(configuredVideoInput, enumVals) != -1) {
            enumDefaultVal = configuredVideoInput;
          }
        }
        updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
          'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_screengrabber').attr('disabled', false);
      }
    }
  });

  conf_editor_screen.watch('root.framegrabber.device_inputs', () => {
    var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
    var videoInputSelected = conf_editor_screen.getEditor("root.framegrabber.device_inputs").getValue();

    //Update hidden input element
    conf_editor_screen.getEditor("root.framegrabber.input").setValue(parseInt(videoInputSelected));

    var addSchemaElements = {};
    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";

    var deviceProperties = getPropertiesOfDevice(deviceSelected);

    var formats = deviceProperties.video_inputs[videoInputSelected].formats;
    var formatIdx = 0;
    /*
    if (formatSelected !== "NONE") {
      formatIdx = formats.findIndex(x => x.format === formatSelected);
    }
    */

    var resolutions = formats[formatIdx].resolutions;
    if (resolutions.length <= 1) {
      addSchemaElements.access = "advanced";
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
        var configuredResolutionText = window.serverConfig.framegrabber.width + "x" + window.serverConfig.framegrabber.height;
        var idx = $.inArray(configuredResolutionText, enumTitelVals)
        if (idx != -1) {
          enumDefaultVal = idx;
        }
      }

      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
    }

    if (!window.readOnlyMode) {
      $('#btn_submit_videograbber').attr('disabled', false);
    }
  });

  conf_editor_screen.watch('root.framegrabber.resolutions', () => {
    var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
    var videoInputSelected = conf_editor_screen.getEditor("root.framegrabber.device_inputs").getValue();
    var resolutionSelected = conf_editor_screen.getEditor("root.framegrabber.resolutions").getValue();

    var addSchemaElements = {};
    var enumVals = [];
    var enumDefaultVal = "";

    var deviceProperties = getPropertiesOfDevice(deviceSelected);

    var formats = deviceProperties.video_inputs[videoInputSelected].formats;
    var formatIdx = 0;
    /*
    if (formatSelected !== "NONE") {
      formatIdx = formats.findIndex(x => x.format === formatSelected);
    }
    */

    //Update hidden resolution related elements
    var width = parseInt(formats[formatIdx].resolutions[resolutionSelected].width);
    conf_editor_screen.getEditor("root.framegrabber.width").setValue(width);

    var height = parseInt(formats[formatIdx].resolutions[resolutionSelected].height);
    conf_editor_screen.getEditor("root.framegrabber.height").setValue(height);

    //Update crop rage depending on selected resolution
    updateCropForWidth(conf_editor_screen, "root.framegrabber");
    updateCropForHeight(conf_editor_screen, "root.framegrabber");

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
        var configuredFps = window.serverConfig.framegrabber.fps;
        if ($.inArray(configuredFps, enumVals) != -1) {
          enumDefaultVal = configuredFps;
        }
      }
      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
    }

    if (!window.readOnlyMode) {
      $('#btn_submit_screengrabber').attr('disabled', false);
    }
  });

  conf_editor_screen.watch('root.framegrabber.framerates', () => {
    //Update hidden fps element
    var fps = 0;
    var framerates = conf_editor_screen.getEditor("root.framegrabber.framerates").getValue();
    if (framerates !== "NONE") {
      fps = parseInt(framerates);
    }

    //Show Frameskipping only when more than 2 fps
    if (fps > 2 && storedAccess === "expert") {
      showVideoInputOptions(["fpsSoftwareDecimation"], true);
    }
    else {
      showVideoInputOptions(["fpsSoftwareDecimation"], false);
    }
    conf_editor_screen.getEditor("root.framegrabber.fps").setValue(fps);
  });


  conf_editor_screen.watch('root.framegrabber.width', () => {
    updateCropForWidth(conf_editor_screen, "root.framegrabber");
  });

  conf_editor_screen.watch('root.framegrabber.height', () => {
    updateCropForHeight(conf_editor_screen, "root.framegrabber");
  });

  function showScreenInputOptions(el, state) {
    for (var i = 0; i < el.length; i++) {
      $('[data-schemapath*="root.framegrabber.' + el[i] + '"]').toggle(state);
    }
  }

  function filterScreenInputOptions(type) {
    //hide specific options for grabbers found
    var grabbers = window.serverInfo.grabbers.available;
    if (grabbers.indexOf(type) > -1) {
      showScreenInputOptions(["width", "height", "pixelDecimation", "display"], true);

      switch (type) {
        case "dispmanx":
          showScreenInputOptions(["pixelDecimation", "display"], false);
          break;
        case "x11":
        case "xcb":
          showScreenInputOptions(["width", "height", "display"], false);
          break;
        case "framebuffer":
          showScreenInputOptions(["display"], false);
          break;
        case "amlogic":
          showScreenInputOptions(["pixelDecimation", "display"], false);
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

  $('#btn_submit_screengrabber').off().on('click', function () {
    var saveOptions = conf_editor_screen.getValue();

    var instCaptOptions = window.serverConfig.instCapture;
    instCaptOptions.systemEnable = true;
    saveOptions.instCapture = instCaptOptions;

    requestWriteConfig(saveOptions);
  });

  // External Input Sources (Video-Grabbers)

  var configuredDevice = "";
  var discoveredInputSources = {};
  var deviceProperties = {};

  if (VIDEOGRABBER_AVAIL) {
    conf_editor_video = createJsonEditor('editor_container_videograbber', {
      grabberV4L2: window.schema.grabberV4L2
    }, true, true);

    conf_editor_video.on('ready', function () {
      var v4lEnable = conf_editor_instCapt.getEditor("root.instCapture.v4lEnable").getValue();
      if (v4lEnable) {
        discoverInputSources("video");
      }
    });

    conf_editor_video.on('change', function () {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      if (!conf_editor_video.validate().length) {

        switch (deviceSelected) {
          case "SELECT":
            showAllVideoInputOptions(conf_editor_video, "grabberV4L2", false);
            break;
          case "NONE":
            break;
          default:
            window.readOnlyMode ? $('#btn_submit_videograbber').attr('disabled', true) : $('#btn_submit_videograbber').attr('disabled', false);
            break;
        }
      }
      else {
        $('#btn_submit_videograbber').attr('disabled', true);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.available_devices', () => {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      if (deviceSelected === "NONE" || deviceSelected === "SELECT" || deviceSelected === "") {
        $('#btn_submit_videograbber').attr('disabled', true);
      }
      else {
        var addSchemaElements = {};
        var enumVals = [];
        var enumTitelVals = [];
        var enumDefaultVal = "";

        var deviceProperties = getPropertiesOfDevice(deviceSelected);

        //Update hidden input element
        conf_editor_video.getEditor("root.grabberV4L2.device").setValue(deviceProperties.device);

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

          updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
            'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
        }

        if (!window.readOnlyMode) {
          $('#btn_submit_videograbber').attr('disabled', false);
        }
      }
    });

    conf_editor_video.watch('root.grabberV4L2.device_inputs', () => {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();

      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice(deviceSelected);
      var formats = deviceProperties.video_inputs[videoInputSelected].formats;

      addSchemaElements.access = "advanced";

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
        updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
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

        updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
          'standard', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_videograbber').attr('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.encoding', () => {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();
      var formatSelected = conf_editor_video.getEditor("root.grabberV4L2.encoding").getValue();

      //Update hidden input element
      conf_editor_video.getEditor("root.grabberV4L2.input").setValue(parseInt(videoInputSelected));

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
        addSchemaElements.access = "advanced";
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

        updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_videograbber').attr('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.resolutions', () => {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      var videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();
      var formatSelected = conf_editor_video.getEditor("root.grabberV4L2.encoding").getValue();
      var resolutionSelected = conf_editor_video.getEditor("root.grabberV4L2.resolutions").getValue();

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
      conf_editor_video.getEditor("root.grabberV4L2.width").setValue(width);

      var height = parseInt(formats[formatIdx].resolutions[resolutionSelected].height);
      conf_editor_video.getEditor("root.grabberV4L2.height").setValue(height);

      //Update crop rage depending on selected resolution
      updateCropForWidth(conf_editor_video, "root.grabberV4L2");
      updateCropForHeight(conf_editor_video, "root.grabberV4L2");

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
        updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
          'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (!window.readOnlyMode) {
        $('#btn_submit_videograbber').attr('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.framerates', () => {
      //Update hidden fps element
      var fps = 0;
      var framerates = conf_editor_video.getEditor("root.grabberV4L2.framerates").getValue();
      if (framerates !== "NONE") {
        fps = parseInt(framerates);
      }

      //Show Frameskipping only when more than 2 fps
      if (fps > 2 && storedAccess === "expert" ) {
        showVideoInputOptions(["fpsSoftwareDecimation"], true);
      }
      else {
        showVideoInputOptions(["fpsSoftwareDecimation"], false);
      }
      conf_editor_video.getEditor("root.grabberV4L2.fps").setValue(fps);
    });

    $('#btn_submit_videograbber').off().on('click', function () {
      var saveOptions = conf_editor_video.getValue();

      var instCaptOptions = window.serverConfig.instCapture;
      instCaptOptions.v4lEnable = true;
      saveOptions.instCapture = instCaptOptions;

      requestWriteConfig(saveOptions);
    });
  }

  //////////////////////////////////////////////////

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_screengrabber");
    if (VIDEOGRABBER_AVAIL) {
      createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_videograbber");
    }
  }

  removeOverlay();

  // build dynamic screen input enum
  var updateScreenSourcesList = function (type, discoveryInfo) {
    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";

    if (jQuery.isEmptyObject(discoveryInfo)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_grabber_discovered_none'));

      conf_editor_screen.getEditor('root.framegrabber').disable();
      showAllVideoInputOptions(conf_editor_screen, "framegrabber", false);
    }
    else {
      for (const device of discoveryInfo) {
        enumVals.push(device.device_name);
      }
      conf_editor_screen.getEditor('root.framegrabber').enable();
    }

    if (enumVals.length > 0) {
      configuredDevice = window.serverConfig.framegrabber.available_devices;
      if ($.inArray(configuredDevice, enumVals) != -1) {
        enumDefaultVal = configuredDevice;
      }

      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, false);
    }
  }

  // build dynamic video input enum
  var updateVideoSourcesList = function (type, discoveryInfo) {
    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";
    var addSelect = false;

    if (jQuery.isEmptyObject(discoveryInfo)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_grabber_discovered_none'));

      conf_editor_video.getEditor('root.grabberV4L2').disable();
      showAllVideoInputOptions(conf_editor_video, "grabberV4L2", false);
    }
    else {
      for (const device of discoveryInfo) {
        enumVals.push(device.device_name);
      }
      conf_editor_video.getEditor('root.grabberV4L2').enable();
      configuredDevice = window.serverConfig.grabberV4L2.available_devices;

      if ($.inArray(configuredDevice, enumVals) != -1) {
        enumDefaultVal = configuredDevice;
      }
      else {
        addSelect = true;
        showAllVideoInputOptions(conf_editor_video, "grabberV4L2", false);
      }
    }

    updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
      'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect);
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

    //console.log("discoveryResult", discoveryResult);
    discoveredInputSources = discoveryResult.video_sources;

    switch (type) {
      case "screen":
        updateScreenSourcesList(type, discoveredInputSources);
        break;
      case "video":
        updateVideoSourcesList(type, discoveredInputSources);
        break;
    }
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

  function showVideoInputOptions(path, elements, state) {
    for (var i = 0; i < elements.length; i++) {
      $('[data-schemapath*="'+ path + '.' + elements[i] + '"]').toggle(state);
    }
  }

  function showAllVideoInputOptions(editor, item, state) {
    var elements = [];
    for (var key in editor.schema.properties[item].properties) {

      if (key !== "available_devices") {
        elements.push(key);
      }
    }
    showVideoInputOptions("root." + item, elements, state);
  }
});
