$(document).ready(function() {
	performTranslation();

	var oldEffects = [];
	var cpcolor = '#B500FF';
	var mappingList = serverSchema.properties.color.properties.imageToLedMappingType.enum;
	var duration = 0;
	var rgb = {r:255,g:0,b:0};

	//create html
	createTable('ssthead', 'sstbody', 'sstcont');
	$('.ssthead').html(createTableRow([$.i18n('remote_input_origin'), $.i18n('remote_input_owner'), $.i18n('remote_input_priority'), $.i18n('remote_input_status')], true, true));
	createTable('crthead', 'crtbody', 'adjust_content', true);


	//create introduction
	if(showOptHelp)
	{
		createHint("intro", $.i18n('remote_color_intro', $.i18n('remote_losthint')), "color_intro");
		createHint("intro", $.i18n('remote_input_intro', $.i18n('remote_losthint')), "sstcont");
		createHint("intro", $.i18n('remote_adjustment_intro', $.i18n('remote_losthint')), "adjust_content");
		createHint("intro", $.i18n('remote_components_intro', $.i18n('remote_losthint')), "comp_intro");
		createHint("intro", $.i18n('remote_maptype_intro', $.i18n('remote_losthint')), "maptype_intro");
		createHint("intro", $.i18n('remote_videoMode_intro', $.i18n('remote_losthint')), "videomode_intro");
	}

	//color adjustment
	var sColor = sortProperties(serverSchema.properties.color.properties.channelAdjustment.items.properties)
	var values = serverInfo.adjustment[0]

	for(key in sColor)
	{
		if(sColor[key].key != "id" && sColor[key].key != "leds")
		{
			var title = '<label for="cr_'+sColor[key].key+'">'+$.i18n(sColor[key].title)+'</label>';
			var property;
			var value = values[sColor[key].key]

			if(sColor[key].type == "array")
			{
				property = '<div id="cr_'+sColor[key].key+'" class="input-group colorpicker-component" ><input type="text" class="form-control" /><span class="input-group-addon"><i></i></span></div>';
				$('.crtbody').append(createTableRow([title, property], false, true));
				createCP('cr_'+sColor[key].key, value, function(rgb,hex,e){
					requestAdjustment(e.target.id.substr(e.target.id.indexOf("_") + 1), '['+rgb.r+','+rgb.g+','+rgb.b+']');
				});
			}
			else if(sColor[key].type == "boolean")
			{
				property = '<div class="checkbox"><input id="cr_'+sColor[key].key+'" type="checkbox" value="'+value+'"/><label></label></div>';
				$('.crtbody').append(createTableRow([title, property], false, true));

				$('#cr_'+sColor[key].key).off().on('change', function(e){
					requestAdjustment(e.target.id.substr(e.target.id.indexOf("_") + 1), e.currentTarget.checked);
				});
			}
			else
			{
				if(sColor[key].key == "brightness" || sColor[key].key == "brightnessCompensation" || sColor[key].key == "backlightThreshold")
					property = '<div class="input-group"><input id="cr_'+sColor[key].key+'" type="number" class="form-control" min="0" max="100" step="10" value="'+value+'"/><span class="input-group-addon">'+$.i18n("edt_append_percent")+'</span></div>';
				else
					property = '<input id="cr_'+sColor[key].key+'" type="number" class="form-control" min="0.1" max="4.0" step="0.1" value="'+value+'"/>';

				$('.crtbody').append(createTableRow([title, property], false, true));
				$('#cr_'+sColor[key].key).off().on('change', function(e){
					valValue(this.id,this.value,this.min,this.max);
					requestAdjustment(e.target.id.substr(e.target.id.indexOf("_") + 1), e.currentTarget.value);
				});
			}
		}
	}

	function sendEffect()
	{
		efx = $("#effect_select").val();
		if(efx != "__none__")
		{
			requestPriorityClear();
			$(hyperion).one("cmd-clear", function(event) {
				setTimeout(function() {requestPlayEffect(efx,duration)}, 100);
			});
		}
	}

	function sendColor()
	{
		requestSetColor(rgb.r, rgb.g, rgb.b,duration);
	}

	function updateInputSelect()
	{
		$('.sstbody').html("");
		var data = "";
		var prios = serverInfo.priorities
		var i;
		var clearAll = false;

		for(i = 0; i < prios.length; i++)
		{
			var origin   = prios[i].origin ? prios[i].origin : "System";
			origin = origin.split("@");
			var ip = origin[1];
			origin = origin[0];

			var owner    = prios[i].owner;
			var active   = prios[i].active;
			var visible  = prios[i].visible;
			var priority = prios[i].priority;
			var compId   = prios[i].componentId;
			var duration = prios[i].duration_ms/1000;
			var value = "0,0,0";
			var btn_type = "default";
			var btn_text = $.i18n('remote_input_setsource_btn');
			var btn_state = "enabled";

			if (active)
				btn_type = "primary";

			if(priority > 254)
				continue;
			if(priority < 254 && (compId == "EFFECT" || compId == "COLOR") )
				clearAll = true;

			if (visible)
			{
				 btn_state = "disabled";
				 btn_type = "success";
				 btn_text = $.i18n('remote_input_sourceactiv_btn');
			}

			if(ip)
				origin += '<br/><span style="font-size:80%; color:grey;">'+$.i18n('remote_input_ip')+' '+ip+'</span>';

			if("value" in prios[i])
				value = prios[i].value.RGB;

			switch (compId)
			{
				case "EFFECT":
					owner = $.i18n('remote_effects_label_effects')+'  '+owner;
					break;
				case "COLOR":
					owner = $.i18n('remote_color_label_color')+'  '+'<div style="width:18px; height:18px; border-radius:20px; margin-bottom:-4px; border:1px grey solid; background-color: rgb('+value+'); display:inline-block" title="RGB: ('+value+')"></div>';
					break;
				case  "GRABBER":
					owner = $.i18n('general_comp_GRABBER')+': ('+owner+')';
					break;
				case "V4L":
					owner = $.i18n('general_comp_V4L')+': ('+owner+')';
					break;
				case "BOBLIGHTSERVER":
					owner = $.i18n('general_comp_BOBLIGHTSERVER');
					break;
				case "UDPLISTENER":
					owner = $.i18n('general_comp_UDPLISTENER');
					break;
				case "PROTOSERVER":
					owner = "Proto";
					break;
			}

			if(duration && compId != "GRABBER" && compId != "PROTOSERVER")
				owner += '<br/><span style="font-size:80%; color:grey;">'+$.i18n('remote_input_duration')+' '+duration.toFixed(0)+$.i18n('edt_append_s')+'</span>';

			var btn = '<button id="srcBtn'+i+'" type="button" '+btn_state+' class="btn btn-'+btn_type+' btn_input_selection" onclick="requestSetSource('+priority+');">'+btn_text+'</button>';

			if((compId == "EFFECT" || compId == "COLOR") && priority < 254)
				btn += '<button type="button" class="btn btn-sm btn-danger" style="margin-left:10px;" onclick="requestPriorityClear('+priority+');"><i class="fa fa-close"></button>';

			if(btn_type != 'default')
				$('.sstbody').append(createTableRow([origin, owner, priority, btn], false, true));
		}
		var btn_auto_color = (serverInfo.priorities_autoselect? "btn-success" : "btn-danger");
		var btn_auto_state = (serverInfo.priorities_autoselect? "disabled" : "enabled");
		var btn_auto_text = (serverInfo.priorities_autoselect? $.i18n('general_btn_on') : $.i18n('general_btn_off'));
		var btn_call_state = (clearAll? "enabled" : "disabled");
		$('#auto_btn').html('<button id="srcBtn'+i+'" type="button" '+btn_auto_state+' class="btn '+btn_auto_color+'" style="margin-right:5px;display:inline-block;" onclick="requestSetSource(\'auto\');">'+$.i18n('remote_input_label_autoselect')+' ('+btn_auto_text+')</button>');
		$('#auto_btn').append('<button type="button" '+btn_call_state+' class="btn btn-danger" style="display:inline-block;" onclick="requestClearAll();">'+$.i18n('remote_input_clearall')+'</button>');

		var max_width=100;
		$('.btn_input_selection').each(function() {
			if ($(this).innerWidth() > max_width)
				max_width = $(this).innerWidth();
		});
		$('.btn_input_selection').css("min-width",max_width+"px");
	}

	function updateLedMapping()
	{
		mapping = serverInfo.imageToLedMappingType;

		$('#mappingsbutton').html("");
		for(var ix = 0; ix < mappingList.length; ix++)
		{
			if(mapping == mappingList[ix])
				btn_style = 'btn-success';
			else
				btn_style = 'btn-primary';

			$('#mappingsbutton').append('<button type="button" id="lmBtn_'+mappingList[ix]+'" class="btn '+btn_style+'" style="margin:3px;min-width:200px" onclick="requestMappingType(\''+mappingList[ix]+'\');">'+$.i18n('remote_maptype_label_'+mappingList[ix])+'</button><br/>');
		}
	}

	function updateComponents()
	{
		components = comps;
		// create buttons
		$('#componentsbutton').html("");
		for ( idx=0; idx<components.length;idx++)
		{
			if(components[idx].name == "ALL")
				continue

			enable_style = (components[idx].enabled? "btn-success" : "btn-danger");
			enable_icon  = (components[idx].enabled? "fa-play" : "fa-stop");
			comp_name    = components[idx].name;
			comp_btn_id  = "comp_btn_"+comp_name;
			comp_goff	 = serverInfo.hyperion.enabled? "enabled" : "disabled";

			// create btn if not there
			if ($("#"+comp_btn_id).length == 0)
			{
				d='<span style="display:block;margin:3px"><button type="button" '+comp_goff+' id="'+comp_btn_id+'" class="btn '+enable_style
					+'" onclick="requestSetComponentState(\''+comp_name+'\','+(!components[idx].enabled)
					+')"><i id="'+comp_btn_id+'_icon" class="fa '+enable_icon+'"></i></button> '+$.i18n('general_comp_'+components[idx].name)+'</span>';
				$('#componentsbutton').append(d);
			}
			else // already create, update state
			{
				setClassByBool( $('#'+comp_btn_id)        , components[idx].enabled, "btn-danger", "btn-success" );
				setClassByBool( $('#'+comp_btn_id+"_icon"), components[idx].enabled, "fa-stop"    , "fa-play" );
				$('#'+comp_btn_id).attr("onclick",'requestSetComponentState(\''+comp_name+'\','+(!components[idx].enabled)+')').attr(comp_goff);
			}
		}
	}

	function updateEffectlist()
	{
		var newEffects = serverInfo.effects;
		if (newEffects.length != oldEffects.length)
		{
			$('#effect_select').html('<option value="__none__"></option>');
			var usrEffArr = [];
			var sysEffArr = [];

			for(i = 0; i < newEffects.length; i++) {
				var effectName = newEffects[i].name;
				if(!/^\:/.test(newEffects[i].file)){
					usrEffArr.push(effectName);
				}
				else{
					sysEffArr.push(effectName);
				}
			}
			$('#effect_select').append(createSel(usrEffArr, $.i18n('remote_optgroup_usreffets')));
			$('#effect_select').append(createSel(sysEffArr, $.i18n('remote_optgroup_syseffets')));
			oldEffects = newEffects;
		}
	}

	function updateVideoMode()
	{
		videoModes = ["2D","3DSBS","3DTAB"];
		currVideoMode = serverInfo.videomode;

		$('#videomodebtns').html("");
		for(var ix = 0; ix < videoModes.length; ix++)
		{
			if(currVideoMode == videoModes[ix])
				btn_style = 'btn-success';
			else
				btn_style = 'btn-primary';
			$('#videomodebtns').append('<button type="button" id="vModeBtn_'+videoModes[ix]+'" class="btn '+btn_style+'" style="margin:3px;min-width:200px" onclick="requestVideoMode(\''+videoModes[ix]+'\');">'+$.i18n('remote_videoMode_'+videoModes[ix])+'</button><br/>');
		}
	}

	// colorpicker and effect
	if (getStorage('rmcpcolor') != null)
	{
		cpcolor = getStorage('rmcpcolor');
		rgb = hexToRgb(cpcolor);
	}

	if (getStorage('rmduration') != null)
	{
		$("#remote_duration").val(getStorage('rmduration'));
		duration = getStorage('rmduration');
	}

	createCP('cp2', cpcolor, function(rgbT,hex){
		rgb = rgbT;
		sendColor()
		setStorage('rmcpcolor', hex);
	});

	$("#reset_color").off().on("click", function(){
		requestPriorityClear();
		$("#effect_select").val("__none__");
	});

	$("#remote_duration").off().on("change", function(){
		duration = valValue(this.id,this.value,this.min,this.max);
		setStorage('rmduration', duration);
	});

	$("#effect_select").off().on("change", function(event) {
		sendEffect();
	});

	$("#remote_input_reseff, #remote_input_rescol").off().on("click", function(){
		if(this.id == "remote_input_rescol")
			sendColor();
		else
			sendEffect();
	});

	$("#remote_input_img").change(function(){
		readImg(this, function(src,width,height){
			console.log(src,width,height)
			requestSetImage(src,width,height,duration)
		});
	});

	//force first update
	updateComponents();
	updateInputSelect();
	updateLedMapping();
	updateVideoMode();
	updateEffectlist();

	// interval updates
	$(hyperion).on("components-updated",updateComponents);

	$(hyperion).on("cmd-priorities-update", function(event){
		serverInfo.priorities = event.response.data.priorities
		serverInfo.priorities_autoselect = event.response.data.priorities_autoselect
		updateInputSelect()
	});
	$(hyperion).on("cmd-imageToLedMapping-update", function(event){
		serverInfo.imageToLedMappingType = event.response.data.imageToLedMappingType
		updateLedMapping()
	});

	$(hyperion).on("cmd-videomode-update", function(event){
		serverInfo.videomode = event.response.data.videomode
		updateVideoMode()
	});

	$(hyperion).on("cmd-effects-update", function(event){
		serverInfo.effects = event.response.data.effects
		updateEffectlist();
	});

	removeOverlay();
});
