
var ledsCustomCfgInitialized = false;
var finalLedArray = [];

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

function validateText(){
	e = isJsonString($("#ledconfig").val());

	if (e.length != 0){
		showInfoDialog("error", $.i18n('InfoDialog_leds_validfail_title'), e);
		return false
	}
	return true
}

function round(number) {
	var factor = Math.pow(10, 4);
	var tempNumber = number * factor;
	var roundedTempNumber = Math.round(tempNumber);
	return roundedTempNumber / factor;
};

function createLedPreview(leds, origin){

	if (origin == "classic"){
		$('#previewcreator').html('<h5>'+$.i18n('conf_leds_layout_preview_originCL')+'</h5>');
		$('#leds_preview').css("padding-top", "56.25%");
	}
	else if(origin == "text"){
		$('#previewcreator').html('<h5>'+$.i18n('conf_leds_layout_preview_originTEXT')+'</h5>');
		$('#leds_preview').css("padding-top", "56.25%");
	}
	else if(origin == "matrix"){
		$('#previewcreator').html('<h5>'+$.i18n('conf_leds_layout_preview_originMA')+'</h5>');
		$('#leds_preview').css("padding-top", "100%");
	}
	
	$('#previewledcount').html('<h5>'+$.i18n('conf_leds_layout_preview_totalleds', leds.length)+'</h5>');
	
	$('.st_helper').css("border", "8px solid grey");
	
	canvas_height = $('#leds_preview').innerHeight();
	canvas_width = $('#leds_preview').innerWidth();

	leds_html = "";
	for(var idx=0; idx<leds.length; idx++)
	{
		led = leds[idx];
		led_id='ledc_'+[idx];
		bgcolor = "background-color:hsl("+(idx*360/leds.length)+",100%,50%);";
		pos = "left:"+(led.hscan.minimum * canvas_width)+"px;"+
			"top:"+(led.vscan.minimum * canvas_height)+"px;"+
			"width:"+((led.hscan.maximum-led.hscan.minimum) * canvas_width-1)+"px;"+
			"height:"+((led.vscan.maximum-led.vscan.minimum) * canvas_height-1)+"px;";
		leds_html += '<div id="'+led_id+'" class="led" style="'+bgcolor+pos+'" title="'+led.index+'"><span id="'+led_id+'_num" class="led_prev_num">'+led.index+'</span></div>';
	}
	$('#leds_preview').html(leds_html);
	$('#ledc_0').css({"background-color":"black","z-index":"12"});
	$('#ledc_1').css({"background-color":"grey","z-index":"11"});
	$('#ledc_2').css({"background-color":"#A9A9A9","z-index":"10"});
	
	if($('#leds_prev_toggle_num').hasClass('btn-success'))
		$('.led_prev_num').css("display", "inline");

}

function createClassicLeds(){
	//get values
	var ledsTop = parseInt($("#ip_cl_ledstop").val());
	var ledsBottom = parseInt($("#ip_cl_ledsbottom").val());
	var ledsLeft = parseInt($("#ip_cl_ledsleft").val());
	var ledsRight = parseInt($("#ip_cl_ledsright").val());
	var ledsGlength = parseInt($("#ip_cl_ledsglength").val());
	var ledsGPos = parseInt($("#ip_cl_ledsgpos").val());
	var position = parseInt($("#ip_cl_position").val());
	var reverse = $("#ip_cl_reverse").is(":checked");
	
	//advanced values
	var rawledsVDepth = parseInt($("#ip_cl_ledsvdepth").val());
	var rawledsHDepth = parseInt($("#ip_cl_ledshdepth").val());
	var rawedgeGap = parseInt($("#ip_cl_ledsedgegap").val());
	var rawcornerGap = parseInt($("#ip_cl_ledscornergap").val());
	
	//helper
	var ledsVDepth = rawledsVDepth /100;
	var ledsHDepth = rawledsHDepth /100;
	var edgeVGap = rawedgeGap /100/2;
	var edgeHGap = edgeVGap/(16/9);
	var cornerVGap = rawcornerGap /100/2;
	var cornerHGap = cornerVGap/(16/9);
	var Vmin = 0.0 + edgeVGap;
	var Vmax = 1.0 - edgeVGap;
	var Hmin = 0.0 + edgeHGap;
	var Hmax = 1.0 - edgeHGap;
	var ledArray = [];
	
	function createFinalArray(array){
		finalLedArray = [];
		for(var i = 0; i<array.length; i++){
			hmin = array[i].hscan.minimum;
			hmax = array[i].hscan.maximum;
			vmin = array[i].vscan.minimum;
			vmax = array[i].vscan.maximum;
			finalLedArray[i] = { "index" : i, "hscan": { "maximum" : hmax, "minimum" : hmin }, "vscan": { "maximum": vmax, "minimum": vmin}}
		}
		createLedPreview(finalLedArray, 'classic');
	}
	
	function validateGap(){
		if (ledsGPos+ledsGlength > ledArray.length){
			showInfoDialog('error', $.i18n('infoDialog_leds_gap_title'), $.i18n('infoDialog_leds_gap_text'));
			return false
		}
		return true
	}
	
	function rotateArray(array, times){
		if (times > "0"){
			while( times-- ){
				array.push(array.shift())
			}
			return array;
		}
		else
		{
			while( times++ ){
				array.unshift(array.pop())
			}
			return array;
		}
	}
	
	function createLedArray(hmin, hmax, vmin, vmax){
		hmin = round(hmin);
		hmax = round(hmax);
		vmin = round(vmin);
		vmax = round(vmax);
		ledArray.push( { "hscan" : { "minimum" : hmin, "maximum" : hmax }, "vscan": { "minimum": vmin, "maximum": vmax }} );
	}
	
	function createTopLeds(){
		step=(Hmax-Hmin)/ledsTop;
		hmin=Hmin
		if(cornerVGap != '0'){
			step=(Hmax-Hmin-(cornerHGap*2))/ledsTop;
			hmin=Hmin+(cornerHGap);
		}
		vmin=Vmin
		vmax=vmin+ledsHDepth;
		hmax=hmin+step
		for (var i = 0; i<ledsTop; i++){
			createLedArray(hmin, hmax, vmin, vmax);
			hmin += step
			hmax += step
		}	
	}
	
	function createLeftLeds(){
		step=(Vmax-Vmin)/ledsLeft;
		vmax=Vmax
		if(cornerVGap != '0'){
			step=(Vmax-Vmin-(cornerVGap*2))/ledsLeft;
			vmax=Vmax-(cornerVGap);
		}
		hmin=Hmin;
		hmax=hmin+ledsVDepth;
		vmin=vmax-step
		for (var i = ledsLeft; i>0; i--){
			createLedArray(hmin, hmax, vmin, vmax);
			vmin -= step
			vmax -= step
		}
	}
	
	function createRightLeds(){
		step=(Vmax-Vmin)/ledsRight;
		vmin=Vmin
		if(cornerVGap != '0'){
			step=(Vmax-Vmin-(cornerVGap*2))/ledsRight;
			vmin=Vmin+(cornerVGap);
		}
		hmax=Hmax;
		hmin=hmax-ledsVDepth;
		vmax=vmin+step
		for (var i = 0; i<ledsRight; i++){
			createLedArray(hmin, hmax, vmin, vmax);	
			vmin += step
			vmax += step			
		}
	}
	
	function createBottomLeds(){
		step=(Hmax-Hmin)/ledsBottom;
		hmax=Hmax
		if(cornerVGap != '0'){
			step=(Hmax-Hmin-(cornerHGap*2))/ledsBottom;
			hmax=Hmax-(cornerHGap);
		}
		vmax=Vmax;
		vmin=vmax-ledsHDepth;
		hmin=hmax-step
		for (var i = ledsBottom; i>0; i--){
			createLedArray(hmin, hmax, vmin, vmax);
			hmin -= step;
			hmax -= step;
		}
	}
	
	createLeftLeds(createBottomLeds(createRightLeds(createTopLeds())));

	if(ledsGlength != "0" && validateGap()){
		ledArray.splice(ledsGPos, ledsGlength);
	}
	
	if (position != "0"){
		rotateArray(ledArray, position);
	}

	if (reverse)
		ledArray.reverse();
	
	createFinalArray(ledArray);
}

function createMatrixLeds(){
// Big thank you to RanzQ (Juha Rantanen) from Github for this script
// https://raw.githubusercontent.com/RanzQ/hyperion-audio-effects/master/matrix-config.js

	//get values
	var width = parseInt($("#ip_ma_ledshoriz").val());
	var height = parseInt($("#ip_ma_ledsvert").val());
	var cabling = $("#ip_ma_cabling").val();
	//var order = $("#ip_ma_order").val();
	var start = $("#ip_ma_start").val();

	var parallel = false
	var index = 0
	var leds = []
	var hblock = 1.0 / width
	var vblock = 1.0 / height

	if (cabling == "parallel"){
		parallel = true
	}

/**
 * Adds led to the hyperion config led array
 * @param {Number} index Index of the led
 * @param {Number} x     Horizontal position in matrix
 * @param {Number} y     Vertical position in matrix
 */
	function addLed (index, x, y) {
		var hscanMin = x * hblock
		var hscanMax = (x + 1) * hblock
		var vscanMin = y * vblock
		var vscanMax = (y + 1) * vblock

		hscanMin = round(hscanMin);
		hscanMax = round(hscanMax);
		vscanMin = round(vscanMin);
		vscanMax = round(vscanMax);
		
		leds.push({
			index: index,
			hscan: {
				minimum: hscanMin,
				maximum: hscanMax
			},
			vscan: {
				minimum: vscanMin,
				maximum: vscanMax
			}
		})
	}

	var startYX = start.split('-')
	var startX = startYX[1] === 'right' ? width - 1 : 0
	var startY = startYX[0] === 'bottom' ? height - 1 : 0
	var endX = startX === 0 ? width - 1 : 0
	var endY = startY === 0 ? height - 1 : 0
	var forward = startX < endX

	var downward = startY < endY

	var x, y

	for (y = startY; downward && y <= endY || !downward && y >= endY; y += downward ? 1 : -1) {
		for (x = startX; forward && x <= endX || !forward && x >= endX; x += forward ? 1 : -1) {
			addLed(index, x, y)
			index++
		}
		if (!parallel) {
			forward = !forward
			var tmp = startX
			startX = endX
			endX = tmp
		}
	}
  finalLedArray =[];
  finalLedArray = leds
  createLedPreview(leds, 'matrix');
}

$(document).ready(function() {
	performTranslation();
	//-------------------------------------------------------------------
	$('.ledCLconstr').bind("change", function() {
		createClassicLeds();
	});
	
	// ------------------------------------------------------------------
	$('.ledMAconstr').bind("change", function() {
		createMatrixLeds();
	});
	
	// ------------------------------------------------------------------
	$('#btn_cl_generate').off().on("click", function() {
		if (finalLedArray != ""){
			$("#ledconfig").text(JSON.stringify(finalLedArray, null, "\t"));
			$('#collapse1').collapse('hide')
			$('#collapse4').collapse('show');
		}
	});
	
	// ------------------------------------------------------------------
	$('#btn_ma_generate').off().on("click", function() {
		if (finalLedArray != ""){
			$("#ledconfig").text(JSON.stringify(finalLedArray, null, "\t"));
			$('#collapse2').collapse('hide')
			$('#collapse4').collapse('show');
		}
	});

	// ------------------------------------------------------------------
	$(hyperion).on("cmd-ledcolors-imagestream-update",function(event){
		if ($("#leddevices").length == 0)
		{
			requestLedImageStop();
		}
		else
		{
			imageData = (event.response.result.image);
			$("#image_preview").attr("src", imageData);
		}
	});

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
		devRPiSPI = ['apa102', 'ws2801', 'lpd6803', 'lpd8806', 'p9813', 'sk6812spi', 'ws2812spi'];
		devRPiPWM = ['ws281x'];
		devRPiGPIO = ['piblaster'];
		devNET = ['atmoorb', 'fadecandy', 'philipshue', 'tinkerforge', 'tpm2net', 'udpe131', 'udph801', 'udpraw'];
		devUSB = ['adalight', 'dmx', 'atmo', 'hyperionusbasp', 'lightpack', 'multilightpack', 'paintpack', 'rawhid', 'sedu', 'tpm2'];
		
		var optArr = [[]];
		optArr[1]=[];
		optArr[2]=[];
		optArr[3]=[];
		optArr[4]=[];
		optArr[5]=[];
		
		for (idx=0; idx<ledDevices.length; idx++)
		{
			if($.inArray(ledDevices[idx], devRPiSPI) != -1)
				optArr[0].push(ledDevices[idx]);
			else if($.inArray(ledDevices[idx], devRPiPWM) != -1)
				optArr[1].push(ledDevices[idx]);
			else if($.inArray(ledDevices[idx], devRPiGPIO) != -1)
				optArr[2].push(ledDevices[idx]);
			else if($.inArray(ledDevices[idx], devNET) != -1)
				optArr[3].push(ledDevices[idx]);
			else if($.inArray(ledDevices[idx], devUSB) != -1)
				optArr[4].push(ledDevices[idx]);
			else
				optArr[5].push(ledDevices[idx]);
		}
		
		$("#leddevices").append(createSel(optArr[0], $.i18n('conf_leds_optgroup_RPiSPI')));
		$("#leddevices").append(createSel(optArr[1], $.i18n('conf_leds_optgroup_RPiPWM')));
		$("#leddevices").append(createSel(optArr[2], $.i18n('conf_leds_optgroup_RPiGPIO')));
		$("#leddevices").append(createSel(optArr[3], $.i18n('conf_leds_optgroup_network')));
		$("#leddevices").append(createSel(optArr[4], $.i18n('conf_leds_optgroup_usb')));
		$("#leddevices").append(createSel(optArr[5], $.i18n('conf_leds_optgroup_debug')));

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

		leds_html = '<img src="" id="image_preview" style="position:relative" />"';
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
		$('#led_0').css({"z-index":"10"});
		
		$('#image_preview').hide();		
		$('#image_preview').attr("width" , $('#leds_canvas').innerWidth()-2);		
		$('#image_preview').attr("height", $('#leds_canvas').innerHeight()-2);
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
	$('#leds_toggle_live_video').off().on("click", function() {
		setClassByBool('#leds_toggle_live_video',imageStreamActive,"btn-success","btn-danger");
		if ( imageStreamActive )
		{
			requestLedImageStop();
			$('#image_preview').hide();
		}
		else
		{
			$('#image_preview').show();
			requestLedImageStart();
		}
	});

	// ------------------------------------------------------------------
	$("#leds_custom_updsim").off().on("click", function() {
		if (validateText()){
			createLedPreview(JSON.parse($("#ledconfig").val()), 'text');
		}
	});
	
	// ------------------------------------------------------------------
	$("#leds_custom_save").off().on("click", function() {
		if (validateText())
			requestWriteConfig(JSON.parse('{"leds" :'+$("#ledconfig").val()+'}'));
	});

	// ------------------------------------------------------------------
	$('#leds_prev_toggle_num').off().on("click", function() {
		$('.led_prev_num').toggle();
		toggleClass('#leds_prev_toggle_num', "btn-danger", "btn-success");
	});
	// -------------------------------------------------------------
	$('#leds_cfg_nav a[data-toggle="tab"]').off().on('shown.bs.tab', function (e) {
		var target = $(e.target).attr("href") // activated tab
		if (target == "#menu_gencfg" && !ledsCustomCfgInitialized)
		{
			$('#leds_custom_updsim').trigger('click');
			ledsCustomCfgInitialized = true;
		}
	});

	// ------------------------------------------------------------------
	var conf_editor = null;
	$("#leddevices").off().on("change", function(event) {
		generalOptions  = parsedConfSchemaJSON.properties.device;
		specificOptions = parsedConfSchemaJSON.properties.alldevices[$(this).val()];
		//$('#ledDeviceOptions').html(JSON.stringify(generalOptions)+"<br>"+JSON.stringify(specificOptions));
		conf_editor = createJsonEditor('editor_container', {
			generalOptions : generalOptions,
			specificOptions : specificOptions,
		});
		
		values_general = {};
		values_specific = {};
		isCurrentDevice = (server.info.ledDevices.active == $(this).val());

		for(var key in parsedConfJSON.device){
			if (key != "type" && key in generalOptions.properties)
				values_general[key] = parsedConfJSON.device[key];
		};
		conf_editor.getEditor("root.generalOptions").setValue( values_general );

		if (isCurrentDevice)
		{
			specificOptions_val = conf_editor.getEditor("root.specificOptions").getValue()
			for(var key in specificOptions_val){
					values_specific[key] = (key in parsedConfJSON.device) ? parsedConfJSON.device[key] : specificOptions_val[key];
			};

			conf_editor.getEditor("root.specificOptions").setValue( values_specific );
		};

		$('#editor_container .well').css("background-color","white");
		$('#editor_container .well').css("border","none");
		$('#editor_container .well').css("box-shadow","none");
		
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
		if (conf_editor==null)
			return;

		ledDevice = $("#leddevices").val();
		result = {device:{}};
		
		general = conf_editor.getEditor("root.generalOptions").getValue();
		specific = conf_editor.getEditor("root.specificOptions").getValue();
		for(var key in general){
			result.device[key] = general[key];
		}

		for(var key in specific){
			result.device[key] = specific[key];
		}
		result.device.type=ledDevice;
		requestWriteConfig(result)
	});

	requestServerConfig();
});


