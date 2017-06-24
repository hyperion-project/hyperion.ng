$(document).ready(function() {
	var modalOpened = false;
	var ledsim_width = 540;
	var ledsim_height = 489;
	var dialog;
	var leds;
	var lC = false;
	
	$(hyperion).one("ready",function(){
		leds = serverConfig.leds;
		
		if(showOptHelp)
		{
			createHint('intro', $.i18n('main_ledsim_text'), 'ledsim_text');
			$('#ledsim_text').css({'margin':'10px 15px 0px 15px'});
			$('#ledsim_text .bs-callout').css("margin","0px")
		}
		
		if(getStorage('ledsim_width') != null)
		{
			ledsim_width = getStorage('ledsim_width');
			ledsim_height = getStorage('ledsim_height');
		}
		
		dialog = $("#ledsim_dialog").dialog({
			uiLibrary: 'bootstrap',
			resizable: true,
			modal: false,
			minWidth: 250,
			width: ledsim_width,
			minHeight: 350,
			height: ledsim_height,
			closeOnEscape: true,
			autoOpen: false,
			title: $.i18n('main_ledsim_title'),
			resize: function (e) {
				updateLedLayout();
			},
			opened: function (e) {
				if(!lC)
				{
					updateLedLayout();
					lC = true;
				}
				modalOpened = true;
				requestLedColorsStart();
			
				if($('#leds_toggle_live_video').hasClass('btn-success'))
					requestLedImageStart();
			},
			closed: function (e) {
				modalOpened = false;
			},
			resizeStop: function (e) {
				setStorage("ledsim_width", $("#ledsim_dialog").outerWidth());
				setStorage("ledsim_height", $("#ledsim_dialog").outerHeight());	
			}
		});
	});
	
	function updateLedLayout()
	{
		//calculate body size
		var canvas_height = $('#ledsim_dialog').outerHeight()-$('#ledsim_text').outerHeight()-$('[data-role=footer]').outerHeight()-$('[data-role=header]').outerHeight()-40;
		var canvas_width = $('#ledsim_dialog').outerWidth()-30;
		
		$('#leds_canvas').html("");
		leds_html = '<img src="" id="image_preview" style="position:relative" />';
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
		
		if($('#leds_toggle_num').hasClass('btn-success'))
			$('.led_num').toggle(true);
		
		if($('#leds_toggle').hasClass('btn-danger'))
			$('.led').toggle(false);
	
		$('#image_preview').attr("width" , canvas_width-1);		
		$('#image_preview').attr("height", canvas_height-1);
	}
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
	$('#leds_toggle_live_video').off().on("click", function() {
		setClassByBool('#leds_toggle_live_video',imageStreamActive,"btn-success","btn-danger");
		if ( imageStreamActive )
		{
			requestLedImageStop();
			$('#image_preview').removeAttr("src");
		}
		else
		{
			requestLedImageStart();
		}
	});
	
	// ------------------------------------------------------------------
	$(hyperion).on("cmd-ledcolors-ledstream-update",function(event){
		if (!modalOpened)
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
	$(hyperion).on("cmd-ledcolors-imagestream-update",function(event){
		if (!modalOpened)
		{
			requestLedImageStop();
		}
		else
		{
			imageData = (event.response.result.image);
			$("#image_preview").attr("src", imageData);
		}
	});
	
	$("#btn_open_ledsim").off().on("click", function(event) {
		dialog.open();
	});
});