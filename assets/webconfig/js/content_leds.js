
var ledsCustomCfgInitialized = false;
var finalLedArray = [];
var conf_editor = null;

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
			hmin = array[i].hscan.minimum;
			hmax = array[i].hscan.maximum;
			vmin = array[i].vscan.minimum;
			vmax = array[i].vscan.maximum;
			finalLedArray[i] = { "index" : i, "hscan": { "maximum" : hmax, "minimum" : hmin }, "vscan": { "maximum": vmax, "minimum": vmin}}
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
			return val = 1;
		if(val < 0)
			return val = 0;
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
		ledArray.push( { "hscan" : { "minimum" : hmin, "maximum" : hmax }, "vscan": { "minimum": vmin, "maximum": vmax }} );
	}
	
	function createTopLeds(){
		step=(Hmax-Hmin)/ledstop;
		//if(cornerVGap != '0')
		//	step=(Hmax-Hmin-(cornerHGap*2))/ledstop;

		vmin=Vmin;
		vmax=vmin+ledsHDepth;
		for (var i = 0; i<ledstop; i++){
			hmin = ovl("-",(Hdiff/ledstop*[i])+edgeHGap);
			hmax = ovl("+",(Hdiff/ledstop*[i])+step+edgeHGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}	
	}
	
	function createLeftLeds(){
		step=(Vmax-Vmin)/ledsleft;
		//if(cornerVGap != '0')
		//	step=(Vmax-Vmin-(cornerVGap*2))/ledsleft;

		hmin=Hmin;
		hmax=hmin+ledsVDepth;
		for (var i = ledsleft-1; i>-1; --i){
			vmin = ovl("-",(Vdiff/ledsleft*[i])+edgeVGap);
			vmax = ovl("+",(Vdiff/ledsleft*[i])+step+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}
	
	function createRightLeds(){
		step=(Vmax-Vmin)/ledsright;
		//if(cornerVGap != '0')
		//	step=(Vmax-Vmin-(cornerVGap*2))/ledsright;

		hmax=Hmax;
		hmin=hmax-ledsVDepth;
		for (var i = 0; i<ledsright; i++){
			vmin = ovl("-",(Vdiff/ledsright*[i])+edgeVGap);
			vmax = ovl("+",(Vdiff/ledsright*[i])+step+edgeVGap);
			createLedArray(hmin, hmax, vmin, vmax);			
		}
	}
	
	function createBottomLeds(){
		step=(Hmax-Hmin)/ledsbottom;
		//if(cornerVGap != '0')
		//	step=(Hmax-Hmin-(cornerHGap*2))/ledsbottom;
		
		vmax=Vmax;
		vmin=vmax-ledsHDepth;
		for (var i = ledsbottom-1; i>-1; i--){
			hmin = ovl("-",(Hdiff/ledsbottom*[i])+edgeHGap);
			hmax = ovl("+",(Hdiff/ledsbottom*[i])+step+edgeHGap);
			createLedArray(hmin, hmax, vmin, vmax);
		}
	}
	
	createLeftLeds(createBottomLeds(createRightLeds(createTopLeds())));

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
	var index = 0
	var leds = []
	var hblock = 1.0 / ledshoriz
	var vblock = 1.0 / ledsvert

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
	var startX = startYX[1] === 'right' ? ledshoriz - 1 : 0
	var startY = startYX[0] === 'bottom' ? ledsvert - 1 : 0
	var endX = startX === 0 ? ledshoriz - 1 : 0
	var endY = startY === 0 ? ledsvert - 1 : 0
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
	// translate
	performTranslation();
	
	//add intros
	if(showOptHelp)
	{
		createHintH("intro", $.i18n('conf_leds_device_intro'), "leddevice_intro");
		createHintH("intro", $.i18n('conf_leds_layout_intro'), "layout_intro");
		$('#led_vis_help').html('<div><div class="led_ex" style="background-color:black;margin-right:5px;margin-top:3px"></div><div style="display:inline-block;vertical-align:top">'+$.i18n('conf_leds_layout_preview_l1')+'</div></div><div class="led_ex" style="background-color:grey;margin-top:3px;margin-right:2px"></div><div class="led_ex" style="background-color: rgb(169, 169, 169);margin-right:5px;margin-top:3px;"></div><div style="display:inline-block;vertical-align:top">'+$.i18n('conf_leds_layout_preview_l2')+'</div>');
	}
	
	var slConfig = serverConfig.ledConfig;
	
	//restore ledConfig
	for(key in slConfig)
	{
		if(typeof(slConfig[key]) === "boolean")
			$('#ip_cl_'+key).prop('checked', slConfig[key]);
		else
			$('#ip_cl_'+key).val(slConfig[key]);
	}

	function saveValues()
	{
		var ledConfig = {};
		for(key in slConfig)
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
		$('#btn_ma_generate').toggle(false);
		$('#btn_cl_generate').toggle(false);
		$('#texfield_panel').toggle(false);
		$('#previewcreator').toggle(false);
	}
	else
	{
		$('#btn_ma_save').toggle(false);
		$('#btn_cl_save').toggle(false);
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
	var ledschema = {"items":{"additionalProperties":false,"required":["hscan","vscan","index"],"properties":{"clone":{"type":"integer"},"colorOrder":{"enum":["rgb","bgr","rbg","brg","gbr","grb"],"type":"string"},"hscan":{"additionalProperties":false,"properties":{"maximum":{"maximum":1,"minimum":0,"type":"number"},"minimum":{"maximum":1,"minimum":0,"type":"number"}},"type":"object"},"index":{"type":"integer"},"vscan":{"additionalProperties":false,"properties":{"maximum":{"maximum":1,"minimum":0,"type":"number"},"minimum":{"maximum":1,"minimum":0,"type":"number"}},"type":"object"}},"type":"object"},"type":"array"}
	//create jsonace editor
	var aceEdt = new JSONACEEditor(document.getElementById("aceedit"),{
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
	}, serverConfig.leds);
	
	//TODO: HACK! No callback for schema validation - Add it!
	setInterval(function(){
		if($('#aceedit table').hasClass('jsoneditor-text-errors'))
		{
			$('#leds_custom_updsim').attr("disabled", true);
			$('#leds_custom_save').attr("disabled", true);
		}
	},1000)
	
	$('.jsoneditor-menu').toggle();
	
	// leds to finalLedArray
	finalLedArray = serverConfig.leds;
	
	// cl/ma leds push to textfield
	$('#btn_cl_generate, #btn_ma_generate').off().on("click", function(e) {
		if(e.currentTarget.id == "btn_cl_generate")
			$('#collapse1').collapse('hide');
		else
			$('#collapse2').collapse('hide');
		
		aceEdt.set(finalLedArray);
		$('#collapse4').collapse('show');
	});
	
	// create and update editor
	$("#leddevices").off().on("change", function() {
		generalOptions  = serverSchema.properties.device;
		specificOptions = serverSchema.properties.alldevices[$(this).val()];
		conf_editor = createJsonEditor('editor_container', {
			generalOptions : generalOptions,
			specificOptions : specificOptions,
		});
		
		values_general = {};
		values_specific = {};
		isCurrentDevice = (serverInfo.ledDevices.active == $(this).val());

		for(var key in serverConfig.device){
			if (key != "type" && key in generalOptions.properties)
				values_general[key] = serverConfig.device[key];
		};
		conf_editor.getEditor("root.generalOptions").setValue( values_general );

		if (isCurrentDevice)
		{
			specificOptions_val = conf_editor.getEditor("root.specificOptions").getValue()
			for(var key in specificOptions_val){
					values_specific[key] = (key in serverConfig.device) ? serverConfig.device[key] : specificOptions_val[key];
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
	ledDevices = serverInfo.ledDevices.available
	devRPiSPI = ['apa102', 'ws2801', 'lpd6803', 'lpd8806', 'p9813', 'sk6812spi', 'sk6822spi', 'ws2812spi'];
	devRPiPWM = ['ws281x'];
	devRPiGPIO = ['piblaster'];
	devNET = ['atmoorb', 'fadecandy', 'philipshue', 'tinkerforge', 'tpm2net', 'udpe131', 'udpartnet', 'udph801', 'udpraw'];
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
	$("#leddevices").val(serverInfo.ledDevices.active);
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

	removeOverlay();
});



