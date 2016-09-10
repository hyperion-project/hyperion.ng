
var ledsCustomCfgInitialized = false;


$(document).ready(function() {
	// ------------------------------------------------------------------
	$(hyperion).on("cmd-ledcolors-ledstream-update",function(event){
		if ($("#leddevices").length == 0)
		{
			requestLedColorsStop();
		}
		else
		{
			ledColors = (event.response.result.leds);
			for(var idx=0; idx<ledColors.length; idx++)
			{
				led = ledColors[idx]
				$("#led_"+led.index).css("background","rgb("+led.red+","+led.green+","+led.blue+")");
			}
		}
	});

	// ------------------------------------------------------------------
	$(hyperion).on("cmd-ledcolors-ledstream-stop",function(event){
		led_count = $(".led").length;
		for(var idx=0; idx<led_count; idx++)
		{
			$('#led_'+idx).css("background-color","hsl("+(idx*360/led_count)+",100%,50%)");
		}
	});

	// ------------------------------------------------------------------
	$(hyperion).one("cmd-serverinfo",function(event){
		server = event.response;
		ledDevices = server.info.ledDevices.available

		ledDevicesHtml = "";
		for (idx=0; idx<ledDevices.length; idx++)
		{
			ledDevicesHtml += '<option value="'+ledDevices[idx]+'">'+ledDevices[idx]+'</option>';
		}
		$("#leddevices").html(ledDevicesHtml);
		$("#leddevices").val(server.info.ledDevices.active);
		$("#leddevices").trigger("change");
	});

	// ------------------------------------------------------------------
	$(hyperion).on("cmd-config-getconfig",function(event){
		parsedConfJSON = event.response.result;
		leds = parsedConfJSON.leds;
		$("#ledconfig").text(JSON.stringify(leds, null, "\t"));
		canvas_height = $('#leds_canvas').innerHeight();
		canvas_width = $('#leds_canvas').innerWidth();

		leds_html = "";
		for(var idx=0; idx<leds.length; idx++)
		{
			led = leds[idx];
			led_id='led_'+led.index;
			bgcolor = "background-color:hsl("+(idx*360/leds.length)+",100%,50%);";
			pos = "left:"+(led.hscan.minimum * canvas_width)+"px;"+
				"top:"+(led.vscan.minimum * canvas_height)+"px;"+
				"width:"+((led.hscan.maximum-led.hscan.minimum) * canvas_width-1)+"px;"+
				"height:"+((led.vscan.maximum-led.vscan.minimum) * canvas_height-1)+"px;";
			leds_html += '<div id="'+led_id+'" class="led" style="'+bgcolor+pos+'" title="'+led.index+'"><span id="'+led_id+'_num" class="led_num">'+led.index+'</span></div>';
		}
		$('#leds_canvas').html(leds_html);
		$('#led_0').css("border","2px dotted red");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle_num').off().on("click", function() {
		$('.led_num').toggle();
		toggleClass('#leds_toggle_num', "btn-danger", "btn-success");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle').off().on("click", function() {
		$('.led').toggle();
		toggleClass('#leds_toggle', "btn-success", "btn-danger");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle_live').off().on("click", function() {
		setClassByBool('#leds_toggle_live',ledStreamActive,"btn-success","btn-danger");
		if ( ledStreamActive )
		{
			requestLedColorsStop();
		}
		else
		{
			requestLedColorsStart();
		}
	});

	// ------------------------------------------------------------------
	$("#leds_custom_check").off().on("click", function() {
		e = isJsonString($("#ledconfig").val());
		
		if (e.length == 0)
			showErrorDialog("Validation success", "Your config is valid!");
		else
			showErrorDialog("Validation failed!", e);
	});
	
	// ------------------------------------------------------------------
	$("#leds_custom_save").off().on("click", function() {
		
	});

	$('#leds_cfg_nav a[data-toggle="tab"]').off().on('shown.bs.tab', function (e) {
		var target = $(e.target).attr("href") // activated tab
		if (target == "#menu_gencfg" && !ledsCustomCfgInitialized)
		{
			ledsCustomCfgInitialized = true;
// 			$("#ledconfig").linedtextarea();
// 			$(window).resize(function(){
// 				$("#ledconfig").trigger("resize");
// 			});
		}
	});

	$("#leddevices").off().on("change", function(event) {
		generalOptions  = parsedConfSchemaJSON.properties.device;
		specificOptions = parsedConfSchemaJSON.properties.alldevices[$(this).val()];
		//$('#ledDeviceOptions').html(JSON.stringify(generalOptions)+"<br>"+JSON.stringify(specificOptions));
		$('#editor_container').off();
		$('#editor_container').html("");
		var element = document.getElementById('editor_container');
	
		var grabber_conf_editor = new JSONEditor(element,{
			theme: 'bootstrap3',
			disable_collapse: 'true',
			form_name_root: 'sa',
			disable_edit_json: 'true',
			disable_properties: 'true',
			no_additional_properties: 'true',
			schema: {
				title:' ',
				properties: {
					generalOptions,
					specificOptions,
				}
			}
		});

		values_general = {};
		values_specific = {};
		isCurrentDevice = (server.info.ledDevices.active == parsedConfJSON.device.type);

		for(var key in parsedConfJSON.device){
			if (key in generalOptions.properties)
				values_general[key] = parsedConfJSON.device[key];
		};
		grabber_conf_editor.setValue( { "generalOptions" : values_general, "specificOptions" : specificOptions });
	
		if (isCurrentDevice)
		{
			for(var key in parsedConfJSON.device){
				if (key in specificOptions.properties)
					values_specific[key] = parsedConfJSON.device[key];
			};
			grabber_conf_editor.setValue( { "generalOptions" : values_general, "specificOptions" : values_specific });
		};
		
		if ($(this).val() == "philipshue")
		{
			$("#huebridge").show();
		}
		else
		{
			$("#huebridge").hide();
		}
	});
	
	requestServerConfig();
});

