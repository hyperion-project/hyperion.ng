$(document).ready(function () {
  performTranslation();

  var conf_editor_video = null;
  var conf_editor_screen = null;

  // Screen-Grabber
  $('#conf_cont').append(createRow('conf_cont_screen'));
  $('#conf_cont_screen').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_screengrabber', 'btn_submit_screengrabber', 'panel-system', 'screengrabberPanelId'));
  if (window.showOptHelp) {
    $('#conf_cont_screen').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title"), "screengrabberHelpPanelId"));
  }

  // Video-Grabber
  $('#conf_cont').append(createRow('conf_cont_video'));
  $('#conf_cont_video').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_videograbber', 'btn_submit_videograbber', 'panel-system', 'videograbberPanelId'));
  if (window.showOptHelp) {
    $('#conf_cont_video').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title"), "videograbberHelpPanelId"));
  }

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
    var width = editor.getEditor(path + ".width").getValue();
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
    // Trigger conf_editor_screen.watch - 'root.framegrabber.enable'
    var screenEnable = window.serverConfig.framegrabber.enable;
    conf_editor_screen.getEditor("root.framegrabber.enable").setValue(screenEnable);
  });

  conf_editor_screen.on('change', function () {

    if (!conf_editor_screen.validate().length) {
      var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
      switch (deviceSelected) {
        case "SELECT":
          showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
          break;
        case "NONE":
          showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
          break;
        default:
          window.readOnlyMode ? $('#btn_submit_screengrabber').attr('disabled', true) : $('#btn_submit_screengrabber').attr('disabled', false);
          break;
      }
    }
    else {
      $('#btn_submit_screengrabber').attr('disabled', true);
    }
  });

  conf_editor_screen.watch('root.framegrabber.enable', () => {

    var screenEnable = conf_editor_screen.getEditor("root.framegrabber.enable").getValue();
    if (screenEnable) {
      showInputOptionsForKey(conf_editor_screen, "framegrabber", "enable", true);
      if (window.showOptHelp) {
        $('#screengrabberHelpPanelId').show();
      }
      discoverInputSources("screen");
    }
    else {
      $('#btn_submit_screengrabber').attr('disabled', false);
      showInputOptionsForKey(conf_editor_screen, "framegrabber", "enable", false);
      $('#screengrabberHelpPanelId').hide();
    }

  });

  conf_editor_screen.watch('root.framegrabber.available_devices', () => {

    var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
    if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
      $('#btn_submit_screengrabber').attr('disabled', true);
      showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
    }
    else {
      showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], true);
      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice("screen", deviceSelected);

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
            enumDefaultVal = configuredVideoInput.toString();
          }
        }
        updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
          'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (conf_editor_screen.validate().length && !window.readOnlyMode) {
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

    var deviceProperties = getPropertiesOfDevice("screen", deviceSelected);

    var formats = deviceProperties.video_inputs[videoInputSelected].formats;
    var formatIdx = 0;

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
          enumDefaultVal = idx.toString();
        }
      }

      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
    }

    if (conf_editor_screen.validate().length && !window.readOnlyMode) {
      $('#btn_submit_screengrabber').attr('disabled', false);
    }
  });

  conf_editor_screen.watch('root.framegrabber.resolutions', () => {
    var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
    var videoInputSelected = conf_editor_screen.getEditor("root.framegrabber.device_inputs").getValue();
    var resolutionSelected = conf_editor_screen.getEditor("root.framegrabber.resolutions").getValue();

    var addSchemaElements = {};
    var enumVals = [];
    var enumDefaultVal = "";

    var deviceProperties = getPropertiesOfDevice("screen", deviceSelected);

    var formats = deviceProperties.video_inputs[videoInputSelected].formats;
    var formatIdx = 0;

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
          enumDefaultVal = configuredFps.toString();
        }
      }
      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
    }

    if (conf_editor_screen.validate().length && !window.readOnlyMode) {
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
      showInputOptions(["fpsSoftwareDecimation"], true);
    }
    else {
      showInputOptions(["fpsSoftwareDecimation"], false);
    }
    conf_editor_screen.getEditor("root.framegrabber.fps").setValue(fps);
  });


  conf_editor_screen.watch('root.framegrabber.width', () => {
    updateCropForWidth(conf_editor_screen, "root.framegrabber");
  });

  conf_editor_screen.watch('root.framegrabber.height', () => {
    updateCropForHeight(conf_editor_screen, "root.framegrabber");
  });

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

  conf_editor_video = createJsonEditor('editor_container_videograbber', {
    grabberV4L2: window.schema.grabberV4L2
  }, true, true);

  conf_editor_video.on('ready', function () {
    // Trigger conf_editor_video.watch - 'root.grabberV4L2.enable'
    var videoEnable = window.serverConfig.grabberV4L2.enable;
    conf_editor_video.getEditor("root.grabberV4L2.enable").setValue(videoEnable);
  });

  conf_editor_video.on('change', function () {

    if (!conf_editor_video.validate().length) {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      switch (deviceSelected) {
        case "SELECT":
          showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
          break;
        case "NONE":
          showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
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

  conf_editor_video.watch('root.grabberV4L2.enable', () => {

    var videoEnable = conf_editor_video.getEditor("root.grabberV4L2.enable").getValue();
    if (videoEnable) {
      showInputOptionsForKey(conf_editor_video, "grabberV4L2", "enable", true);
      if (window.showOptHelp) {
        $('#videograbberHelpPanelId').show();
      }
      discoverInputSources("video");
    }
    else {
      $('#btn_submit_videograbber').attr('disabled', false);
      showInputOptionsForKey(conf_editor_video, "grabberV4L2", "enable", false);
      $('#videograbberHelpPanelId').hide();
    }
  });

  conf_editor_video.watch('root.grabberV4L2.available_devices', () => {
    var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
    if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
      $('#btn_submit_videograbber').attr('disabled', true);
      showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable","available_devices"], false);
    }
    else {
      showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], true);
      var addSchemaElements = {};
      var enumVals = [];
      var enumTitelVals = [];
      var enumDefaultVal = "";

      var deviceProperties = getPropertiesOfDevice("video", deviceSelected);

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
            enumDefaultVal = configuredVideoInput.toString();
          }
        }

        updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
          'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false, false);
      }

      if (conf_editor_video.validate().length && !window.readOnlyMode) {
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

    var deviceProperties = getPropertiesOfDevice("video", deviceSelected);
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

    if (conf_editor_video.validate().length && !window.readOnlyMode) {
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

    var deviceProperties = getPropertiesOfDevice("video", deviceSelected);

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
          enumDefaultVal = idx.toString();
        }
      }

      updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
        'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
    }

    if (conf_editor_video.validate().length && !window.readOnlyMode) {
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

    var deviceProperties = getPropertiesOfDevice("video", deviceSelected);

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
          enumDefaultVal = configuredFps.toString();
        }
      }
      updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
        'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
    }

    if (conf_editor_video.validate().length && !window.readOnlyMode) {
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
    if (fps > 2 && storedAccess === "expert") {
      showInputOptions(["fpsSoftwareDecimation"], true);
    }
    else {
      showInputOptions(["fpsSoftwareDecimation"], false);
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

  //////////////////////////////////////////////////

  //create introduction
  if (window.showOptHelp) {
    createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_screengrabber");
    createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_videograbber");
  }

  removeOverlay();

  // build dynamic screen input enum
  var updateScreenSourcesList = function (type, discoveryInfo) {
    var enumVals = [];
    var enumTitelVals = [];
    var enumDefaultVal = "";
    var addSelect = false;

    if (jQuery.isEmptyObject(discoveryInfo)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_grabber_discovered_none'));
    }
    else {
      for (const device of discoveryInfo) {
        enumVals.push(device.device_name);
      }
      conf_editor_screen.getEditor('root.framegrabber').enable();
      configuredDevice = window.serverConfig.framegrabber.available_devices;

      if ($.inArray(configuredDevice, enumVals) != -1) {
        enumDefaultVal = configuredDevice;
      }
      else {
        addSelect = true;
      }
    }
    if (enumVals.length > 0) {
      updateJsonEditorSelection(conf_editor_screen.getEditor('root.framegrabber'),
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
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
      }
    }

    if (enumVals.length > 0) {
      updateJsonEditorSelection(conf_editor_video.getEditor('root.grabberV4L2'),
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
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

    switch (type) {
      case "screen":
        discoveredInputSources.screen = discoveryResult.video_sources;
        updateScreenSourcesList(type, discoveredInputSources.screen);
        break;
      case "video":
        discoveredInputSources.video = discoveryResult.video_sources;
        updateVideoSourcesList(type, discoveredInputSources.video);
        break;
    }
  }

  function getPropertiesOfDevice(type, deviceName) {
    deviceProperties = {};
    for (const deviceRecord of discoveredInputSources[type]) {
      if (deviceRecord.device_name === deviceName) {
        deviceProperties = deviceRecord;
        break;
      }
    }
    return deviceProperties;
  }

});

