
	function updateInputSelect()
	{
		var data = "";
		var i;
 		for(i = 0; i < parsedServerInfoJSON.info.priorities.length; i++) {
			var owner    = parsedServerInfoJSON.info.priorities[i].owner;
			var active   = parsedServerInfoJSON.info.priorities[i].active;
			var visible  = parsedServerInfoJSON.info.priorities[i].visible;
			var priority = parsedServerInfoJSON.info.priorities[i].priority;
			var btn_type = "default";
			if (active) btn_type = "warning";
			if (visible) btn_type = "success";
			
			if(btn_type != 'default')
				data += '<button id="srcBtn'+i+'" type="button" class="btn btn-lg btn-'+btn_type+' btn_input_selection" style="margin:10px;min-width:200px" onclick="requestSetSource('+priority+');">'+owner+'<span style="font-size:70% !important;"> ('+priority+')</span></button><br/>';
		}
		var autostate = (parsedServerInfoJSON.info.priorities_autoselect? "btn-success" : "btn-danger");
		data += '<button id="srcBtn'+i+'" type="button" class="btn btn-lg '+autostate+' btn_input_selection" style="margin:10px;min-width:200px" onclick="requestSetSource(\'auto\');">'+$.i18n('remote_input_label_autoselect')+'</button><br/>';
		$('#hyperion_inputs').html(data);
		
		var max_width=200;
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
	// color
		$(function() {
			$('#cp2').colorpicker({
				format: 'rgb',
				customClass: 'colorpicker-2x',
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
