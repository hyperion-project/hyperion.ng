
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
			ledColors = (event.response.result);
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
	$('#leds_toggle_num').on("click", function() {
		$('.led_num').toggle();
		toggleClass('#leds_toggle_num', "btn-danger", "btn-success");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle').on("click", function() {
		$('.led').toggle();
		toggleClass('#leds_toggle', "btn-success", "btn-danger");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle_live').on("click", function() {
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
	$("#leds_custom_check").on("click", function() {
		e = isJsonString($("#ledconfig").val());
		
		if (e.length == 0)
			showErrorDialog("Validation success", "Your config is valid!");
		else
			showErrorDialog("Validation failed!", e);
	});
	
	// ------------------------------------------------------------------
	$("#leds_custom_save").on("click", function() {
		
	});

	$('#leds_cfg_nav a[data-toggle="tab"]').on('shown.bs.tab', function (e) {
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

	requestServerConfig();
});

