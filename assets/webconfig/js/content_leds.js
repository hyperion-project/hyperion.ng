
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
		var bgcolor = "background-color:hsla("+(idx*360/leds.length)+",100%,50%,0.75);";
		var pos = "left:"+(led.hmin * canvas_width)+"px;"+
			"top:"+(led.vmin * canvas_height)+"px;"+
			"width:"+((led.hmax-led.hmin) * (canvas_width-1))+"px;"+
			"height:"+((led.vmax-led.vmin) * (canvas_height-1))+"px;";
		leds_html += '<div id="'+led_id+'" class="led" style="'+bgcolor+pos+'" title="'+idx+'"><span id="'+led_id+'_num" class="led_prev_num">'+((led.name) ? led.name : idx)+'</span></div>';
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
	var overlap = $("#ip_cl_overlap").val()/100;

	//trapezoid values % -> float
	var ptblh  = parseInt($("#ip_cl_pblh").val())/100;
	var ptblv  = parseInt($("#ip_cl_pblv").val())/100;
	var ptbrh  = parseInt($("#ip_cl_pbrh").val())/100;
	var ptbrv  = parseInt($("#ip_cl_pbrv").val())/100;
	var pttlh  = parseInt($("#ip_cl_ptlh").val())/100;
	var pttlv  = parseInt($("#ip_cl_ptlv").val())/100;
	var pttrh  = parseInt($("#ip_cl_ptrh").val())/100;
	var pttrv  = parseInt($("#ip_cl_ptrv").val())/100;

	//helper
	var edgeHGap = edgeVGap/(16/9);
	var ledArray = [];

	function createFinalArray(array){
		finalLedArray = [];
		for(var i = 0; i<array.length; i++){
			var hmin = array[i].hmin;
			var hmax = array[i].hmax;
			var vmin = array[i].vmin;
			var vmax = array[i].vmax;
			finalLedArray[i] = { "hmax": hmax, "hmin": hmin, "vmax": vmax, "vmin": vmin }
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
		ledArray.push({ "hmin": hmin, "hmax": hmax, "vmin": vmin, "vmax": vmax });
	}

	function createTopLeds(){
		var steph = (pttrh - pttlh - (2*edgeHGap))/ledstop;
		var stepv = (pttrv - pttlv)/ledstop;

		for (var i = 0; i<ledstop; i++){
			var hmin = ovl("-",pttlh+(steph*Number([i]))+edgeHGap);
			var hmax = ovl("+",pttlh+(steph*Number([i+1]))+edgeHGap);
			var vmin = pttlv+(stepv*Number([i]));
			var vmax = vmin + ledsHDepth;
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createRightLeds(){
		var steph = (ptbrh - pttrh)/ledsright;
		var stepv = (ptbrv - pttrv - (2*edgeVGap))/ledsright;

		for (var i = 0; i<ledsright; i++){
			var hmax = pttrh+(steph*Number([i+1]));
			var hmin = hmax-ledsVDepth;
			var vmin = ovl("-",pttrv+(stepv*Number([i]))+edgeVGap);
			var vmax = ovl("+",pttrv+(stepv*Number([i+1]))+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createBottomLeds(){
		var steph = (ptbrh - ptblh - (2*edgeHGap))/ledsbottom;
		var stepv = (ptbrv - ptblv)/ledsbottom;

		for (var i = ledsbottom-1; i>-1; i--){
			var hmin = ovl("-",ptblh+(steph*Number([i]))+edgeHGap);
			var hmax = ovl("+",ptblh+(steph*Number([i+1]))+edgeHGap);
			var vmax= ptblv+(stepv*Number([i]));
			var vmin = vmax-ledsHDepth;
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}

	function createLeftLeds(){
		var steph = (ptblh - pttlh)/ledsleft;
		var stepv = (ptblv - pttlv - (2*edgeVGap))/ledsleft;

		for (var i = ledsleft-1; i>-1; i--){
			var hmin = pttlh+(steph*Number([i]));
			var hmax = hmin+ledsVDepth;
			var vmin = ovl("-",pttlv+(stepv*Number([i]))+edgeVGap);
			var vmax = ovl("+",pttlv+(stepv*Number([i+1]))+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}

	}

	//rectangle
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
			hmin: hscanMin,
			hmax: hscanMax,
			vmin: vscanMin,
			vmax: vscanMax
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

function migrateLedConfig(slConfig){

	var newLedConfig = {classic:{}, matrix:{}};

	//Default Classic layout
  	newLedConfig.classic = {
				"top"	 	: 8,
				"bottom"	: 8,
				"left"		: 5,
				"right"		: 5,
				"glength"	: 0,
				"gpos"		: 0,
				"position"	: 0,
				"reverse"	: false,
				"hdepth"	: 8,
				"vdepth"	: 5,
				"overlap"	: 0,
				"edgegap"	: 0
				}

	//Move Classic layout
	newLedConfig.classic.top 	= slConfig.top;
	newLedConfig.classic.bottom	= slConfig.bottom;
	newLedConfig.classic.left 	= slConfig.left;
	newLedConfig.classic.right	= slConfig.right;
	newLedConfig.classic.glength 	= slConfig.glength;
	newLedConfig.classic.position	= slConfig.position;
	newLedConfig.classic.reverse 	= slConfig.reverse;
	newLedConfig.classic.hdepth	= slConfig.hdepth;
	newLedConfig.classic.vdepth 	= slConfig.vdepth;
	newLedConfig.classic.overlap	= slConfig.overlap;

	//Default Matrix layout
	newLedConfig["matrix"] = { "ledshoriz": 10,
				"ledsvert" : 10,
				"cabling"  : "snake",
				"start"    : "top-left"
				}

	// Persit new structure
	requestWriteConfig({ledConfig:newLedConfig})
	return newLedConfig

}

function isEmpty(obj) {
    for(var key in obj) {
        if(obj.hasOwnProperty(key))
            return false;
    }
    return true;
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

	//Check, if structure is not aligned to expected -> migrate structure

	if ( isEmpty(slConfig.classic) )
	{
		slConfig = migrateLedConfig( slConfig );
	}

	//restore ledConfig - Classic
	for(var key in slConfig.classic)
	{
		if(typeof(slConfig.classic[key]) === "boolean")
			$('#ip_cl_'+key).prop('checked', slConfig.classic[key]);
		else
			$('#ip_cl_'+key).val(slConfig.classic[key]);
	}

	//restore ledConfig - Matrix
	for(var key in slConfig.matrix)
	{
		if(typeof(slConfig.matrix[key]) === "boolean")
			$('#ip_ma_'+key).prop('checked', slConfig.matrix[key]);
		else
			$('#ip_ma_'+key).val(slConfig.matrix[key]);
	}

	function saveValues()
	{
		var ledConfig = {classic:{}, matrix:{}};

		for(var key in slConfig.classic)
		{
			if(typeof(slConfig.classic[key]) === "boolean")
				ledConfig.classic[key] = $('#ip_cl_'+key).is(':checked');
			else if(Number.isInteger(slConfig.classic[key]))
				ledConfig.classic[key] = parseInt($('#ip_cl_'+key).val());
			else
				ledConfig.classic[key] = $('#ip_cl_'+key).val();
		}

		for(var key in slConfig.matrix)
		{
			if(typeof(slConfig.matrix[key]) === "boolean")
				ledConfig.matrix[key]  = $('#ip_ma_'+key).is(':checked');
			else if(Number.isInteger(slConfig.matrix[key]))
				ledConfig.matrix[key]  = parseInt($('#ip_ma_'+key).val());
			else
				ledConfig.matrix[key]  = $('#ip_ma_'+key).val();
		}
		requestWriteConfig({ledConfig});
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
	var ledschema = { "items": { "additionalProperties": false, "required": ["hmin", "hmax", "vmin", "vmax"], "properties": { "name": { "type": "string" }, "colorOrder": { "enum": ["rgb", "bgr", "rbg", "brg", "gbr", "grb"], "type": "string" }, "hmin": { "maximum": 1, "minimum": 0, "type": "number" }, "hmax": { "maximum": 1, "minimum": 0, "type": "number" }, "vmin": { "maximum": 1, "minimum": 0, "type": "number" }, "vmax": { "maximum": 1, "minimum": 0, "type": "number" } }, "type": "object" }, "type": "array" };
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

	// Modified schema entry "hardwareLedCount" in generalOptions to minimum LedCount
    var ledType = $(this).val();

    //philipshueentertainment backward fix
    if(ledType == "philipshueentertainment") ledType = "philipshue";

    var specificOptions = window.serverSchema.properties.alldevices[ledType];
		conf_editor = createJsonEditor('editor_container', {
			generalOptions : generalOptions,
			specificOptions : specificOptions,
		});

		var values_general = {};
		var values_specific = {};
		var isCurrentDevice = (window.serverConfig.device.type == ledType);

		for(var key in window.serverConfig.device) {
			if (key != "type" && key in generalOptions.properties) values_general[key] = window.serverConfig.device[key];
		};
		conf_editor.getEditor("root.generalOptions").setValue( values_general );

		if (isCurrentDevice)
		{
			var specificOptions_val = conf_editor.getEditor("root.specificOptions").getValue();
			for(var key in specificOptions_val){
				values_specific[key] = (key in window.serverConfig.device) ? window.serverConfig.device[key] : specificOptions_val[key];
			};
			conf_editor.getEditor("root.specificOptions").setValue( values_specific );
		};

		// change save button state based on validation result
		conf_editor.validate().length ? $('#btn_submit_controller').attr('disabled', true) : $('#btn_submit_controller').attr('disabled', false);

		// led controller sepecific wizards
		$('#btn_wiz_holder').html("");
		$('#btn_led_device_wiz').off();

    if(ledType == "philipshue") {
      $('#root_specificOptions_useEntertainmentAPI').bind("change", function() {
        var ledWizardType = (this.checked) ? "philipshueentertainment" : ledType;
        var data = { type: ledWizardType };
        var hue_title = (this.checked) ? 'wiz_hue_e_title' : 'wiz_hue_title';
        changeWizard(data, hue_title, startWizardPhilipsHue);
      });
      $("#root_specificOptions_useEntertainmentAPI").trigger("change");
    }
/*
    else if(ledType == "wled") {
    	    var ledWizardType = (this.checked) ? "wled" : ledType;
    	    var data = { type: ledWizardType };
    	    var wled_title = 'wiz_wled_title';
    	    changeWizard(data, wled_title, startWizardWLED);
	}
*/
    else if(ledType == "atmoorb") {
    	    var ledWizardType = (this.checked) ? "atmoorb" : ledType;
    	    var data = { type: ledWizardType };
    	    var atmoorb_title = 'wiz_atmoorb_title';
    	    changeWizard(data, atmoorb_title, startWizardAtmoOrb);
	}
	else if(ledType == "yeelight") {
		var ledWizardType = (this.checked) ? "yeelight" : ledType;
		var data = { type: ledWizardType };
		var yeelight_title = 'wiz_yeelight_title';
		changeWizard(data, yeelight_title, startWizardYeelight);
	}

    function changeWizard(data, hint, fn) {
      $('#btn_wiz_holder').html("")
			createHint("wizard", $.i18n(hint), "btn_wiz_holder","btn_led_device_wiz");
			$('#btn_led_device_wiz').off().on('click', data , fn);
    }
	});

  //philipshueentertainment backward fix
  if(window.serverConfig.device.type == "philipshueentertainment") window.serverConfig.device.type = "philipshue";

	// create led device selection
	var ledDevices = window.serverInfo.ledDevices.available;
	var devRPiSPI = ['apa102', 'apa104', 'ws2801', 'lpd6803', 'lpd8806', 'p9813', 'sk6812spi', 'sk6822spi', 'ws2812spi'];
	var devRPiPWM = ['ws281x'];
	var devRPiGPIO = ['piblaster'];
	var devNET = ['atmoorb', 'fadecandy', 'philipshue', 'nanoleaf', 'tinkerforge', 'tpm2net', 'udpe131', 'udpartnet', 'udph801', 'udpraw', 'wled', 'yeelight'];
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
