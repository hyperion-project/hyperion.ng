$(document).ready(function () {

  const DEFAULT_CPCOLOR = '#B500FF';
  const BG_PRIORITY = 254;

  // Perform initial translation setup
  performTranslation();

  // Check if the effect engine is enabled
  const EFFECTENGINE_ENABLED = (jQuery.inArray("effectengine", window.serverInfo.services) !== -1);

  // Update the list of Hyperion instances
  updateHyperionInstanceListing();

  // Initialize variables
  let oldEffects = [];
  const mappingList = window.serverSchema.properties.color.properties.imageToLedMappingType.enum;
  let uiInputDuration_s = 0; // Endless
  let rgb = { r: 255, g: 0, b: 0 };
  let lastImgData = "";
  let lastFileName = "";

  // Create initial HTML structure
  createTable('ssthead', 'sstbody', 'sstcont');
  $('.ssthead').html(createTableRow([$.i18n('remote_input_origin'), $.i18n('remote_input_owner'), $.i18n('remote_input_priority'), $.i18n('remote_input_status')], true, true));
  createTable('crthead', 'crtbody', 'adjust_content', true);

  // Create introduction hints if the help option is enabled
  if (window.showOptHelp) {
    createHint("intro", $.i18n('remote_color_intro', $.i18n('remote_losthint')), "color_intro");
    createHint("intro", $.i18n('remote_input_intro', $.i18n('remote_losthint')), "sstcont");
    createHint("intro", $.i18n('remote_adjustment_intro', $.i18n('remote_losthint')), "adjust_content");
    createHint("intro", $.i18n('remote_components_intro', $.i18n('remote_losthint')), "comp_intro");
    createHint("intro", $.i18n('remote_maptype_intro', $.i18n('remote_losthint')), "maptype_intro");
    createHint("intro", $.i18n('remote_videoMode_intro', $.i18n('remote_losthint')), "videomode_intro");
  }

  // Hide color effect table and reset color button if current instance is not running
  if (!isCurrentInstanceRunning()) {
    $("#color_effect_table").hide();
    $("#reset_color").hide();
    removeOverlay();
    return;
  }

  // Function to send the selected effect
  function sendEffect() {
    const effect = $("#effect_select").val();
    if (effect !== "__none__") {
      requestPriorityClear();
      $(window.hyperion).one("cmd-clear", function () {
        setTimeout(function () { requestPlayEffect(effect, uiInputDuration_s); }, 100);
      });
    }
  }

  // Function to send the selected color
  function sendColor() {
    requestSetColor(rgb.r, rgb.g, rgb.b, uiInputDuration_s);
  }

  // Update the channel adjustments
  function updateChannelAdjustments() {
    if (!window.serverInfo.adjustment || window.serverInfo.adjustment.length === 0) return;

    $('.crtbody').html("");
    const sColor = sortProperties(window.serverSchema.properties.color.properties.channelAdjustment.items.properties);
    const values = window.serverInfo.adjustment[0];

    for (const key in sColor) {
      if (sColor[key].key !== "id" && sColor[key].key !== "leds") {
        const title = `<label for="cr_${sColor[key].key}">${$.i18n(sColor[key].title)}</label>`;
        let property;
        const value = values[sColor[key].key];

        // Handle array type adjustments
        if (sColor[key].type === "array") {
          property = `<div id="cr_${sColor[key].key}" class="input-group colorpicker-component">
                        <input type="text" class="form-control" />
                        <span class="input-group-addon"><i></i></span>
                      </div>`;
          $('.crtbody').append(createTableRow([title, property], false, true));
          createCP(`cr_${sColor[key].key}`, value, function (rgb, hex, e) {
            const elementName = e.target.id.substr(e.target.id.indexOf("_") + 1);
            requestAdjustment(elementName, [rgb.r,rgb.g,rgb.b]);
          });
        }
        // Handle boolean type adjustments
        else if (sColor[key].type === "boolean") {
          property = `<div class="checkbox">
                        <input id="cr_${sColor[key].key}" type="checkbox" ${value ? "checked" : ""}/>
                        <label></label>
                      </div>`;
          $('.crtbody').append(createTableRow([title, property], false, true));

          $('#cr_' + sColor[key].key).off().on('change', function (e) {
            const elementName = e.target.id.substr(e.target.id.indexOf("_") + 1);
            requestAdjustment(elementName, e.currentTarget.checked);
          });
        }
        // Handle number type adjustments
        else {
          if (["brightness", "brightnessCompensation", "backlightThreshold", "saturationGain", "brightnessGain", "temperature"].includes(sColor[key].key)) {
            property = `<input id="cr_${sColor[key].key}" type="number" class="form-control" 
                          min="${sColor[key].minimum}" max="${sColor[key].maximum}" step="${sColor[key].step}" value="${value}" />`;
            if (sColor[key].append && sColor[key].append !== "") {
              property = `<div class="input-group">${property}<span class="input-group-addon">${$.i18n(sColor[key].append)}</span></div>`;
            }
          } else {
            property = `<input id="cr_${sColor[key].key}" type="number" class="form-control" min="0.1" max="4.0" step="0.1" value="${value}" />`;
          }

          $('.crtbody').append(createTableRow([title, property], false, true));
          $('#cr_' + sColor[key].key).off().on('change', function (e) {
            const elementName = e.target.id.substr(e.target.id.indexOf("_") + 1);
            const value = valValue(this.id, this.value, this.min, this.max);
            requestAdjustment(elementName, value);
          });
        }
      }
    }
  }

  // Update input select options based on priorities
  function updateInputSelect() {
    // Clear existing elements
    $('.sstbody').empty();

    const prios = window.serverInfo.priorities;
    let clearAll = false;

    // Iterate over priorities
    for (let i = 0; i < prios.length; i++) {
      let origin = prios[i].origin ? prios[i].origin : "System";
      const [originName, ip] = origin.split("@");
      origin = originName;

      let {
        owner,
        active,
        visible,
        priority,
        componentId,
        duration_ms 
      } = prios[i];

      const remoteInputDuration_s = duration_ms / 1000;
      let value = "0,0,0";
      let btnType = "default";
      let btnText = $.i18n('remote_input_setsource_btn');
      let btnState = "enabled";

      if (active) btnType = "primary";
      if (priority > BG_PRIORITY) continue;
      if (priority < BG_PRIORITY && ["EFFECT", "COLOR", "IMAGE"].includes(componentId)) clearAll = true;

      if (visible) {
        btnState = "disabled";
        btnType = "success";
        btnText = $.i18n('remote_input_sourceactiv_btn');
      }

      if (ip) {
        origin += `<br/><span style="font-size:80%; color:grey;">${$.i18n('remote_input_ip')} ${ip}</span>`;
      }

      if ("value" in prios[i]) value = prios[i].value.RGB;

      // Determine owner description based on component ID
      let ownerText = owner;

      switch (componentId) {
        case "EFFECT":
          ownerText = $.i18n('remote_effects_label_effects') + " " + owner;
          break;
        case "COLOR":
          ownerText = $.i18n('remote_color_label_color') + ' ' +
            `<div style="width:18px; height:18px; border-radius:20px; margin-bottom:-4px; 
                 border:1px grey solid; background-color: rgb(${value}); display:inline-block" 
                 title="RGB: (${value})"></div>`;
          break;
        case "IMAGE":
          ownerText = $.i18n('remote_effects_label_picture') + (owner ? `  ${owner}` : "");
          break;
        case "GRABBER":
        case "V4L":
        case "AUDIO":        
          ownerText = `${$.i18n("general_comp_" + componentId)}: (${owner})`;
          break;
        case "BOBLIGHTSERVER":
        case "FLATBUFSERVER":
        case "PROTOSERVER":       
          ownerText = `${$.i18n("general_comp_" + componentId)})`;
          break;
      }

      if (remoteInputDuration_s > 0 && !["GRABBER", "FLATBUFSERVER", "PROTOSERVER"].includes(componentId)) {
          ownerText += `<br/><span style="font-size:80%; color:grey;">${$.i18n('remote_input_duration')} 
                          ${remoteInputDuration_s.toFixed(0)}${$.i18n('edt_append_s')}</span>`;
      }

      if (!remoteInputDuration_s || remoteInputDuration_s > 0 ) {
        // Create buttons
        let btn = `<button id="srcBtn${i}" type="button" ${btnState} class="btn btn-${btnType} btn_input_selection" 
                       onclick="requestSetSource(${priority});">${btnText}</button>`;

        if (["EFFECT", "COLOR", "IMAGE"].includes(componentId) && priority < BG_PRIORITY) {
          btn += `<button type="button" class="btn btn-sm btn-danger" style="margin-left:10px;" 
                        onclick="requestPriorityClear(${priority});"><i class="fa fa-close"></i></button>`;
        }

        if (btnType !== 'default') {
          $('.sstbody').append(createTableRow([origin, ownerText, priority, btn], false, true));
        }
      }
    }

    // Auto-select and Clear All buttons
    const autoColor = window.serverInfo.priorities_autoselect ? "btn-success" : "btn-danger";
    const autoState = window.serverInfo.priorities_autoselect ? "disabled" : "enabled";
    const autoText = window.serverInfo.priorities_autoselect ? $.i18n('general_btn_on') : $.i18n('general_btn_off');
    const callState = clearAll ? "enabled" : "disabled";

    $('#auto_btn').html(`
        <button id="srcBtnAuto" type="button" ${autoState} class="btn ${autoColor}" style="margin-right:5px; display:inline-block;" 
        onclick="requestSetSource('auto');">${$.i18n('remote_input_label_autoselect')} (${autoText})</button>
    `);

    $('#auto_btn').append(`
        <button type="button" ${callState} class="btn btn-danger" style="display:inline-block;" 
        onclick="requestClearAll();">${$.i18n('remote_input_clearall')}</button>
    `);

    // Adjust button widths
    let maxWidth = 100;
    $('.btn_input_selection').each(function () {
      if ($(this).innerWidth() > maxWidth) maxWidth = $(this).innerWidth();
    });
    $('.btn_input_selection').css("min-width", maxWidth + "px");
  }


  function updateLedMapping() {
    const mapping = window.serverInfo.imageToLedMappingType;

    // Clear existing mappings
    $('#mappingsbutton').empty();

    mappingList.forEach((mapType) => {
      const btnStyle = (mapping === mapType) ? 'btn-success' : 'btn-primary';
      const btnText = $.i18n(`remote_maptype_label_${mapType}`);

      $('#mappingsbutton').append(`
      <button type="button" id="lmBtn_${mapType}" 
              class="btn ${btnStyle}" 
              style="margin:3px;min-width:200px"
              onclick="requestMappingType('${mapType}')">
        ${btnText}
      </button><br/>
    `);
    });
  }

  function initComponents() {
    const components = window.comps;
    const hyperionEnabled = components.some(comp => comp.name === "ALL" && comp.enabled);

    components.forEach((comp) => {
      // Skip if component is disabled or not relevant
      if (shouldSkipComponent(comp)) return;

      const compBtnId = `comp_btn_${comp.name}`;
      const checkedStatus = comp.enabled ? "checked" : "";
      const componentHtml = `
      <span style="display:block;margin:3px">
        <label class="checkbox-inline">
          <input id="${compBtnId}" ${checkedStatus} type="checkbox"
                 data-toggle="toggle"
                 data-onstyle="success"
                 data-name="${comp.name}"
                 data-on="${$.i18n('general_btn_on')}"
                 data-off="${$.i18n('general_btn_off')}">
          ${$.i18n('general_comp_' + comp.name)}
        </label>
      </span>
    `;

      // Append component toggle button if not already created
      if (!$(`#${compBtnId}`).length) {
        $('#componentsbutton').append(componentHtml);
        $(`#${compBtnId}`).bootstrapToggle();
        $(`#${compBtnId}`).bootstrapToggle(hyperionEnabled ? "enable" : "disable");
        $(`#${compBtnId}`).on("change", e => {
          const compName = $(e.currentTarget).data("name");
          requestSetComponentState(compName, e.currentTarget.checked);
        });
      }
    });
  }

  function shouldSkipComponent(comp) {
    // Define conditions to skip certain components
    const skipConditions = {
      "ALL": false,
      "FORWARDER": window.currentHyperionInstance !== window.serverConfig.forwarder.instance,
      "GRABBER": !window.serverConfig.framegrabber.enable,
      "V4L": !window.serverConfig.grabberV4L2.enable,
      "AUDIO": !window.serverConfig.grabberAudio.enable
    };

    return skipConditions[comp.name] || comp.name === "ALL";
  }

  function updateComponent(component) {
    if (component.name === "ALL") {
      updateAllComponents(component.enabled);
    } else {
      updateSingleComponent(component);
    }
  }

  function updateAllComponents(enabled) {
    window.comps.forEach((comp) => {
      if (comp.name === "ALL") return;

      const compBtnId = `comp_btn_${comp.name}`;
      const toggle = $(`#${compBtnId}`).bootstrapToggle();

      if (!enabled) {
        toggle.bootstrapToggle("off");
        toggle.bootstrapToggle("disable");
      } else {
        toggle.bootstrapToggle("enable");
        updateSingleComponent(comp);
      }
    });
  }

  function updateSingleComponent(component) {
    const compBtnId = `comp_btn_${component.name}`;
    const toggle = $(`#${compBtnId}`).bootstrapToggle();

    if (component.enabled !== $(`#${compBtnId}`).prop("checked")) {
      toggle.bootstrapToggle(component.enabled ? "on" : "off");
    }
  }

  // Update Effect List
  function updateEffectlist() {
    const newEffects = window.serverInfo.effects;

    if (newEffects.length !== oldEffects.length) {
      $('#effect_select').html('<option value="__none__"></option>');
      const usrEffArr = [];
      const sysEffArr = [];

      newEffects.forEach(effect => {
        const effectName = effect.name;
        const effectArr = effect.file.startsWith(":") ? sysEffArr : usrEffArr;
        effectArr.push(effectName);
      });

      $('#effect_select')
        .append(createSel(usrEffArr, $.i18n('remote_optgroup_usreffets')))
        .append(createSel(sysEffArr, $.i18n('remote_optgroup_syseffets')));

      oldEffects = newEffects;
    }
  }

  // Update Video Mode
  function updateVideoMode() {
    const videoModes = ["2D", "3DSBS", "3DTAB"];
    const currVideoMode = window.serverInfo.videomode;

    $('#videomodebtns').empty();
    videoModes.forEach(mode => {
      const btnStyle = currVideoMode === mode ? 'btn-success' : 'btn-primary';
      const buttonHtml = `<button type="button" id="vModeBtn_${mode}" class="btn ${btnStyle}" 
                          style="margin:3px;min-width:200px" 
                          onclick="requestVideoMode('${mode}')">
                          ${$.i18n('remote_videoMode_' + mode)}
                        </button><br/>`;

      $('#videomodebtns').append(buttonHtml);
    });
  }

  // Initialize Color Picker and Effects
  function initColorPickerAndEffects() {
    let cpcolor = getStorage('rmcpcolor') || DEFAULT_CPCOLOR;
    if (cpcolor) rgb = hexToRgb(cpcolor);

    const storedDuration = getStorage('rmduration');
    if (storedDuration) {
      uiInputDuration_s = Number(storedDuration);
      $("#remote_duration").val(uiInputDuration_s);
      updateDurationPlaceholder(); // Update the placeholder for "Endless"
    }

    createCP('cp2', cpcolor, function (rgbT, hex) {
      rgb = rgbT;
      sendColor();
      setStorage('rmcpcolor', hex);
      updateInputSelect();
    });

    setupEventListeners();
  }

  // Setup Event Listeners for Controls
  function setupEventListeners() {
    $("#reset_color").off().on("click", resetColor);
    $("#remote_duration").off().on("change", updateDuration);
    $("#effect_select").off().on("change", sendEffect);
    $("#remote_input_reseff, #remote_input_rescol").off().on("click", handleResetOrColor);
    $("#remote_input_repimg").off().on("click", handleImageRequest);
    $("#remote_input_img").on("change", handleImageChange);
  }

  // Reset Color Function
  function resetColor() {
    requestPriorityClear();
    lastImgData = "";
    $("#effect_select").val("__none__");
    $("#remote_input_img").val("");
  }

  // Update Duration
  function updateDuration() {
    uiInputDuration_s = valValue(this.id, this.value, this.min, this.max);
    setStorage('rmduration', uiInputDuration_s);
    updateDurationPlaceholder(); // Ensure placeholder is updated dynamically
  }

  // Update the placeholder or display "Endless" dynamically
  function updateDurationPlaceholder() {
    const remoteDuration = $("#remote_duration");
    if (Number(remoteDuration.val()) === 0) {
      remoteDuration.val(""); // Clear value temporarily
      remoteDuration.attr("placeholder", $.i18n('remote_input_duration_endless'));
    } else {
      remoteDuration.attr("placeholder", "");
    }
  }

  // Handle Reset or Color Click
  function handleResetOrColor() {
    if (this.id === "remote_input_rescol") {
      sendColor();
    } else if (EFFECTENGINE_ENABLED) {
      sendEffect();
    }
  }

  // Handle Image Request
  function handleImageRequest() {
    if (lastImgData) {
      requestSetImage(lastImgData, uiInputDuration_s, lastFileName);
    }
  }

  // Handle Image Change
  function handleImageChange() {
    readImg(this, function (src, fileName) {
      lastFileName = fileName;
      lastImgData = src.includes(",") ? src.split(",")[1] : src;
      requestSetImage(lastImgData, uiInputDuration_s, lastFileName);
    });
  }

  // Force First Update
  function forceFirstUpdate() {
    initComponents();
    updateInputSelect();
    updateLedMapping();
    updateVideoMode();
    updateChannelAdjustments();

    if (EFFECTENGINE_ENABLED) {
      updateEffectlist();
    } else {
      $('#effect_row').hide();
    }
  }

  // Interval Updates and Event Handlers
  function setupEventListenersForUpdates() {
    $(window.hyperion).on('components-updated', (e, comp) => updateComponent(comp));

    $(window.hyperion).on("cmd-priorities-update", (event) => {
      window.serverInfo.priorities = event.response.data.priorities;
      window.serverInfo.priorities_autoselect = event.response.data.priorities_autoselect;
      updateInputSelect();
    });

    $(window.hyperion).on("cmd-imageToLedMapping-update", (event) => {
      window.serverInfo.imageToLedMappingType = event.response.data.imageToLedMappingType;
      updateLedMapping();
    });

    $(window.hyperion).on("cmd-videomode-update", (event) => {
      window.serverInfo.videomode = event.response.data.videomode;
      updateVideoMode();
    });

    $(window.hyperion).on("cmd-effects-update", (event) => {
      window.serverInfo.effects = event.response.data.effects;
      updateEffectlist();
    });

    $(window.hyperion).on("cmd-settings-update", (event) => {
      if (event.response.data.color) {
        window.serverInfo.imageToLedMappingType = event.response.data.color.imageToLedMappingType;
        updateLedMapping();
        window.serverInfo.adjustment = event.response.data.color.channelAdjustment;
        updateChannelAdjustments();
      }
    });

    removeOverlay();
  }

  // Initialize everything
  function init() {
    initColorPickerAndEffects();
    forceFirstUpdate();
    setupEventListenersForUpdates();
  }

  init();

});

