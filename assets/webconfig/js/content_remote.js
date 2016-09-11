
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
			
 			data += '<button id="srcBtn'+i+'" type="button" class="btn btn-lg btn-'+btn_type+' btn_input_selection" style="margin:10px;min-width:200px" title="prio '+priority+'" onclick="requestSetSource('+priority+');">'+owner+'</button><br/>';
		}
		data += '<button id="srcBtn'+i+'" type="button" class="btn btn-lg btn-info btn_input_selection" style="margin:10px;min-width:200px" onclick="requestSetSource(\'auto\');">auto select</button><br/>';
		$('#hyperion_inputs').html(data);
		
		var max_width=200;
		$('.btn_input_selection').each(function() {
			if ($(this).innerWidth() > max_width)
				max_width = $(this).innerWidth();
		});
		$('.btn_input_selection').css("min-width",max_width+"px"); 
	}

	function updateComponents(event) {
		if ($('#componentsbutton').length == 0)
		{
			$(hyperion).off("cmd-serverinfo",updateComponents);
		}
		else
		{
			updateInputSelect();
			components = event.response.info.components;
			// create buttons
			$('#componentsbutton').html("");
			for ( idx=0; idx<components.length;idx++)
			{
				//components_html += '<tr><td>'+(components[idx].title)+'</td><td><i class="fa fa-circle component-'+(components[idx].enabled?"on":"off")+'"></i></td></tr>';
				enable_style = (components[idx].enabled? "btn-success" : "btn-danger");
				enable_icon  = (components[idx].enabled? "fa-play" : "fa-stop");
				comp_name    = components[idx].name;
				comp_btn_id  = "comp_btn_"+comp_name;
				
				// create btn if not there
				if ($("#"+comp_btn_id).length == 0)
				{
					d='<p><button type="button" id="'+comp_btn_id+'" class="btn '+enable_style
						+'" onclick="requestSetComponentState(\''+comp_name+'\','+(!components[idx].enabled)
						+')"><i id="'+comp_btn_id+'_icon" class="fa '+enable_icon+'"></i></button> '+components[idx].title+'</p>';
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



$(document).ready(function() {
	// color
		$(function() {
			$('#cp2').colorpicker({
				format: 'rgb',

				colorSelectors: {
					'default': '#777777',
					'primary': '#337ab7',
					'success': '#5cb85c',
					'info'   : '#5bc0de',
					'warning': '#f0ad4e',
					'danger' : '#d9534f'
					},
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
				requestSetColor(color.r, color.g, color.b);
			});
		});

		$("#reset_color").off().on("click", requestPriorityClear);

		$("#effect_select").off().on("change", function(event) {
			efx = $(this).val();
			if(efx != "__none__")
			{
				requestPlayEffect(efx);
			}
		});

		// effects
		effects_html = '<option value="__none__"></option>';
		for(i = 0; i < parsedServerInfoJSON.info.effects.length; i++) {
			//console.log(parsedServerInfoJSON.info.effects[i].name);
			effectName = parsedServerInfoJSON.info.effects[i].name;
			effects_html += '<option value="'+effectName+'">'+effectName+'</option>';
		}
		$('#effect_select').html(effects_html);
		
		
		// components
	$(hyperion).on("cmd-serverinfo",updateComponents);
		
});
