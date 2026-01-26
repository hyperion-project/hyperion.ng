$(document).ready(function () {

  performTranslation();

  const screenGrabberAvailable = (globalThis.serverInfo.grabbers.screen.available.length !== 0);
  const videoGrabberAvailable = (globalThis.serverInfo.grabbers.video.available.length !== 0);
  const audioGrabberAvailable = (globalThis.serverInfo.grabbers.audio.available.length !== 0);

  let conf_editor_video = null;
  let conf_editor_audio = null;
  let conf_editor_screen = null;

  let configuredDevice = "";
  const discoveredInputSources = {};
  const DDA_INACTIVE_TIMEOUT = 300; // The DDA grabber will not issue updates when no screen activity, define timeout of 5 minutes to avoid off/on blinking

  // Screen-Grabber
  if (screenGrabberAvailable) {
    $('#conf_cont').append(createRow('conf_cont_screen'));
    $('#conf_cont_screen').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_screengrabber', 'btn_submit_screengrabber', 'panel-system', 'screengrabberPanelId'));
    if (globalThis.showOptHelp) {
      $('#conf_cont_screen').append(createHelpTable(globalThis.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title"), "screengrabberHelpPanelId"));
    }
  }

  // Video-Grabber
  if (videoGrabberAvailable) {
    $('#conf_cont').append(createRow('conf_cont_video'));
    $('#conf_cont_video').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_videograbber', 'btn_submit_videograbber', 'panel-system', 'videograbberPanelId'));

    if (storedAccess === 'expert') {
      const conf_cont_video_footer = document.getElementById("editor_container_videograbber").nextElementSibling;
      $(conf_cont_video_footer).prepend('<button class="btn btn-primary mdi-24px" id="btn_videograbber_set_defaults" disabled data-toggle="tooltip" data-placement="top" title="' + $.i18n("edt_conf_v4l2_hardware_set_defaults_tip") + '">'
        + '<i class= "fa fa-fw fa-undo" ></i >' + $.i18n("edt_conf_v4l2_hardware_set_defaults") + '</button > ');
    }

    if (globalThis.showOptHelp) {
      $('#conf_cont_video').append(createHelpTable(globalThis.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title"), "videograbberHelpPanelId"));
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

    if (globalThis.showOptHelp) {
      $('#conf_cont_audio').append(createHelpTable(globalThis.schema.grabberAudio.properties, $.i18n("edt_conf_audio_heading_title"), "audiograbberHelpPanelId"));
    }
  }

  JSONEditor.defaults.custom_validators.push(function (schema, value, path) {
    let errors = [];

    if (path === "root.grabberV4L2" || path === "root.framegrabber") {
      let editor;
      switch (path) {
        case "root.framegrabber":
          editor = conf_editor_screen;
          break;
        case "root.grabberV4L2":
          editor = conf_editor_video;
          break;
      }

      if (value.cropLeft || value.cropRight) {
        const width = editor.getEditor(path + ".width").getValue();
        if (value.cropLeft + value.cropRight > width) {
          errors.push({
            path: path,
            property: 'maximum',
            message: $.i18n('edt_conf_v4l2_cropWidthValidation_error', width)
          });
        }
      }

      if (value.cropTop || value.cropBottom) {
        const height = editor.getEditor(path + ".height").getValue();
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

  // Screen-Grabber
  if (screenGrabberAvailable) {
    conf_editor_screen = createJsonEditor('editor_container_screengrabber', {
      framegrabber: globalThis.schema.framegrabber
    }, true, true);

    conf_editor_screen.on('ready', function () {
      // Trigger conf_editor_screen.watch - 'root.framegrabber.enable'
      const screenEnable = globalThis.serverConfig.framegrabber.enable;
      conf_editor_screen.getEditor("root.framegrabber.enable").setValue(screenEnable);
    });

    conf_editor_screen.on('change', function () {

      if (conf_editor_screen.validate().length) {
        $('#btn_submit_screengrabber').prop('disabled', true);
      }
      else {
        const deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
        switch (deviceSelected) {
          case "SELECT":
            showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
            break;
          case "NONE":
            showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
            break;
          default:
            globalThis.readOnlyMode ? $('#btn_submit_screengrabber').prop('disabled', true) : $('#btn_submit_screengrabber').prop('disabled', false);
            break;
        }
      }
    });

    conf_editor_screen.watch('root.framegrabber.enable', () => {

      const screenEnable = conf_editor_screen.getEditor("root.framegrabber.enable").getValue();
      if (screenEnable) {
        showInputOptionsForKey(conf_editor_screen, "framegrabber", "enable", true);
        if (globalThis.showOptHelp) {
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

      const deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
      if (deviceSelected === "SELECT" || deviceSelected === "NONE" || deviceSelected === "") {
        $('#btn_submit_screengrabber').prop('disabled', true);
        showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], false);
      }
      else {
        showInputOptionsForKey(conf_editor_screen, "framegrabber", ["enable", "available_devices"], true);
        let addSchemaElements = {};
        let enumVals = [];
        let enumTitelVals = [];
        let enumDefaultVal = "";

        const deviceProperties = getPropertiesOfDevice("screen", deviceSelected);

        //Update hidden input element
        conf_editor_screen.getEditor("root.framegrabber.device").setValue(deviceProperties.device);

        const video_inputs = deviceProperties.video_inputs;
        if (video_inputs.length <= 1) {
          addSchemaElements.access = "expert";
        }

        for (const video_input of video_inputs) {
          enumVals.push(video_input.inputIdx.toString());
          enumTitelVals.push(video_input.name);
        }

        if (enumVals.length > 0) {
          if (deviceSelected === configuredDevice) {
            const configuredVideoInput = globalThis.serverConfig.framegrabber.input.toString();
            if ($.inArray(configuredVideoInput, enumVals) != -1) {
              enumDefaultVal = configuredVideoInput;
            }
          }
          updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
            'device_inputs', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
        }

        if (conf_editor_screen.validate().length && !globalThis.readOnlyMode) {
          $('#btn_submit_screengrabber').prop('disabled', false);
        }
      }
    });

    conf_editor_screen.watch('root.framegrabber.device_inputs', () => {
      const deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
      const videoInputSelected = conf_editor_screen.getEditor("root.framegrabber.device_inputs").getValue();

     //Update hidden input element
      conf_editor_screen.getEditor("root.framegrabber.input").setValue(Number.parseInt(videoInputSelected));

      let addSchemaElements = {};
      let enumVals = [];
      let enumTitelVals = [];
      let enumDefaultVal = "";

      const deviceProperties = getPropertiesOfDevice("screen", deviceSelected);
      
      const videoInput = deviceProperties.video_inputs.find(input => input.inputIdx === Number.parseInt(videoInputSelected));
      const formats = videoInput.formats;
      let formatIdx = 0;

      let resolutions = formats[formatIdx].resolutions;
      if (resolutions.length <= 1) {
        addSchemaElements.access = "advanced";
      } else {
        resolutions.sort(compareTwoValues('width', 'height', 'asc'));
      }

      for (let i = 0; i < resolutions.length; i++) {
        enumVals.push(i.toString());
        const resolutionText = resolutions[i].width + "x" + resolutions[i].height;
        enumTitelVals.push(resolutionText);
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredResolutionText = globalThis.serverConfig.framegrabber.width + "x" + globalThis.serverConfig.framegrabber.height;
          const idx = $.inArray(configuredResolutionText, enumTitelVals);
          if (idx != -1) {
            enumDefaultVal = idx.toString();
          }
        }

        updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (conf_editor_screen.validate().length && !globalThis.readOnlyMode) {
        $('#btn_submit_screengrabber').prop('disabled', false);
      }
    });

    conf_editor_screen.watch('root.framegrabber.resolutions', () => {
      const deviceSelected = conf_editor_screen.getEditor("root.framegrabber.available_devices").getValue();
      const videoInputSelected = conf_editor_screen.getEditor("root.framegrabber.device_inputs").getValue();
      const resolutionSelected = conf_editor_screen.getEditor("root.framegrabber.resolutions").getValue();

      let addSchemaElements = {};
      let enumVals = [];
      let enumDefaultVal = "";

      const deviceProperties = getPropertiesOfDevice("screen", deviceSelected);
      const videoInput = deviceProperties.video_inputs.find(input => input.inputIdx == videoInputSelected);
      const formats = videoInput.formats;
      let formatIdx = 0;

      //Update hidden resolution related elements
      const width = Number.parseInt(formats[formatIdx].resolutions[resolutionSelected].width);
      conf_editor_screen.getEditor("root.framegrabber.width").setValue(width);

      const height = Number.parseInt(formats[formatIdx].resolutions[resolutionSelected].height);
      conf_editor_screen.getEditor("root.framegrabber.height").setValue(height);

      //Update crop rage depending on selected resolution
      updateCropForWidth(conf_editor_screen, "root.framegrabber");
      updateCropForHeight(conf_editor_screen, "root.framegrabber");

      const fps = formats[formatIdx].resolutions[resolutionSelected].fps;
      if (fps) {
        fps.sort((a, b) => a - b);
        for (const element of fps) {
          enumVals.push(element.toString());
        }
      } else {
        enumVals.push("NONE");
        addSchemaElements.options = { "hidden": true };
      }

      if (enumVals.length <= 1) {
        addSchemaElements.access = "expert";
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredFps = globalThis.serverConfig.framegrabber.fps.toString();
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

      if (conf_editor_screen.validate().length && !globalThis.readOnlyMode) {
        $('#btn_submit_screengrabber').prop('disabled', false);
      }
    });

    conf_editor_screen.watch('root.framegrabber.framerates', () => {
      //Update hidden fps element
      let fps = 0;
      const framerates = conf_editor_screen.getEditor("root.framegrabber.framerates").getValue();
      if (framerates !== "NONE") {
        fps = Number.parseInt(framerates);
      }
      conf_editor_screen.getEditor("root.framegrabber.fps").setValue(fps);
    });

    $('#btn_submit_screengrabber').off().on('click', function () {
      let saveOptions = conf_editor_screen.getValue();

      // As the DDA grabber will not issue updates when no screen activity, set all instances to a timeout of 5 minutes to avoid off/on blinking
      // until a better design is in place
      if (saveOptions.framegrabber.device === "dda") {
        let instCaptOptions = globalThis.serverConfig.instCapture;
        instCaptOptions.systemEnable = saveOptions.framegrabber.enable;
        instCaptOptions.screenInactiveTimeout = DDA_INACTIVE_TIMEOUT;

        saveOptions.instCapture = instCaptOptions;
        requestWriteConfig(saveOptions, false, getConfiguredInstances());
        return;
      }

      const currentInstance = globalThis.currentHyperionInstance;
      //If an instance exists, enable/disable grabbing in line with the global state
      if (currentInstance !== null && globalThis.serverConfig.instCapture) {
        let instCaptOptions = globalThis.serverConfig.instCapture;
        instCaptOptions.systemEnable = saveOptions.framegrabber.enable;
        instCaptOptions.screenInactiveTimeout = globalThis.schema.instCapture.properties.screenInactiveTimeout.default;

        saveOptions.instCapture = instCaptOptions;
      }
      requestWriteConfig(saveOptions);
    });
  }

  // External Input Sources (Video-Grabbers)
  if (videoGrabberAvailable) {
    function updateDeviceProperties(deviceProperties, property, key) {
      let properties = {};
      if (deviceProperties) {
        if (deviceProperties.hasOwnProperty(property)) {
          properties = deviceProperties[property];
        }
      }
      updateJsonEditorRange(conf_editor_video, "root.grabberV4L2", key,
        properties.minValue,
        properties.maxValue,
        properties.default,
        properties.step,
        true);

      if (jQuery.isEmptyObject(properties)) {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", key, false);
      } else {
        showInputOptionForItem(conf_editor_video, "grabberV4L2", key, true);
      }
    }

    conf_editor_video = createJsonEditor('editor_container_videograbber', {
      grabberV4L2: globalThis.schema.grabberV4L2
    }, true, true);

    conf_editor_video.on('ready', function () {
      // Trigger conf_editor_video.watch - 'root.grabberV4L2.enable'
      const videoEnable = globalThis.serverConfig.grabberV4L2.enable;
      conf_editor_video.getEditor("root.grabberV4L2.enable").setValue(videoEnable);
    });

    conf_editor_video.on('change', function () {

      // Validate the current editor's content
      if (conf_editor_video.validate().length) {
        $('#btn_submit_videograbber').prop('disabled', true);
      }
      else {
        const deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
        switch (deviceSelected) {
          case "SELECT":
            showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
            break;
          case "NONE":
            showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
            break;
          default:
            globalThis.readOnlyMode ? $('#btn_submit_videograbber').prop('disabled', true) : $('#btn_submit_videograbber').prop('disabled', false);
            break;
        }
      }
    });

    conf_editor_video.watch('root.grabberV4L2.enable', () => {

      const videoEnable = conf_editor_video.getEditor("root.grabberV4L2.enable").getValue();
      if (videoEnable) {
        showInputOptionsForKey(conf_editor_video, "grabberV4L2", "enable", true);
        $('#btn_videograbber_set_defaults').show();
        if (globalThis.showOptHelp) {
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
      const editor = conf_editor_video.getEditor("root.grabberV4L2.available_devices");
      const deviceSelected = editor.getValue();
      const invalidSelections = ["SELECT", "NONE", ""];

      if (invalidSelections.includes(deviceSelected)) {
        $('#btn_submit_videograbber').prop('disabled', true);
        showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], false);
        return;
      }

      showInputOptionsForKey(conf_editor_video, "grabberV4L2", ["enable", "available_devices"], true);

      const deviceProperties = getPropertiesOfDevice("video", deviceSelected);
      conf_editor_video.getEditor("root.grabberV4L2.device").setValue(deviceProperties.device);

      const defaultProperties = deviceProperties.default?.properties ?? {};
      const hasDefaults = Object.keys(defaultProperties).length > 0;
      $('#btn_videograbber_set_defaults').prop('disabled', !hasDefaults);

      const isConfiguredDevice = (deviceSelected === configuredDevice);
      const { grabberV4L2 } = globalThis.serverConfig;
      const currentProps = deviceProperties.properties;

      const propMappings = {
        brightness: 'hardware_brightness',
        contrast: 'hardware_contrast',
        saturation: 'hardware_saturation',
        hue: 'hardware_hue'
      };

      for (const prop in propMappings) {
        if (hasDefaults && currentProps[prop] && Object.hasOwn(defaultProperties, prop)) {
          currentProps[prop].default = defaultProperties[prop];
        }
        // Ensure min,max and step values are set inline with the selected grabber to ensure valid input
        updateDeviceProperties(currentProps, prop, [propMappings[prop]]);

        let currentValue = 0;
        if (isConfiguredDevice) {
          currentValue = globalThis.serverConfig.grabberV4L2[propMappings[prop]];
        } else if (hasDefaults && currentProps[prop]?.hasOwnProperty('default')) {
          currentValue = currentProps[prop].default;
        }

        if (currentValue !== undefined) {
          conf_editor_video.getEditor("root.grabberV4L2." + propMappings[prop]).setValue(currentValue);
        }
      }

      const { video_inputs = [] } = deviceProperties;
      
      const addSchemaElements = {};

      if (video_inputs.length <= 1) {
        addSchemaElements.access = "expert";
      }

      const enumVals = video_inputs.map(input => input.inputIdx.toString());
      const enumTitelVals = video_inputs.map(input => input.name);

      if (enumVals.length > 0) {
        let enumDefaultVal = "";
        if (isConfiguredDevice) {
          const configuredInput = grabberV4L2.input.toString();
          if (enumVals.includes(configuredInput)) {
            enumDefaultVal = configuredInput;
          }
        }

        updateJsonEditorSelection(
          conf_editor_video, 'root.grabberV4L2', 'device_inputs',
          addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false, false
        );
      }

      const isValid = conf_editor_video.validate().length === 0;
      if (isValid && !globalThis.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.device_inputs', () => {
      const deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      const videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();

      let addSchemaElements = {};
      let enumVals = [];
      let enumTitelVals = [];
      let enumDefaultVal = "";

      const deviceProperties = getPropertiesOfDevice("video", deviceSelected);
      const formats = deviceProperties.video_inputs[videoInputSelected].formats;

      addSchemaElements.access = "advanced";

      for (const element of formats) {
        if (element.format) {
          enumVals.push(element.format);
          enumTitelVals.push(element.format.toUpperCase());
        }
        else {
          enumVals.push("NONE");
        }
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredEncoding = globalThis.serverConfig.grabberV4L2.encoding;
          if ($.inArray(configuredEncoding, enumVals) != -1) {
            enumDefaultVal = configuredEncoding;
          }
        }
        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'encoding', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      const standards = deviceProperties.video_inputs[videoInputSelected].standards;
      if (standards) {
        enumVals = standards;
      } else {
        enumVals.push("NONE");
        addSchemaElements.options = { "hidden": true };
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredStandard = globalThis.serverConfig.grabberV4L2.standard;
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
      const deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      const videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();
      const formatSelected = conf_editor_video.getEditor("root.grabberV4L2.encoding").getValue();

      //Update hidden input element
      conf_editor_video.getEditor("root.grabberV4L2.input").setValue(Number.parseInt(videoInputSelected));

      let addSchemaElements = {};
      let enumVals = [];
      let enumTitelVals = [];
      let enumDefaultVal = "";

      const deviceProperties = getPropertiesOfDevice("video", deviceSelected);

      const formats = deviceProperties.video_inputs[videoInputSelected].formats;
      let formatIdx = 0;
      if (formatSelected !== "NONE") {
        formatIdx = formats.findIndex(x => x.format === formatSelected);
      }

      const resolutions = formats[formatIdx].resolutions;
      if (resolutions.length <= 1) {
        addSchemaElements.access = "advanced";
      } else {
        resolutions.sort(compareTwoValues('width', 'height', 'asc'));
      }

      for (let i = 0; i < resolutions.length; i++) {
        enumVals.push(i.toString());
        const resolutionText = resolutions[i].width + "x" + resolutions[i].height;
        enumTitelVals.push(resolutionText);
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredResolutionText = globalThis.serverConfig.grabberV4L2.width + "x" + globalThis.serverConfig.grabberV4L2.height;
          const idx = $.inArray(configuredResolutionText, enumTitelVals);
          if (idx != -1) {
            enumDefaultVal = idx.toString();
          }
        }

        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'resolutions', addSchemaElements, enumVals, enumTitelVals, enumDefaultVal, false);
      }

      if (conf_editor_video.validate().length && !globalThis.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.resolutions', () => {
      const deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      const videoInputSelected = conf_editor_video.getEditor("root.grabberV4L2.device_inputs").getValue();
      const formatSelected = conf_editor_video.getEditor("root.grabberV4L2.encoding").getValue();
      const resolutionSelected = conf_editor_video.getEditor("root.grabberV4L2.resolutions").getValue();

      let addSchemaElements = {};
      let enumVals = [];
      let enumDefaultVal = "";

      const deviceProperties = getPropertiesOfDevice("video", deviceSelected);
      const formats = deviceProperties.video_inputs[videoInputSelected].formats;
      let formatIdx = 0;
      if (formatSelected !== "NONE") {
        formatIdx = formats.findIndex(x => x.format === formatSelected);
      }

      //Update hidden resolution related elements
      const width = Number.parseInt(formats[formatIdx].resolutions[resolutionSelected].width);
      conf_editor_video.getEditor("root.grabberV4L2.width").setValue(width);

      const height = Number.parseInt(formats[formatIdx].resolutions[resolutionSelected].height);
      conf_editor_video.getEditor("root.grabberV4L2.height").setValue(height);

      //Update crop rage depending on selected resolution
      updateCropForWidth(conf_editor_video, "root.grabberV4L2");
      updateCropForHeight(conf_editor_video, "root.grabberV4L2");

      const fps = formats[formatIdx].resolutions[resolutionSelected].fps;
      if (fps) {
        fps.sort((a, b) => a - b);
        for (const element of fps) {
          enumVals.push(element.toString());
        }
      } else {
        addSchemaElements.options = { "hidden": true };
      }

      if (enumVals.length <= 1) {
        addSchemaElements.access = "expert";
      }

      if (enumVals.length > 0) {
        if (deviceSelected === configuredDevice) {
          const configuredFps = globalThis.serverConfig.grabberV4L2.fps.toString();
          if ($.inArray(configuredFps, enumVals) != -1) {
            enumDefaultVal = configuredFps;
          }
        }
        updateJsonEditorSelection(conf_editor_video, 'root.grabberV4L2',
          'framerates', addSchemaElements, enumVals, [], enumDefaultVal, false);
      }

      if (conf_editor_video.validate().length && !globalThis.readOnlyMode) {
        $('#btn_submit_videograbber').prop('disabled', false);
      }
    });

    conf_editor_video.watch('root.grabberV4L2.framerates', () => {
      //Update hidden fps element
      let fps = 0;
      const framerates = conf_editor_video.getEditor("root.grabberV4L2.framerates").getValue();
      if (framerates !== "NONE") {
        fps = Number.parseInt(framerates);
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
      let saveOptions = conf_editor_video.getValue();

      const currentInstance = globalThis.currentHyperionInstance;
      //If an instance exists, enable/disable grabbing in line with the global state
      if (currentInstance !== null && globalThis.serverConfig.instCapture) {
        let instCaptOptions = globalThis.serverConfig.instCapture;
        instCaptOptions.v4lEnable = saveOptions.grabberV4L2.enable;
        saveOptions.instCapture = instCaptOptions;
      }

      requestWriteConfig(saveOptions);
    });

    // ------------------------------------------------------------------

    $('#btn_videograbber_set_defaults').off().on('click', function () {
      const deviceSelected = conf_editor_video.getEditor("root.grabberV4L2.available_devices").getValue();
      const deviceProperties = getPropertiesOfDevice("video", deviceSelected);

      let defaultDeviceProperties = {};
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
      grabberAudio: globalThis.schema.grabberAudio
    }, true, true);

    conf_editor_audio.on('ready', () => {
      // Trigger conf_editor_audio.watch - 'root.grabberAudio.enable'
      const audioEnable = globalThis.serverConfig.grabberAudio.enable;
      conf_editor_audio.getEditor("root.grabberAudio.enable").setValue(audioEnable);
    });

    conf_editor_audio.on('change', () => {

      // Validate the current editor's content
      if (conf_editor_audio.validate().length) {
        $('#btn_submit_audiograbber').prop('disabled', true);
      }
      else {
        const deviceSelected = conf_editor_audio.getEditor("root.grabberAudio.available_devices").getValue();
        switch (deviceSelected) {
          case "SELECT":
            showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], false);
            break;
          case "NONE":
            showInputOptionsForKey(conf_editor_audio, "grabberAudio", ["enable", "available_devices"], false);
            break;
          default:
            globalThis.readOnlyMode ? $('#btn_submit_audiograbber').prop('disabled', true) : $('#btn_submit_audiograbber').prop('disabled', false);
            break;
        }
      }
    });

    // Enable
    conf_editor_audio.watch('root.grabberAudio.enable', () => {

      const audioEnable = conf_editor_audio.getEditor("root.grabberAudio.enable").getValue();
      if (audioEnable)
      {
        showInputOptionsForKey(conf_editor_audio, "grabberAudio", "enable", true);

        $('#btn_audiograbber_set_effect_defaults').show();

        if (globalThis.showOptHelp) {
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

        if (conf_editor_audio.validate().length && !globalThis.readOnlyMode) {
          $('#btn_submit_audiograbber').prop('disabled', false);
        }
      }
    });

    $('#btn_submit_audiograbber').off().on('click', function () {
      let saveOptions = conf_editor_audio.getValue();

      const currentInstance = globalThis.currentHyperionInstance;
      //If an instance exists, enable/disable grabbing in line with the global state
      if (currentInstance !== null && globalThis.serverConfig.instCapture) {
        let instCaptOptions = globalThis.serverConfig.instCapture;
        instCaptOptions.audioEnable = saveOptions.grabberAudio.enable;
        saveOptions.instCapture = instCaptOptions;
      }

      requestWriteConfig(saveOptions);
    });

    // ------------------------------------------------------------------

    $('#btn_audiograbber_set_effect_defaults').off().on('click', function () {
      const currentEffect = conf_editor_audio.getEditor("root.grabberAudio.audioEffect").getValue();
      let effectEditor = conf_editor_audio.getEditor("root.grabberAudio." + currentEffect);
      const defaultProperties = effectEditor.schema.defaultProperties;

      let default_values = {};
      for (const item of defaultProperties) {

        default_values[item] = effectEditor.schema.properties[item].default;
      }
      effectEditor.setValue(default_values);
    });
  }

  // ------------------------------------------------------------------

  //////////////////////////////////////////////////

  //create introduction
  if (globalThis.showOptHelp) {
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
  const updateScreenSourcesList = function (type, discoveryInfo) {
    let enumVals = [];
    let enumTitelVals = [];
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
      conf_editor_screen.getEditor('root.framegrabber').enable();
      configuredDevice = globalThis.serverConfig.framegrabber.available_devices;

      if ($.inArray(configuredDevice, enumVals) == -1) {
        addSelect = true;
      }
      else {
        enumDefaultVal = configuredDevice;
      }
    }
    if (enumVals.length > 0) {
      updateJsonEditorSelection(conf_editor_screen, 'root.framegrabber',
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  };

  // build dynamic video input enum
  const updateVideoSourcesList = function (type, discoveryInfo) {
    let enumVals = [];
    let enumTitelVals = [];
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
      conf_editor_video.getEditor('root.grabberV4L2').enable();
      configuredDevice = globalThis.serverConfig.grabberV4L2.available_devices;

      if ($.inArray(configuredDevice, enumVals) == -1) {
        addSelect = true;
      }
      else {
        enumDefaultVal = configuredDevice;
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
      configuredDevice = globalThis.serverConfig.grabberAudio.available_devices;

      if ($.inArray(configuredDevice, enumVals) == -1) {
        addSelect = true;
      }
      else {
        enumDefaultVal = configuredDevice;
      }
    }

    if (enumVals.length > 0) {
      updateJsonEditorSelection(conf_editor_audio, 'root.grabberAudio',
        'available_devices', {}, enumVals, enumTitelVals, enumDefaultVal, addSelect, false);
    }
  };

  async function discoverInputSources(type, params) {
    const result = await requestInputSourcesDiscovery(type, params);

    let discoveryResult;
    if (result && !result.error) {
      discoveryResult = result.info;
    }
    else {
      discoveryResult = {
        "video_sources": [],
        "audio_sources": []
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
    let props = {};
    const sourceList = discoveredInputSources[type] || [];
    for (const deviceRecord of sourceList) {
      if (deviceRecord.device_name === deviceName) {
        // Deep copy to prevent modifying the original object in discoveredInputSources
        props = structuredClone(deviceRecord);
        break;
      }
    }
    return props;
  }

});

function updateCropForWidth(editor, path) {
  const width = editor.getEditor(path + ".width").getValue();
  updateJsonEditorRange(editor, path, 'cropLeft', 0, width);
  updateJsonEditorRange(editor, path, 'cropRight', 0, width);
}

function updateCropForHeight(editor, path) {
  const height = editor.getEditor(path + ".height").getValue();
  updateJsonEditorRange(editor, path, 'cropTop', 0, height);
  updateJsonEditorRange(editor, path, 'cropBottom', 0, height);
}