
var ledsCustomCfgInitialized = false;
var finalLedArray = [];
var conf_editor = null;
var aceEdt = null;

function round(number) {
	var factor = Math.pow(10, 4);
	var tempNumber = number * factor;
	var roundedTempNumber = Math.round(tempNumber);
	return roundedTempNumber / factor;
};

function createLedPreview(leds, origin){

	if (origin == "classic"){
		$('#previewcreator').html($.i18n('conf_leds_layout_preview_originCL'));
		$('#leds_preview').css("padding-top", "56.25%");
	}
	else if(origin == "text"){
		$('#previewcreator').html($.i18n('conf_leds_layout_preview_originTEXT'));
		$('#leds_preview').css("padding-top", "56.25%");
	}
	else if(origin == "matrix"){
		$('#previewcreator').html($.i18n('conf_leds_layout_preview_originMA'));
		$('#leds_preview').css("padding-top", "100%");
	}

	$('#previewledcount').html($.i18n('conf_leds_layout_preview_totalleds', leds.length));
	$('#previewledpower').html($.i18n('conf_leds_layout_preview_ledpower', ((leds.length * 0.06)*1.1).toFixed(1)));

	$('.st_helper').css("border", "8px solid grey");

	var canvas_height = $('#leds_preview').innerHeight();
	var canvas_width = $('#leds_preview').innerWidth();

	var leds_html = "";
	for(var idx=0; idx<leds.length; idx++)
	{
		var led = leds[idx];
		var led_id='ledc_'+[idx];
		var bgcolor = "background-color:hsl("+(idx*360/leds.length)+",100%,50%);";
		var pos = "left:"+(led.h.min * canvas_width)+"px;"+
			"top:"+(led.v.min * canvas_height)+"px;"+
			"width:"+((led.h.max-led.h.min) * (canvas_width-1))+"px;"+
			"height:"+((led.v.max-led.v.min) * (canvas_height-1))+"px;";
		leds_html += '<div id="'+led_id+'" class="led" style="'+bgcolor+pos+'" title="'+idx+'"><span id="'+led_id+'_num" class="led_prev_num">'+idx+'</span></div>';
	}
	$('#leds_preview').html(leds_html);
	$('#ledc_0').css({"background-color":"black","z-index":"12"});
	$('#ledc_1').css({"background-color":"grey","z-index":"11"});
	$('#ledc_2').css({"background-color":"#A9A9A9","z-index":"10"});

	if($('#leds_prev_toggle_num').hasClass('btn-success'))
		$('.led_prev_num').css("display", "inline");

	// update ace Editor content
	aceEdt.set(finalLedArray);

}

function createClassicLeds(){
	//get values
	var ledstop = parseInt($("#ip_cl_top").val());
	var ledsbottom = parseInt($("#ip_cl_bottom").val());
	var ledsleft = parseInt($("#ip_cl_left").val());
	var ledsright = parseInt($("#ip_cl_right").val());
	var ledsglength = parseInt($("#ip_cl_glength").val());
	var ledsgpos = parseInt($("#ip_cl_gpos").val());
	var position = parseInt($("#ip_cl_position").val());
	var reverse = $("#ip_cl_reverse").is(":checked");

	//advanced values
	var ledsVDepth = parseInt($("#ip_cl_vdepth").val())/100;
	var ledsHDepth = parseInt($("#ip_cl_hdepth").val())/100;
	var edgeVGap = parseInt($("#ip_cl_edgegap").val())/100/2;
	//var cornerVGap = parseInt($("#ip_cl_cornergap").val())/100/2;
	var overlap = $("#ip_cl_overlap").val()/4000;

	//helper
	var edgeHGap = edgeVGap/(16/9);
	//var cornerHGap = cornerVGap/(16/9);
	var Vmin = 0.0 + edgeVGap;
	var Vmax = 1.0 - edgeVGap;
	var Hmin = 0.0 + edgeHGap;
	var Hmax = 1.0 - edgeHGap;
	var Hdiff = Hmax-Hmin;
	var Vdiff = Vmax-Vmin;
	var ledArray = [];

	function createFinalArray(array){
		finalLedArray = [];
		for(var i = 0; i<array.length; i++){
			var hmin = array[i].h.min;
			var hmax = array[i].h.max;
			var vmin = array[i].v.min;
			var vmax = array[i].v.max;
			finalLedArray[i] = { "h": { "max" : hmax, "min" : hmin }, "v": { "max": vmax, "min": vmin}}
		}
		createLedPreview(finalLedArray, 'classic');
	}

	function rotateArray(array, times){
		if (times > 0){
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

	function valScan(val)
	{
		if(val > 1)
			return 1;
		if(val < 0)
			return 0;
		return val;
	}

	function ovl(scan,val)
	{
		if(scan == "+")
			return valScan(val += overlap);
		else
			return valScan(val -= overlap);
	}

	function createLedArray(hmin, hmax, vmin, vmax){
		hmin = round(hmin);
		hmax = round(hmax);
		vmin = round(vmin);
		vmax = round(vmax);
		ledArray.push( { "h" : { "min" : hmin, "max" : hmax }, "v": { "min": vmin, "max": vmax }} );
	}

	function createTopLeds(){
		var step=(Hmax-Hmin)/ledstop;
		//if(cornerVGap != '0')
		//	step=(Hmax-Hmin-(cornerHGap*2))/ledstop;

		var vmin=Vmin;
		var vmax=vmin+ledsHDepth;
		for (var i = 0; i<ledstop; i++){
			var hmin = ovl("-",(Hdiff/ledstop*Number([i]))+edgeHGap);
			var hmax = ovl("+",(Hdiff/ledstop*Number([i]))+step+edgeHGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createLeftLeds(){
		var step=(Vmax-Vmin)/ledsleft;
		//if(cornerVGap != '0')
		//	step=(Vmax-Vmin-(cornerVGap*2))/ledsleft;

		var hmin=Hmin;
		var hmax=hmin+ledsVDepth;
		for (var i = ledsleft-1; i>-1; --i){
			var vmin = ovl("-",(Vdiff/ledsleft*Number([i]))+edgeVGap);
			var vmax = ovl("+",(Vdiff/ledsleft*Number([i]))+step+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createRightLeds(){
		var step=(Vmax-Vmin)/ledsright;
		//if(cornerVGap != '0')
		//	step=(Vmax-Vmin-(cornerVGap*2))/ledsright;

		var hmax=Hmax;
		var hmin=hmax-ledsVDepth;
		for (var i = 0; i<ledsright; i++){
			var vmin = ovl("-",(Vdiff/ledsright*Number([i]))+edgeVGap);
			var vmax = ovl("+",(Vdiff/ledsright*Number([i]))+step+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createBottomLeds(){
		var step=(Hmax-Hmin)/ledsbottom;
		//if(cornerVGap != '0')
		//	step=(Hmax-Hmin-(cornerHGap*2))/ledsbottom;

		var vmax=Vmax;
		var vmin=vmax-ledsHDepth;
		for (var i = ledsbottom-1; i>-1; i--){
			var hmin = ovl("-",(Hdiff/ledsbottom*Number([i]))+edgeHGap);
			var hmax = ovl("+",(Hdiff/ledsbottom*Number([i]))+step+edgeHGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	createTopLeds();
	createRightLeds();
	createBottomLeds();
	createLeftLeds();

	//check led gap pos
	if (ledsgpos+ledsglength > ledArray.length)
	{
		var mpos = Math.max(0,ledArray.length-ledsglength);
		$('#ip_cl_ledsgpos').val(mpos);
		ledsgpos = mpos;
	}

	//check led gap length
	if(ledsglength >= ledArray.length)
	{
		$('#ip_cl_ledsglength').val(ledArray.length-1);
		ledsglength = ledArray.length-ledsglength-1;
	}

	if(ledsglength != 0){
		ledArray.splice(ledsgpos, ledsglength);
	}

	if (position != 0){
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
	var ledshoriz = parseInt($("#ip_ma_ledshoriz").val());
	var ledsvert = parseInt($("#ip_ma_ledsvert").val());
	var cabling = $("#ip_ma_cabling").val();
	//var order = $("#ip_ma_order").val();
	var start = $("#ip_ma_start").val();

	var parallel = false
	var leds = []
	var hblock = 1.0 / ledshoriz
	var vblock = 1.0 / ledsvert

	if (cabling == "parallel"){
		parallel = true
	}

/**
 * Adds led to the hyperion config led array
 * @param {Number} x     Horizontal position in matrix
 * @param {Number} y     Vertical position in matrix
 */
	function addLed (x, y) {
		var hscanMin = x * hblock
		var hscanMax = (x + 1) * hblock
		var vscanMin = y * vblock
		var vscanMax = (y + 1) * vblock

		hscanMin = round(hscanMin);
		hscanMax = round(hscanMax);
		vscanMin = round(vscanMin);
		vscanMax = round(vscanMax);

		leds.push({
			h: {
				min: hscanMin,
				max: hscanMax
			},
			v: {
				min: vscanMin,
				max: vscanMax
			}
		})
	}

	var startYX = start.split('-')
	var startX = startYX[1] === 'right' ? ledshoriz - 1 : 0
	var startY = startYX[0] === 'bottom' ? ledsvert - 1 : 0
	var endX = startX === 0 ? ledshoriz - 1 : 0
	var endY = startY === 0 ? ledsvert - 1 : 0
	var forward = startX < endX

	var downward = startY < endY

	var x, y

	for (y = startY; downward && y <= endY || !downward && y >= endY; y += downward ? 1 : -1) {
		for (x = startX; forward && x <= endX || !forward && x >= endX; x += forward ? 1 : -1) {
			addLed(x, y)
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
	// translate
	performTranslation();

	//add intros
	if(window.showOptHelp)
	{
		createHintH("intro", $.i18n('conf_leds_device_intro'), "leddevice_intro");
		createHintH("intro", $.i18n('conf_leds_layout_intro'), "layout_intro");
		$('#led_vis_help').html('<div><div class="led_ex" style="background-color:black;margin-right:5px;margin-top:3px"></div><div style="display:inline-block;vertical-align:top">'+$.i18n('conf_leds_layout_preview_l1')+'</div></div><div class="led_ex" style="background-color:grey;margin-top:3px;margin-right:2px"></div><div class="led_ex" style="background-color: rgb(169, 169, 169);margin-right:5px;margin-top:3px;"></div><div style="display:inline-block;vertical-align:top">'+$.i18n('conf_leds_layout_preview_l2')+'</div>');
	}

	var slConfig = window.serverConfig.ledConfig;

	//restore ledConfig
	for(var key in slConfig)
	{
		if(typeof(slConfig[key]) === "boolean")
			$('#ip_cl_'+key).prop('checked', slConfig[key]);
		else
			$('#ip_cl_'+key).val(slConfig[key]);
	}

	function saveValues()
	{
		var ledConfig = {};
		for(var key in slConfig)
		{
			if(typeof(slConfig[key]) === "boolean")
				ledConfig[key] = $('#ip_cl_'+key).is(':checked');
			else if(Number.isInteger(slConfig[key]))
				ledConfig[key] = parseInt($('#ip_cl_'+key).val());
			else
				ledConfig[key] = $('#ip_cl_'+key).val();
		}
		setTimeout(requestWriteConfig, 100, {ledConfig});
	}

	// check access level and adjust ui
	if(storedAccess == "default")
	{
		$('#texfield_panel').toggle(false);
		$('#previewcreator').toggle(false);
	}

	//Wiki link
	$('#leds_wl').append('<p style="font-weight:bold">'+$.i18n('general_wiki_moreto',$.i18n('conf_leds_nav_label_ledlayout'))+buildWL("user/moretopics/ledarea","Wiki")+'</p>');

	// bind change event to all inputs
	$('.ledCLconstr').bind("change", function() {
		valValue(this.id,this.value,this.min,this.max);
		createClassicLeds();
	});

	$('.ledMAconstr').bind("change", function() {
		valValue(this.id,this.value,this.min,this.max);
		createMatrixLeds();
	});

	// v4 of json schema with diff required assignment - remove when hyperion schema moved to v4
	var ledschema = {"items":{"additionalProperties":false,"required":["h","v"],"properties":{"colorOrder":{"enum":["rgb","bgr","rbg","brg","gbr","grb"],"type":"string"},"h":{"additionalProperties":false,"properties":{"max":{"maximum":1,"minimum":0,"type":"number"},"min":{"maximum":1,"minimum":0,"type":"number"}},"type":"object"},"v":{"additionalProperties":false,"properties":{"max":{"maximum":1,"minimum":0,"type":"number"},"min":{"maximum":1,"minimum":0,"type":"number"}},"type":"object"}},"type":"object"},"type":"array"};
	//create jsonace editor
	aceEdt = new JSONACEEditor(document.getElementById("aceedit"),{
		mode: 'code',
		schema: ledschema,
		onChange: function(){
			var success = true;
			try{
				aceEdt.get();
			}
			catch(err)
			{
				success = false;
			}

			if(success)
			{
				$('#leds_custom_updsim').attr("disabled", false);
				$('#leds_custom_save').attr("disabled", false);
			}
			else
			{
				$('#leds_custom_updsim').attr("disabled", true);
				$('#leds_custom_save').attr("disabled", true);
			}
		}
	}, window.serverConfig.leds);

	//TODO: HACK! No callback for schema validation - Add it!
	setInterval(function(){
		if($('#aceedit table').hasClass('jsoneditor-text-errors'))
		{
			$('#leds_custom_updsim').attr("disabled", true);
			$('#leds_custom_save').attr("disabled", true);
		}
	},1000);

	$('.jsoneditor-menu').toggle();

	// leds to finalLedArray
	finalLedArray = window.serverConfig.leds;

	// create and update editor
	$("#leddevices").off().on("change", function() {
		var generalOptions  = window.serverSchema.properties.device;

		// Modified schema enty "hardwareLedCount" in generalOptions to minimum LedCount

		var specificOptions = window.serverSchema.properties.alldevices[$(this).val()];
		conf_editor = createJsonEditor('editor_container', {
			generalOptions : generalOptions,
			specificOptions : specificOptions,
		});

		var values_general = {};
		var values_specific = {};
		var isCurrentDevice = (window.serverConfig.device.type == $(this).val());

		for(var key in window.serverConfig.device){
			if (key != "type" && key in generalOptions.properties)
				values_general[key] = window.serverConfig.device[key];
		};
		conf_editor.getEditor("root.generalOptions").setValue( values_general );

		if (isCurrentDevice)
		{
			var specificOptions_val = conf_editor.getEditor("root.specificOptions").getValue()
			for(var key in specificOptions_val){
					values_specific[key] = (key in window.serverConfig.device) ? window.serverConfig.device[key] : specificOptions_val[key];
			};

			conf_editor.getEditor("root.specificOptions").setValue( values_specific );
		};

		// change save button state based on validation result
		conf_editor.validate().length ? $('#btn_submit_controller').attr('disabled', true) : $('#btn_submit_controller').attr('disabled', false);

		// led controller sepecific wizards
		if($(this).val() == "philipshue")
		{
			createHint("wizard", $.i18n('wiz_hue_title'), "btn_wiz_holder","btn_led_device_wiz");
			$('#btn_led_device_wiz').off().on('click',startWizardPhilipsHue);
		}
		else
		{
			$('#btn_wiz_holder').html("")
			$('#btn_led_device_wiz').off();
		}
	});

	// create led device selection
	var ledDevices = window.serverInfo.ledDevices.available;
	var devRPiSPI = ['apa102', 'apa104', 'ws2801', 'lpd6803', 'lpd8806', 'p9813', 'sk6812spi', 'sk6822spi', 'ws2812spi'];
	var devRPiPWM = ['ws281x'];
	var devRPiGPIO = ['piblaster'];
	var devNET = ['atmoorb', 'fadecandy', 'philipshue', 'nanoleaf', 'tinkerforge', 'tpm2net', 'udpe131', 'udpartnet', 'udph801', 'udpraw'];
	var devUSB = ['adalight', 'dmx', 'atmo', 'hyperionusbasp', 'lightpack', 'multilightpack', 'paintpack', 'rawhid', 'sedu', 'tpm2', 'karate'];

	var optArr = [[]];
	optArr[1]=[];
	optArr[2]=[];
	optArr[3]=[];
	optArr[4]=[];
	optArr[5]=[];

	for (var idx=0; idx<ledDevices.length; idx++)
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
	$("#leddevices").val(window.serverConfig.device.type);
	$("#leddevices").trigger("change");

	// validate textfield and update preview
	$("#leds_custom_updsim").off().on("click", function() {
		createLedPreview(aceEdt.get(), 'text');
	});

	// save led config and saveValues - passing textfield
	$("#btn_ma_save, #btn_cl_save").off().on("click", function() {
		requestWriteConfig({"leds" :finalLedArray});
		saveValues();
	});

	// save led config from textfield
	$("#leds_custom_save").off().on("click", function() {
		requestWriteConfig(JSON.parse('{"leds" :'+aceEdt.getText()+'}'));
		saveValues();
	});

	// toggle led numbers
	$('#leds_prev_toggle_num').off().on("click", function() {
		$('.led_prev_num').toggle();
		toggleClass('#leds_prev_toggle_num', "btn-danger", "btn-success");
	});

	// open checklist
	$('#leds_prev_checklist').off().on("click", function() {
		var liList = [$.i18n('conf_leds_layout_checkp1'),$.i18n('conf_leds_layout_checkp3'),$.i18n('conf_leds_layout_checkp2'),$.i18n('conf_leds_layout_checkp4')];
		var ul = document.createElement("ul");
		ul.className = "checklist"

		for(var i = 0; i<liList.length; i++)
		{
			var li = document.createElement("li");
			li.innerHTML = liList[i];
			ul.appendChild(li);
		}
		showInfoDialog('checklist', "", ul);
	});

	// nav
	$('#leds_cfg_nav a[data-toggle="tab"]').off().on('shown.bs.tab', function (e) {
		var target = $(e.target).attr("href") // activated tab
		if (target == "#menu_gencfg" && !ledsCustomCfgInitialized)
		{
			$('#leds_custom_updsim').trigger('click');
			ledsCustomCfgInitialized = true;
		}
	});

	// save led device config
	$("#btn_submit_controller").off().on("click", function(event) {

		var ledDevice = $("#leddevices").val();
		var result = {device:{}};

		var general = conf_editor.getEditor("root.generalOptions").getValue();
		var specific = conf_editor.getEditor("root.specificOptions").getValue();
		for(var key in general){
			result.device[key] = general[key];
		}

		for(var key in specific){
			result.device[key] = specific[key];
		}
		result.device.type=ledDevice;
		requestWriteConfig(result)
	});

	removeOverlay();
});
