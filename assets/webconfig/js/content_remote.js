
	function updateInputSelect()
	{	
		$('#sstbody').html("");
		var data = "";
		var prios = parsedServerInfoJSON.info.priorities
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
				owner = $.i18n('remote_colors_label_color')+'  '+'<div style="width:18px; height:18px; border-radius:20px; margin-bottom:-4px; border:1px grey solid; background-color: rgb('+prios[i].value.RGB+'); display:inline-block" title="RGB: ('+prios[i].value.RGB+')"></div>';
			if(compId == "7")
				owner = $.i18n('general_comp_GRABBER')+': ('+owner+')';
			if(compId == "8")
				owner = $.i18n('general_comp_V4L')+': ('+owner+')';
			if(compId == "6")
				owner = $.i18n('general_comp_BOBLIGHTSERVER');
			if(compId == "5")
				owner = $.i18n('general_comp_UDPLISTENER');
			if(duration)
				owner += '<br/><span style="font-size:80%; color:grey;">'+$.i18n('remote_input_duration')+' '+duration.toFixed(0)+$.i18n('edt_append_s')+'</span>';
			
			var btn = '<button id="srcBtn'+i+'" type="button" '+btn_state+' class="btn btn-'+btn_type+' btn_input_selection" onclick="requestSetSource('+priority+');">'+btn_text+'</button>';
			
			if((compId == "10" || compId == "9") && priority != 254)
				btn += '<button type="button" class="btn btn-sm btn-danger" style="margin-left:10px;" onclick="requestPriorityClear('+priority+');"><i class="fa fa-close"></button>';
			
			if(btn_type != 'default')
				$('#sstbody').append(createTableRow([origin, owner, priority, btn], false, true));
		}
		var btn_auto_color = (parsedServerInfoJSON.info.priorities_autoselect? "btn-success" : "btn-danger");
		var btn_auto_state = (parsedServerInfoJSON.info.priorities_autoselect? "disabled" : "enabled");
		var btn_auto_text = (parsedServerInfoJSON.info.priorities_autoselect? $.i18n('general_btn_on') : $.i18n('general_btn_off'));
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
		mappingList = ["multicolor_mean", "unicolor_mean"];
		mapping = parsedServerInfoJSON.info.ledMAppingType;

		$('#mappingsbutton').html("");
		for(var ix = 0; ix < mappingList.length; ix++)
		{
			if(mapping == mappingList[ix])
				btn_style = 'btn-success';
			else
				btn_style = 'btn-warning';

			$('#mappingsbutton').append('<button type="button" id="lmBtn_'+mappingList[ix]+'" class="btn btn-lg '+btn_style+'" style="margin:10px;min-width:200px" onclick="requestMappingType(\''+mappingList[ix]+'\');">'+$.i18n('remote_maptype_label_'+mappingList[ix])+'</button><br/>');
		}
	}

	function updateComponents(event) {
		if ($('#componentsbutton').length == 0)
		{
			$(hyperion).off("cmd-serverinfo",updateComponents);
		}
		else
		{
			updateInputSelect();
			updateLedMapping();
			components = event.response.info.components;
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
	}
	
	var oldEffects = [];
	
	function updateEffectlist(event){
		var newEffects = event.response.info.effects;
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

$(document).ready(function() {
	performTranslation();
	createTable('ssthead', 'sstbody', 'sstcont');
	$('#ssthead').html(createTableRow([$.i18n('remote_input_origin'), $.i18n('remote_input_owner'), $.i18n('remote_input_priority'), $.i18n('remote_input_status')], true, true));
	
	// color
		$(function() {
			$('#cp2').colorpicker({
				format: 'rgb',
				customClass: 'colorpicker-2x',
				color: '#B500FF',
				sliders: {
					saturation: {
						maxLeft: 200,
						maxTop: 200
					},
					hue: {
						maxTop: 200
					},
					alpha: {
						maxTop: 200
					},
				}
			});
			$('#cp2').colorpicker().on('changeColor', function(e) {
				color = e.color.toRGB();
				$("#effect_select").val("__none__");
				requestSetColor(color.r, color.g, color.b);
			});
		});

		$("#reset_color").off().on("click", requestPriorityClear);

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
		
	// components
	$(hyperion).on("cmd-serverinfo",updateComponents);
	// effects
	$(hyperion).on("cmd-serverinfo",updateEffectlist);
		
});
