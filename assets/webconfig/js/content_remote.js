$(document).ready(function() {
	performTranslation();
	
	var oldEffects = [];
	var cpcolor = '#B500FF';
	var mappingList = serverSchema.properties.color.properties.imageToLedMappingType.enum;

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
	}
	
	//color adjustment
	var sColor = sortProperties(serverSchema.properties.color.properties.channelAdjustment.items.properties)
	var values = serverInfo.info.adjustment[0]
	
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
				if(sColor[key].key == "brightness" || sColor[key].key == "backlightThreshold")
					property = '<input id="cr_'+sColor[key].key+'" type="number" class="form-control" min="0.0" max="1.0" step="0.05" value="'+value+'"/>';
				else
					property = '<input id="cr_'+sColor[key].key+'" type="number" class="form-control" min="0.0" max="4.0" step="0.1" value="'+value+'"/>';
				
				$('.crtbody').append(createTableRow([title, property], false, true));
				$('#cr_'+sColor[key].key).off().on('change', function(e){
					requestAdjustment(e.target.id.substr(e.target.id.indexOf("_") + 1), e.currentTarget.value);
				});
			}
		}
	}

	function updateRemote()
	{
		if ($('#componentsbutton').length == 0)
		{
			$(hyperion).off("cmd-serverinfo",updateRemote);
		}
		else
		{
			updateInputSelect();
			updateLedMapping();
			updateComponents();
			updateEffectlist();
		}
	}
	
	function updateInputSelect()
	{	
		$('.sstbody').html("");
		var data = "";
		var prios = serverInfo.info.priorities
		var i;
		for(i = 0; i < prios.length; i++)
		{
			var origin   = "not impl";
			var ip       = "xxx.xxx.xxx.xxx";
			var owner    = prios[i].owner;
			var active   = prios[i].active;
			var visible  = prios[i].visible;
			var priority = prios[i].priority;
			var compId   = prios[i].componentId;
			var duration = prios[i].duration_ms/1000;
			var btn_type = "default";
			var btn_text = $.i18n('remote_input_setsource_btn');
			var btn_state = "enabled";
			if (active) btn_type = "warning";
			if (visible)
			{
				 btn_state = "disabled";
				 btn_type = "success";
				 btn_text = $.i18n('remote_input_sourceactiv_btn');
			}
			if(ip)
				origin += '<br/><span style="font-size:80%; color:grey;">'+$.i18n('remote_input_ip')+' '+ip+'</span>';
			if(compId == "10")
				owner = $.i18n('remote_effects_label_effects')+'  '+owner;
			if(compId == "9")
				owner = $.i18n('remote_color_label_color')+'  '+'<div style="width:18px; height:18px; border-radius:20px; margin-bottom:-4px; border:1px grey solid; background-color: rgb('+prios[i].value.RGB+'); display:inline-block" title="RGB: ('+prios[i].value.RGB+')"></div>';
			if(compId == "7")
				owner = $.i18n('general_comp_GRABBER')+': ('+owner+')';
			if(compId == "8")
				owner = $.i18n('general_comp_V4L')+': ('+owner+')';
			if(compId == "6")
				owner = $.i18n('general_comp_BOBLIGHTSERVER');
			if(compId == "5")
				owner = $.i18n('general_comp_UDPLISTENER');
			if(owner == "Off")
				owner = $.i18n('general_btn_off');
			if(duration)
				owner += '<br/><span style="font-size:80%; color:grey;">'+$.i18n('remote_input_duration')+' '+duration.toFixed(0)+$.i18n('edt_append_s')+'</span>';
			
			var btn = '<button id="srcBtn'+i+'" type="button" '+btn_state+' class="btn btn-'+btn_type+' btn_input_selection" onclick="requestSetSource('+priority+');">'+btn_text+'</button>';
			
			if((compId == "10" || compId == "9") && priority != 254)
				btn += '<button type="button" class="btn btn-sm btn-danger" style="margin-left:10px;" onclick="requestPriorityClear('+priority+');"><i class="fa fa-close"></button>';
			
			if(btn_type != 'default')
				$('.sstbody').append(createTableRow([origin, owner, priority, btn], false, true));
		}
		var btn_auto_color = (serverInfo.info.priorities_autoselect? "btn-success" : "btn-danger");
		var btn_auto_state = (serverInfo.info.priorities_autoselect? "disabled" : "enabled");
		var btn_auto_text = (serverInfo.info.priorities_autoselect? $.i18n('general_btn_on') : $.i18n('general_btn_off'));
		$('#auto_btn').html('<button id="srcBtn'+i+'" type="button" '+btn_auto_state+' class="btn '+btn_auto_color+'" style="margin:10px;display:inline-block;" onclick="requestSetSource(\'auto\');">'+$.i18n('remote_input_label_autoselect')+' ('+btn_auto_text+')</button>');
		
		var max_width=100;
		$('.btn_input_selection').each(function() {
			if ($(this).innerWidth() > max_width)
				max_width = $(this).innerWidth();
		});
		$('.btn_input_selection').css("min-width",max_width+"px"); 
	}
	
	function updateLedMapping()
	{
		mapping = serverInfo.info.ledMAppingType;

		$('#mappingsbutton').html("");
		for(var ix = 0; ix < mappingList.length; ix++)
		{
			if(mapping == mappingList[ix])
				btn_style = 'btn-success';
			else
				btn_style = 'btn-warning';

			$('#mappingsbutton').append('<button type="button" id="lmBtn_'+mappingList[ix]+'" class="btn '+btn_style+'" style="margin:10px;min-width:200px" onclick="requestMappingType(\''+mappingList[ix]+'\');">'+$.i18n('remote_maptype_label_'+mappingList[ix])+'</button><br/>');
		}
	}

	function updateComponents()
	{
		components = serverInfo.info.components;
		// create buttons
		$('#componentsbutton').html("");
		for ( idx=0; idx<components.length;idx++)
		{
			enable_style = (components[idx].enabled? "btn-success" : "btn-danger");
			enable_icon  = (components[idx].enabled? "fa-play" : "fa-stop");
			comp_name    = components[idx].name;
			comp_btn_id  = "comp_btn_"+comp_name;
			
			// create btn if not there
			if ($("#"+comp_btn_id).length == 0)
			{
				d='<p><button type="button" id="'+comp_btn_id+'" class="btn '+enable_style
					+'" onclick="requestSetComponentState(\''+comp_name+'\','+(!components[idx].enabled)
					+')"><i id="'+comp_btn_id+'_icon" class="fa '+enable_icon+'"></i></button> '+$.i18n('general_comp_'+components[idx].name)+'</p>';
				$('#componentsbutton').append(d);
			}
			else // already create, update state
			{
				setClassByBool( $('#'+comp_btn_id)        , components[idx].enabled, "btn-danger", "btn-success" );
				setClassByBool( $('#'+comp_btn_id+"_icon"), components[idx].enabled, "fa-stop"    , "fa-play" );
				$('#'+comp_btn_id).attr("onclick",'requestSetComponentState(\''+comp_name+'\','+(!components[idx].enabled)+')');
			}
		}
	}
	
	function updateEffectlist()
	{
		var newEffects = serverInfo.info.effects;
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
	
	// colorpicker and effect
	if (getStorage('rmcpcolor') != null)
		cpcolor = getStorage('rmcpcolor');
			
	createCP('cp2', cpcolor, function(rgb,hex){
		requestSetColor(rgb.r, rgb.g, rgb.b);
		$("#effect_select").val("__none__");
		setStorage('rmcpcolor', hex);
	});

	$("#reset_color").off().on("click", function(){
		requestPriorityClear();
		$("#effect_select").val("__none__");
	});

	$("#effect_select").off().on("change", function(event) {
		efx = $(this).val();
		if(efx != "__none__")
		{
			requestPriorityClear();
			$(hyperion).one("cmd-clear", function(event) {
				setTimeout(function() {requestPlayEffect(efx)}, 100);
			});
		}
	});
	
	//force first update
	updateRemote();	
	
	// interval updates
	$(hyperion).on("cmd-serverinfo",updateRemote);
	
	removeOverlay();	
});
