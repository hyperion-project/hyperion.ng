$(document).ready(function () {

  performTranslation();

  var screenGrabberAvailable = (window.serverInfo.grabbers.screen.available.length !== 0);
  var videoGrabberAvailable = (window.serverInfo.grabbers.video.available.length !== 0);
  const audioGrabberAvailable = (window.serverInfo.grabbers.audio.available.length !== 0);
  var CEC_ENABLED = (jQuery.inArray("cec", window.serverInfo.services) !== -1);

  var conf_editor_video = null;
  var conf_editor_audio = null;
  var conf_editor_screen = null;

  var configuredDevice = "";
  var discoveredInputSources = {};
  var deviceProperties = {};

  // Screen-Grabber
  if (screenGrabberAvailable) {
    $('#conf_cont').append(createRow('conf_cont_screen'));
    $('#conf_cont_screen').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_screengrabber', 'btn_submit_screengrabber', 'panel-system', 'screengrabberPanelId'));
    if (window.showOptHelp) {
      $('#conf_cont_screen').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title"), "screengrabberHelpPanelId"));
    }
  }

  // Video-Grabber
  if (videoGrabberAvailable) {
    $('#conf_cont').append(createRow('conf_cont_video'));
    $('#conf_cont_video').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_videograbber', 'btn_submit_videograbber', 'panel-system', 'videograbberPanelId'));

    if (storedAccess === 'expert') {
      var conf_cont_video_footer = document.getElementById("editor_container_videograbber").nextElementSibling;
      $(conf_cont_video_footer).prepend('<button class="btn btn-primary mdi-24px" id="btn_videograbber_set_defaults" disabled data-toggle="tooltip" data-placement="top" title="' + $.i18n("edt_conf_v4l2_hardware_set_defaults_tip") + '">'
        + '<i class= "fa fa-fw fa-undo" ></i >' + $.i18n("edt_conf_v4l2_hardware_set_defaults") + '</button > ');
    }

    if (window.showOptHelp) {
      $('#conf_cont_video').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title"), "videograbberHelpPanelId"));
    }
  }

  // Audio-Grabber
  if (audioGrabberAvailable) {
    $('#conf_cont').append(createRow('conf_cont_audio'));
    $('#conf_cont_audio').append(createOptPanel('fa-volume', $.i18n("edt_conf_audio_heading_title"), 'editor_container_audiograbber', 'btn_submit_audiograbber', 'panel-system', 'audiograbberPanelId'));

    if (storedAccess === 'expert') {
      const conf_cont_audio_footer = document.getElementById("editor_container_audiograbber").nextElementSibling;
      $(conf_cont_audio_footer).prepend('<button class="btn btn-primary mdi-24px" id="btn_audiograbber_set_effect_defaults" disabled data-toggle="tooltip" data-placement="top" title="' + $.i18n("edt_conf_audio_hardware_set_defaults_tip") + '">'
        + '<i class= "fa fa-fw fa-undo" ></i >' + $.i18n("edt_conf_audio_effect_set_defaults") + '</button > ');
    }

    if (window.showOptHelp) {
      $('#conf_cont_audio').append(createHelpTable(window.schema.grabberAudio.properties, $.i18n("edt_conf_audio_heading_title"), "audiograbberHelpPanelId"));
    }
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
    updateJsonEditorRange(editor, path, 'cropLeft', 0, width);
    updateJsonEditorRange(editor, path, 'cropRight', 0, width);
  }

  function updateCropForHeight(editor, path) {
    var height = editor.getEditor(path + ".height").getValue();
    updateJsonEditorRange(editor, path, 'cropTop', 0, height);
    updateJsonEditorRange(editor, path, 'cropBottom', 0, height);
  }

  // Screen-Grabber
  if (screenGrabberAvailable) {
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
            window.readOnlyMode ? $('#btn_submit_screengrabber').prop('disabled', true) : $('#btn_submit_screengrabber').prop('disabled', false);
            break;
        }
      }
      else {
        $('#btn_submit_screengrabber').prop('disabled', true);
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
        $('#btn_submit_screengrabber').prop('disabled', false);
        showInputOptionsForKey(conf_editor_screen, "framegrabber", "enable", false);
        $('#screengrabberHelpPanelId').hide();
      }

    });

    conf_editor_screen.watch('root.framegrabber.available_devices', () => {

      var deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
      if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
        $('#btn_submit_screengrabber').prop('disabled', true);
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
          enumVals.push(video_input.inputIdx.toString());
          enumTitelVals.push(video_input.name);
        }

        if (enumVals.length > 0) {
          if (deviceSelected === configuredDevice) {
            var configuredVideoInput = window.serverConfig.framegrabber.input.toString();
            if ($.inArray(configuredVideoInput, enumVals) != -1) {
              enumDefaultVal = configuredVideoInput;
            }
          }
          updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
            'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
        }

        if (conf_editor_screen.validate().length && !window.readOnlyMode) {
          $('#btn_submit_screengrabber').prop('disabled', false);
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
        enumVals.push(i.toString());
        var resolutionText = resolutions[i].width + "x" + resolutions[i].height;
        enumTitelVals.push(resolutionText);
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredResolutionText = window.serverConfig.framegrabber.width + "x" + window.serverConfig.framegrabber.height;
          var idx = $.inArray(configuredResolutionText, enumTitelVals);
          if (idx != -1) {
            enumDefaultVal = idx.toString();
          }
        }

        updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (conf_editor_screen.validate().length && !window.readOnlyMode) {
        $('#btn_submit_screengrabber').prop('disabled', false);
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
          enumVals.push(fps[i].toString());
        }
      }

      if (enumVals.length <= 1) {
        addSchemaElements.access = "expert";
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredFps = window.serverConfig.framegrabber.fps.toString();
          if ($.inArray(configuredFps, enumVals) != -1) {
            enumDefaultVal = configuredFps;
          }
        } else if (deviceProperties.hasOwnProperty('default') && !jQuery.isEmptyObject(deviceProperties.default.video_input)) {
          if (deviceProperties.default.video_input.resolution.fps) {
            enumDefaultVal = deviceProperties.default.video_input.resolution.fps;
          }
        }
        updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
          'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (conf_editor_screen.validate().length && !window.readOnlyMode) {
        $('#btn_submit_screengrabber').prop('disabled', false);
      }
    });

    conf_editor_screen.watch('root.framegrabber.framerates', () => {
      //Update hidden fps element
      var fps = 0;
      var framerates = conf_editor_screen.getEditor("root.framegrabber.framerates").getValue();
      if (framerates !== "NONE") {
        fps = parseInt(framerates);
      }
      conf_editor_screen.getEditor("root.framegrabber.fps").setValue(fps);
    });


    $('#btn_submit_screengrabber').off().on('click', function () {
      var saveOptions = conf_editor_screen.getValue();

      var instCaptOptions = window.serverConfig.instCapture;
      instCaptOptions.systemEnable = true;
      saveOptions.instCapture = instCaptOptions;

      requestWriteConfig(saveOptions);
    });
  }

  // External Input Sources (Video-Grabbers)
  if (videoGrabberAvailable) {
    function updateDeviceProperties(deviceProperties, property, key) {
      var properties = {};
      if (deviceProperties) {
        if (deviceProperties.hasOwnProperty(property)) {
          properties = deviceProperties[property];
        }
      }
      updateJsonEditorRange(conf_editor_video, "root.grabberV4L2", key,
        properties.minValue,
        properties.maxValue,
        properties.current,
        properties.step,
        true);

      if (jQuery.isEmptyObject(properties)) {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", key, false);
      } else {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", key, true);
      }
    }

    conf_editor_video = createJsonEditor('editor_container_videograbber', {
      grabberV4L2: window.schema.grabberV4L2
    }, true, true);

    conf_editor_video.on('ready', function () {
      // Trigger conf_editor_video.watch - 'root.grabberV4L2.enable'
      var videoEnable = window.serverConfig.grabberV4L2.enable;
      conf_editor_video.getEditor("root.grabberV4L2.enable").setValue(videoEnable);
    });

    conf_editor_video.on('change', function () {

      // Hide elements not supported by the backend
      if (window.serverInfo.cec.enabled === false || !CEC_ENABLED) {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", "cecDetection", false);
      }

      // Validate the current editor's content
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
            window.readOnlyMode ? $('#btn_submit_videograbber').prop('disabled', true) : $('#btn_submit_videograbber').prop('disabled', false);
            break;
        }
      }
      else {
        $('#btn_submit_videograbber').prop('disabled', true);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.enable', () => {

      var videoEnable = conf_editor_video.getEditor("root.grabberV4L2.enable").getValue();
      if (videoEnable) {
        showInputOptionsForKey(conf_editor_video, "grabberV4L2", "enable", true);
        $('#btn_videograbber_set_defaults').show();
        if (window.showOptHelp) {
          $('#videograbberHelpPanelId').show();
        }
        discoverInputSources("video");
      }
      else {
        $('#btn_submit_videograbber').prop('disabled', false);
        $('#btn_videograbber_set_defaults').hide();
        showInputOptionsForKey(conf_editor_video, "grabberV4L2", "enable", false);
        $('#videograbberHelpPanelId').hide();
      }
    });

    conf_editor_video.watch('root.grabberV4L2.available_devices', () => {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
        $('#btn_submit_videograbber').prop('disabled', true);
        showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
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

        if (deviceProperties.hasOwnProperty('default') && !jQuery.isEmptyObject(deviceProperties.default.properties)) {
          $('#btn_videograbber_set_defaults').prop('disabled', false);
        } else {
          $('#btn_videograbber_set_defaults').prop('disabled', true);
        }

        //If configured device is selected, use the saved values as current values
        if (deviceSelected === configuredDevice) {
          // Only if the device reported properties, use the configured values. In case no properties are presented, the device properties cannot be controlled.
          if (deviceProperties.hasOwnProperty('properties') && !jQuery.isEmptyObject(deviceProperties.properties)) {
            let properties = {
              brightness: { current: window.serverConfig.grabberV4L2.hardware_brightness },
              contrast: { current: window.serverConfig.grabberV4L2.hardware_contrast },
              saturation: { current: window.serverConfig.grabberV4L2.hardware_saturation },
              hue: { current: window.serverConfig.grabberV4L2.hardware_hue }
            };
            deviceProperties.properties = properties;
          }
        }

        updateDeviceProperties(deviceProperties.properties, "brightness", "hardware_brightness");
        updateDeviceProperties(deviceProperties.properties, "contrast", "hardware_contrast");
        updateDeviceProperties(deviceProperties.properties, "saturation", "hardware_saturation");
        updateDeviceProperties(deviceProperties.properties, "hue", "hardware_hue");

        var video_inputs = deviceProperties.video_inputs;
        if (video_inputs.length <= 1) {
          addSchemaElements.access = "expert";
        }

        for (const video_input of video_inputs) {
          enumVals.push(video_input.inputIdx.toString());
          enumTitelVals.push(video_input.name);
        }

        if (enumVals.length > 0) {
          if (deviceSelected === configuredDevice) {
            var configuredVideoInput = window.serverConfig.grabberV4L2.input.toString();
            if ($.inArray(configuredVideoInput, enumVals) != -1) {
              enumDefaultVal = configuredVideoInput;
            }
          }

          updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
            'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false, false);
        }

        if (conf_editor_video.validate().length && !window.readOnlyMode) {
          $('#btn_submit_videograbber').prop('disabled', false);
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
        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
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

        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'standard', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (conf_editor_video.validate().length && !window.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
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
        enumVals.push(i.toString());
        var resolutionText = resolutions[i].width + "x" + resolutions[i].height;
        enumTitelVals.push(resolutionText);
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredResolutionText = window.serverConfig.grabberV4L2.width + "x" + window.serverConfig.grabberV4L2.height;
          var idx = $.inArray(configuredResolutionText, enumTitelVals);
          if (idx != -1) {
            enumDefaultVal = idx.toString();
          }
        }

        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (conf_editor_video.validate().length && !window.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
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
        addSchemaElements.options = { "hidden": true };
      } else {
        fps.sort((a, b) => a - b);
        for (var i = 0; i < fps.length; i++) {
          enumVals.push(fps[i].toString());
        }
      }

      if (enumVals.length <= 1) {
        addSchemaElements.access = "expert";
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          var configuredFps = window.serverConfig.grabberV4L2.fps.toString();
          if ($.inArray(configuredFps, enumVals) != -1) {
            enumDefaultVal = configuredFps;
          }
        }
        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (conf_editor_video.validate().length && !window.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
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
      if (fps > 2) {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", "fpsSoftwareDecimation", true);
      }
      else {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", "fpsSoftwareDecimation", false);
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

    // ------------------------------------------------------------------

    $('#btn_videograbber_set_defaults').off().on('click', function () {
      var deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      var deviceProperties = getPropertiesOfDevice("video", deviceSelected);

      var defaultDeviceProperties = {};
      if (deviceProperties.hasOwnProperty('default')) {
        if (deviceProperties.default.hasOwnProperty('properties')) {
          defaultDeviceProperties = deviceProperties.default.properties;
          if (defaultDeviceProperties.brightness) {
            conf_editor_video.getEditor("root.grabberV4L2.hardware_brightness").setValue(defaultDeviceProperties.brightness);
          }
          if (defaultDeviceProperties.contrast) {
            conf_editor_video.getEditor("root.grabberV4L2.hardware_contrast").setValue(defaultDeviceProperties.contrast);
          }
          if (defaultDeviceProperties.saturation) {
            conf_editor_video.getEditor("root.grabberV4L2.hardware_saturation").setValue(defaultDeviceProperties.saturation);
          }
          if (defaultDeviceProperties.hue) {
            conf_editor_video.getEditor("root.grabberV4L2.hardware_hue").setValue(defaultDeviceProperties.hue);
          }
        }
      }
    });
  }

  // External Input Sources (Audio-Grabbers)
  if (audioGrabberAvailable) {

    conf_editor_audio = createJsonEditor('editor_container_audiograbber', {
      grabberAudio: window.schema.grabberAudio
    }, true, true);

    conf_editor_audio.on('ready', () => {
      // Trigger conf_editor_audio.watch - 'root.grabberAudio.enable'
      const audioEnable = window.serverConfig.grabberAudio.enable;
      conf_editor_audio.getEditor("root.grabberAudio.enable").setValue(audioEnable);
    });

    conf_editor_audio.on('change', () => {

      // Validate the current editor's content
      if (!conf_editor_audio.validate().length) {
        const deviceSelected = conf_editor_audio.getEditor("root.grabberAudio.available_devices").getValue();
        switch (deviceSelected) {
          case "SELECT":
            showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], false);
            break;
          case "NONE":
            showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], false);
            break;
          default:
            window.readOnlyMode ? $('#btn_submit_audiograbber').prop('disabled', true) : $('#btn_submit_audiograbber').prop('disabled', false);
            break;
        }
      }
      else {
        $('#btn_submit_audiograbber').prop('disabled', true);
      }
    });

    // Enable
    conf_editor_audio.watch('root.grabberAudio.enable', () => {

      const audioEnable = conf_editor_audio.getEditor("root.grabberAudio.enable").getValue();
      if (audioEnable)
      {
        showInputOptionsForKey(conf_editor_audio, "grabberAudio", "enable", true);

        $('#btn_audiograbber_set_effect_defaults').show();

        if (window.showOptHelp) {
          $('#audiograbberHelpPanelId').show();
        }

        discoverInputSources("audio");
      }
      else
      {
        $('#btn_submit_audiograbber').prop('disabled', false);
        $('#btn_audiograbber_set_effect_defaults').hide();
        showInputOptionsForKey(conf_editor_audio, "grabberAudio", "enable", false);
        $('#audiograbberHelpPanelId').hide();
      }
    });

    // Available Devices
    conf_editor_audio.watch('root.grabberAudio.available_devices', () => {
      const deviceSelected = conf_editor_audio.getEditor("root.grabberAudio.available_devices").getValue();

      if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
        $('#btn_submit_audiograbber').prop('disabled', true);
        showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], false);
      }
      else
      {
        showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], true);

        const deviceProperties = getPropertiesOfDevice("audio", deviceSelected);

        //Update hidden input element
        conf_editor_audio.getEditor("root.grabberAudio.device").setValue(deviceProperties.device);

        //Enfore configured JSON-editor dependencies
        conf_editor_audio.notifyWatchers("root.grabberAudio.audioEffect");

        //Enable set defaults button
        $('#btn_audiograbber_set_effect_defaults').prop('disabled', false);

        if (conf_editor_audio.validate().length && !window.readOnlyMode) {
          $('#btn_submit_audiograbber').prop('disabled', false);
        }
      }
    });

    $('#btn_submit_audiograbber').off().on('click', function () {
      const saveOptions = conf_editor_audio.getValue();

      const instCaptOptions = window.serverConfig.instCapture;
      instCaptOptions.audioEnable = true;
      saveOptions.instCapture = instCaptOptions;

      requestWriteConfig(saveOptions);
    });

    // ------------------------------------------------------------------

    $('#btn_audiograbber_set_effect_defaults').off().on('click', function () {
      const currentEffect = conf_editor_audio.getEditor("root.grabberAudio.audioEffect").getValue();
      var effectEditor = conf_editor_audio.getEditor("root.grabberAudio." + currentEffect);
      var defaultProperties = effectEditor.schema.defaultProperties;

      var default_values = {};
      for (const item of defaultProperties) {

        default_values[item] = effectEditor.schema.properties[item].default;
      }
      effectEditor.setValue(default_values);
    });
  }

  // ------------------------------------------------------------------

  //////////////////////////////////////////////////

  //create introduction
  if (window.showOptHelp) {
    if (screenGrabberAvailable) {
      createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_screengrabber");
    }
    if (videoGrabberAvailable) {
      createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_videograbber");
    }
    if (audioGrabberAvailable) {
      createHint("intro", $.i18n('conf_grabber_audio_intro'), "editor_container_audiograbber");
    }
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
      updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  };

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
      updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  };

  // build dynamic audio input enum
  const updateAudioSourcesList = function (type, discoveryInfo) {
    const enumVals = [];
    const enumTitelVals = [];
    let enumDefaultVal = "";
    let addSelect = false;

    if (jQuery.isEmptyObject(discoveryInfo)) {
      enumVals.push("NONE");
      enumTitelVals.push($.i18n('edt_conf_grabber_discovered_none'));
    }
    else {
      for (const device of discoveryInfo) {
        enumVals.push(device.device_name);
      }
      conf_editor_audio.getEditor('root.grabberAudio').enable();
      configuredDevice = window.serverConfig.grabberAudio.available_devices;

      if ($.inArray(configuredDevice, enumVals) != -1) {
        enumDefaultVal = configuredDevice;
      }
      else {
        addSelect = true;
      }
    }

    if (enumVals.length > 0) {
      updateJsonEditorSelection(conf_editor_audio, 'root.grabberAudio',
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  };

  async function discoverInputSources(type, params) {
    const result = await requestInputSourcesDiscovery(type, params);

    var discoveryResult;
    if (result && !result.error) {
      discoveryResult = result.info;
    }
    else {
      discoveryResult = {
        "video_sources": [],
        "audio_soruces": []
      };
    }

    switch (type) {
      case "screen":
        discoveredInputSources.screen = discoveryResult.video_sources;
        if (screenGrabberAvailable) {
          updateScreenSourcesList(type, discoveredInputSources.screen);
        }
        break;
      case "video":
        discoveredInputSources.video = discoveryResult.video_sources;
        if (videoGrabberAvailable) {
          updateVideoSourcesList(type, discoveredInputSources.video);
        }
        break;
      case "audio":
        discoveredInputSources.audio = discoveryResult.audio_sources;
        if (audioGrabberAvailable) {
          updateAudioSourcesList(type, discoveredInputSources.audio);
        }
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
