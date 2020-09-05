$(document).ready( function() {
  performTranslation();
  var conf_editor_v4l2 = null;
  var conf_editor_fg = null;
  var conf_editor_instCapt = null;
  var V4L2_AVAIL = window.serverInfo.grabbers.available.includes("v4l2");

  if(V4L2_AVAIL) {
    // Dynamic v4l2 enum schema
    var v4l2_dynamic_enum_schema = {
      "available_devices":
      {
        "type": "string",
        "title": "edt_conf_v4l2_device_title",
        "propertyOrder" : 1,
        "required" : true
      },
      "device_inputs":
      {
        "type": "string",
        "title": "edt_conf_v4l2_input_title",
        "propertyOrder" : 3,
        "required" : true
      },
      "resolutions":
      {
        "type": "string",
        "title": "edt_conf_v4l2_resolution_title",
        "propertyOrder" : 6,
        "required" : true
      },
      "framerates":
      {
        "type": "string",
        "title": "edt_conf_v4l2_framerate_title",
        "propertyOrder" : 9,
        "required" : true
      }
    };

    // Build dynamic v4l2 enum schema parts
    var buildSchemaPart = function(key, schema, device) {
      if (schema[key]) {
        var enumVals = [];
        var enumTitelVals = [];
        var v4l2_properties = JSON.parse(JSON.stringify(window.serverInfo.grabbers.v4l2_properties));

        if (key === 'available_devices') {
          for (var i = 0; i < v4l2_properties.length; i++) {
            enumVals.push(v4l2_properties[i]['device']);

            v4l2_properties[i].hasOwnProperty('name')
              ? enumTitelVals.push(v4l2_properties[i]['name'])
              : enumTitelVals.push(v4l2_properties[i]['device']);
          }
        } else if (key == 'resolutions' || key == 'framerates') {
          for (var i = 0; i < v4l2_properties.length; i++) {
            if (v4l2_properties[i]['device'] == device) {
              enumVals = enumTitelVals = v4l2_properties[i][key];
              break;
            }
          }
        } else if (key == 'device_inputs') {
          for (var i = 0; i < v4l2_properties.length; i++) {
            if (v4l2_properties[i]['device'] == device) {
              for (var index = 0; index < v4l2_properties[i]['inputs'].length; index++) {
                enumVals.push(v4l2_properties[i]['inputs'][index]['inputIndex'].toString());
                enumTitelVals.push(v4l2_properties[i]['inputs'][index]['inputName']);
              }
              break;
            }
          }
        }

        window.schema.grabberV4L2.properties[key] = {
          "type": schema[key].type,
          "title": schema[key].title,
          "enum": [].concat(["auto"], enumVals, ["custom"]),
          "options" :
          {
            "enum_titles" : [].concat(["edt_conf_enum_automatic"], enumTitelVals, ["edt_conf_enum_custom"]),
          },
          "propertyOrder" : schema[key].propertyOrder,
          "required" : schema[key].required
        };
      }
    };

    // Switch between visible states
    function toggleOption(option, state) {
      $('[data-schemapath="root.grabberV4L2.'+option+'"]').toggle(state);
      if (state) (
        $('[data-schemapath="root.grabberV4L2.'+option+'"]').addClass('col-md-12'),
        $('label[for="root_grabberV4L2_'+option+'"]').css('left','10px'),
        $('[id="root_grabberV4L2_'+option+'"]').css('left','10px')
      );
    }

    // Watch all v4l2 dynamic fields
    var setWatchers = function(schema) {
      var path = 'root.grabberV4L2.';
      Object.keys(schema).forEach(function(key) {
        conf_editor_v4l2.watch(path + key, function() {
          var ed = conf_editor_v4l2.getEditor(path + key);
          var val = ed.getValue();

          if (key == 'available_devices') {
            var V4L2properties = ['device_inputs', 'resolutions', 'framerates'];
            if (val == 'custom') {
              var grabberV4L2 = ed.parent;
              V4L2properties.forEach(function(item) {
                buildSchemaPart(item, v4l2_dynamic_enum_schema, 'none');
                grabberV4L2.original_schema.properties[item] = window.schema.grabberV4L2.properties[item];
                grabberV4L2.schema.properties[item] = window.schema.grabberV4L2.properties[item];
                conf_editor_v4l2.validator.schema.properties.grabberV4L2.properties[item] = window.schema.grabberV4L2.properties[item];

                grabberV4L2.removeObjectProperty(item);
                delete grabberV4L2.cached_editors[item];
                grabberV4L2.addObjectProperty(item);

                conf_editor_v4l2.getEditor(path + item).enable();
              });

              conf_editor_v4l2.getEditor(path + 'standard').enable();
              toggleOption('device', true);

            } else if (val == 'auto') {
              V4L2properties.forEach(function(item) {
                conf_editor_v4l2.getEditor(path + item).setValue('auto');
                conf_editor_v4l2.getEditor(path + item).disable();
              });

              conf_editor_v4l2.getEditor(path + 'standard').setValue('auto');
              conf_editor_v4l2.getEditor(path + 'standard').disable();

              (toggleOption('device', false), toggleOption('input', false),
               toggleOption('width', false), toggleOption('height', false),
               toggleOption('fps', false));

            } else {
              var grabberV4L2 = ed.parent;
              V4L2properties.forEach(function(item) {
                buildSchemaPart(item, v4l2_dynamic_enum_schema, val);
                grabberV4L2.original_schema.properties[item] = window.schema.grabberV4L2.properties[item];
                grabberV4L2.schema.properties[item] = window.schema.grabberV4L2.properties[item];
                conf_editor_v4l2.validator.schema.properties.grabberV4L2.properties[item] = window.schema.grabberV4L2.properties[item];

                grabberV4L2.removeObjectProperty(item);
                delete grabberV4L2.cached_editors[item];
                grabberV4L2.addObjectProperty(item);

                conf_editor_v4l2.getEditor(path + item).enable();
              });

              conf_editor_v4l2.getEditor(path + 'standard').enable();
              toggleOption('device', false);
            }
          }

          if (key == 'resolutions')
            val != 'custom'
              ? (toggleOption('width', false), toggleOption('height', false))
              : (toggleOption('width', true), toggleOption('height', true));

          if (key == 'framerates')
            val != 'custom'
              ? toggleOption('fps', false)
              : toggleOption('fps', true);

          if (key == 'device_inputs')
            val != 'custom'
              ? toggleOption('input', false)
              : toggleOption('input', true);
        });
      });
    };

    // Insert dynamic v4l2 enum schema parts
    Object.keys(v4l2_dynamic_enum_schema).forEach(function(key) {
      buildSchemaPart(key, v4l2_dynamic_enum_schema, window.serverConfig.grabberV4L2.device);
    });
  }

  if(window.showOptHelp) {
    // Instance Capture
    $('#conf_cont').append(createRow('conf_cont_instCapt'));
    $('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));

    // Framegrabber
    $('#conf_cont').append(createRow('conf_cont_fg'));
    $('#conf_cont_fg').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
    $('#conf_cont_fg').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title")));

    // V4L2 - hide if not available
    if(V4L2_AVAIL) {
      $('#conf_cont').append(createRow('conf_cont_v4l'));
      $('#conf_cont_v4l').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
      $('#conf_cont_v4l').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title")));
    }
  } else {
    $('#conf_cont').addClass('row');
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
    $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
    if(V4L2_AVAIL) {
      $('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
    }
  }

  // Instance Capture
  conf_editor_instCapt = createJsonEditor('editor_container_instCapt', {
    instCapture: window.schema.instCapture
  }, true, true);

  conf_editor_instCapt.on('change',function() {
    conf_editor_instCapt.validate().length ? $('#btn_submit_instCapt').attr('disabled', true) : $('#btn_submit_instCapt').attr('disabled', false);
  });

  $('#btn_submit_instCapt').off().on('click',function() {
    requestWriteConfig(conf_editor_instCapt.getValue());
  });

  // Framegrabber
  conf_editor_fg = createJsonEditor('editor_container_fg', {
    framegrabber: window.schema.framegrabber
  }, true, true);

  conf_editor_fg.on('change',function() {
    conf_editor_fg.validate().length ? $('#btn_submit_fg').attr('disabled', true) : $('#btn_submit_fg').attr('disabled', false);
  });

  $('#btn_submit_fg').off().on('click',function() {
    requestWriteConfig(conf_editor_fg.getValue());
  });

  if(V4L2_AVAIL) {
    conf_editor_v4l2 = createJsonEditor('editor_container_v4l2', {
      grabberV4L2 : window.schema.grabberV4L2
    }, true, true);

    conf_editor_v4l2.on('change',function() {
      conf_editor_v4l2.validate().length ? $('#btn_submit_v4l2').attr('disabled', true) : $('#btn_submit_v4l2').attr('disabled', false);
    });

    conf_editor_v4l2.on('ready', function() {
      setWatchers(v4l2_dynamic_enum_schema);

      if (window.serverConfig.grabberV4L2.available_devices == 'custom' && window.serverConfig.grabberV4L2.device != 'auto')
        toggleOption('device', true);

      if (window.serverConfig.grabberV4L2.device == 'auto')
        conf_editor_v4l2.getEditor('root.grabberV4L2.available_devices').setValue('auto');

      if (window.serverConfig.grabberV4L2.available_devices == 'auto') {
        ['device_inputs', 'standard', 'resolutions', 'framerates'].forEach(function(item) {
          conf_editor_v4l2.getEditor('root.grabberV4L2.' + item).setValue('auto');
          conf_editor_v4l2.getEditor('root.grabberV4L2.' + item).disable();
        });
      }

      if (window.serverConfig.grabberV4L2.device_inputs == 'custom' && window.serverConfig.grabberV4L2.device != 'auto')
        toggleOption('input', true);

      if (window.serverConfig.grabberV4L2.resolutions == 'custom' && window.serverConfig.grabberV4L2.device != 'auto')
        (toggleOption('width', true), toggleOption('height', true));

      if (window.serverConfig.grabberV4L2.framerates == 'custom' && window.serverConfig.grabberV4L2.device != 'auto')
        toggleOption('fps', true);

    });

    $('#btn_submit_v4l2').off().on('click',function() {
      var v4l2Options = conf_editor_v4l2.getValue();

      if (v4l2Options.grabberV4L2.available_devices != 'custom' && v4l2Options.grabberV4L2.available_devices != 'auto')
        v4l2Options.grabberV4L2.device = v4l2Options.grabberV4L2.available_devices;

      if (v4l2Options.grabberV4L2.available_devices == 'auto')
        v4l2Options.grabberV4L2.device = 'auto';

      if (v4l2Options.grabberV4L2.device_inputs != 'custom' && v4l2Options.grabberV4L2.device_inputs != 'auto' && v4l2Options.grabberV4L2.available_devices != 'auto')
        v4l2Options.grabberV4L2.input = parseInt(v4l2Options.grabberV4L2.device_inputs);

      if (v4l2Options.grabberV4L2.device_inputs == 'auto')
        v4l2Options.grabberV4L2.input = -1;

      if (v4l2Options.grabberV4L2.resolutions != 'custom' && v4l2Options.grabberV4L2.resolutions != 'auto' && v4l2Options.grabberV4L2.available_devices != 'auto')
        (v4l2Options.grabberV4L2.width = parseInt(v4l2Options.grabberV4L2.resolutions.split('x')[0]),
          v4l2Options.grabberV4L2.height = parseInt(v4l2Options.grabberV4L2.resolutions.split('x')[1]));

      if (v4l2Options.grabberV4L2.resolutions == 'auto')
        (v4l2Options.grabberV4L2.width = 0, v4l2Options.grabberV4L2.height = 0);

      if (v4l2Options.grabberV4L2.framerates != 'custom' && v4l2Options.grabberV4L2.framerates != 'auto' && v4l2Options.grabberV4L2.available_devices != 'auto')
        v4l2Options.grabberV4L2.fps = parseInt(v4l2Options.grabberV4L2.framerates);

      if (v4l2Options.grabberV4L2.framerates == 'auto')
        v4l2Options.grabberV4L2.fps = 15;

      requestWriteConfig(v4l2Options);
    });
  }

  //////////////////////////////////////////////////

  //create introduction
  if(window.showOptHelp) {
    createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_fg");
    if(V4L2_AVAIL){
      createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_v4l2");
    }
  }

  function hideEl(el) {
    for(var i = 0; i<el.length; i++) {
      $('[data-schemapath*="root.framegrabber.'+el[i]+'"]').toggle(false);
    }
  }

  //hide specific options
  conf_editor_fg.on('ready',function() {
    var grabbers = window.serverInfo.grabbers.available;

    if (grabbers.indexOf('dispmanx') > -1)
      hideEl(["device","pixelDecimation"]);
    else if (grabbers.indexOf('x11') > -1 || grabbers.indexOf('xcb') > -1)
      hideEl(["device","width","height"]);
    else if (grabbers.indexOf('osx')  > -1 )
      hideEl(["device","pixelDecimation"]);
    else if (grabbers.indexOf('amlogic')  > -1)
      hideEl(["pixelDecimation"]);
  });

  removeOverlay();
});
