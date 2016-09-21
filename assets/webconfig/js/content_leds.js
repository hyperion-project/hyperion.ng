
var ledsCustomCfgInitialized = false;

function get_hue_lights(){
	$.ajax({
		type: "GET",
		url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/lights',
		processData: false,
		contentType: 'application/json',
		success: function(r) {
			for(var lightid in r){
				//console.log(r[lightid].name);
				$('#hue_lights').append('ID: '+lightid+' Name: '+r[lightid].name+'<br />');
			}
		}
	});
}


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

	// -------------------------------------------------------------
	$('#leds_cfg_nav a[data-toggle="tab"]').off().on('shown.bs.tab', function (e) {
		var target = $(e.target).attr("href") // activated tab
		if (target == "#menu_gencfg" && !ledsCustomCfgInitialized)
		{
			ledsCustomCfgInitialized = true;
		}
	});

	// ------------------------------------------------------------------
	var grabber_conf_editor = null;
	$("#leddevices").off().on("change", function(event) {
		generalOptions  = parsedConfSchemaJSON.properties.device;
		specificOptions = parsedConfSchemaJSON.properties.alldevices[$(this).val()];
		//$('#ledDeviceOptions').html(JSON.stringify(generalOptions)+"<br>"+JSON.stringify(specificOptions));
		$('#editor_container').off();
		$('#editor_container').html("");
		var element = document.getElementById('editor_container');

		grabber_conf_editor = new JSONEditor(element,{
			theme: 'bootstrap3',
			iconlib: "fontawesome4",
			disable_collapse: 'true',
			form_name_root: 'sa',
			disable_edit_json: 'true',
			disable_properties: 'true',
			no_additional_properties: 'true',
			schema: {
				title:' ',
				properties: {
					generalOptions : generalOptions,
					specificOptions : specificOptions,
				}
			}
		});

		values_general = {};
		values_specific = {};
		isCurrentDevice = (server.info.ledDevices.active == $(this).val());

		for(var key in parsedConfJSON.device){
			if (key != "type" && key in generalOptions.properties)
				values_general[key] = parsedConfJSON.device[key];
		};
		grabber_conf_editor.getEditor("root.generalOptions").setValue( values_general );

		if (isCurrentDevice)
		{
			specificOptions_val = grabber_conf_editor.getEditor("root.specificOptions").getValue()
			for(var key in specificOptions_val){
					values_specific[key] = (key in parsedConfJSON.device) ? parsedConfJSON.device[key] : specificOptions_val[key];
			};

			grabber_conf_editor.getEditor("root.specificOptions").setValue( values_specific );
		};

		$('#editor_container .well').css("background-color","white");
		$('#editor_container .well').css("border","none");
		$('#editor_container .well').css("box-shadow","none");
		$('#editor_container .btn').addClass("btn-primary");
		$('#editor_container h3').first().remove();
		
		if ($(this).val() == "philipshue")
		{
			$("#huebridge").show();

			$("#ip").attr('value', values_specific.output);
			$("#user").attr('value', values_specific.username);

			if($("#ip").val() != '' && $("#user").val() != '') {
				get_hue_lights();
			}

		}
		else
		{
			$("#huebridge").hide();
		}
	});

	// ------------------------------------------------------------------
	$("#btn_submit_controller").off().on("click", function(event) {
		if (grabber_conf_editor==null)
			return;

		ledDevice = $("#leddevices").val();
		result = {device:{}};
		
		general = grabber_conf_editor.getEditor("root.generalOptions").getValue();
		specific = grabber_conf_editor.getEditor("root.specificOptions").getValue();
		for(var key in general){
			result.device[key] = general[key];
		}

		for(var key in specific){
			result.device[key] = specific[key];
		}
		result.device.type=ledDevice;
		requestWriteConfig(result)
	});

	// ------------------------------------------------------------------
	
	requestServerConfig();
});


