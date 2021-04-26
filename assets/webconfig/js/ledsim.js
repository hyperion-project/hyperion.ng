$(document).ready(function() {
	var modalOpened = false;
	var ledsim_width = 540;
	var ledsim_height = 489;
	var dialog;
	var leds;
	var lC = false;
	var imageCanvasNodeCtx;
	var ledsCanvasNodeCtx;
	var canvas_height;
	var canvas_width;
	var twoDPaths = [];
	var toggleLeds, toggleLedsNum = false;

	/// add prototype for simple canvas clear() method
	CanvasRenderingContext2D.prototype.clear = function(){
		this.clearRect(0, 0, this.canvas.width, this.canvas.height)
	};

	function create2dPaths(){
		twoDPaths = [];
		for(var idx=0; idx<leds.length; idx++)
		{
			var led = leds[idx];
			twoDPaths.push( build2DPath(led.hmin * canvas_width, led.vmin * canvas_height, (led.hmax-led.hmin) * canvas_width, (led.vmax-led.vmin) * canvas_height, 5) );
		}
	}

	/**
	 * Draws a rounded rectangle into a new Path2D object, returns the created path.
	 * If you omit the last three params, it will draw a rectangle
	 * outline with a 5 pixel border radius
	 * @param {Number} x The top left x coordinate
	 * @param {Number} y The top left y coordinate
	 * @param {Number} width The width of the rectangle
	 * @param {Number} height The height of the rectangle
	 * @param {Number} [radius = 5] The corner radius; It can also be an object
	 *                 to specify different radii for corners
	 * @param {Number} [radius.tl = 0] Top left
	 * @param {Number} [radius.tr = 0] Top right
	 * @param {Number} [radius.br = 0] Bottom right
	 * @param {Number} [radius.bl = 0] Bottom left
	 * @return {Path2D} The final path
	 */
	 function build2DPath(x, y, width, height, radius) {
	  if (typeof radius == 'number') {
	    radius = {tl: radius, tr: radius, br: radius, bl: radius};
	  } else {
	    var defaultRadius = {tl: 0, tr: 0, br: 0, bl: 0};
	    for (var side in defaultRadius) {
	      radius[side] = radius[side] || defaultRadius[side];
	    }
	  }

	  var path = new Path2D();

	  path.moveTo(x + radius.tl, y);
	  path.lineTo(x + width - radius.tr, y);
	  path.quadraticCurveTo(x + width, y, x + width, y + radius.tr);
	  path.lineTo(x + width, y + height - radius.br);
	  path.quadraticCurveTo(x + width, y + height, x + width - radius.br, y + height);
	  path.lineTo(x + radius.bl, y + height);
	  path.quadraticCurveTo(x, y + height, x, y + height - radius.bl);
	  path.lineTo(x, y + radius.tl);
	  path.quadraticCurveTo(x, y, x + radius.tl, y);

	  return path;
	}

	$(window.hyperion).one("ready",function(){
		leds = window.serverConfig.leds;

		if(window.showOptHelp)
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

				setClassByBool('#leds_toggle_live_video', window.imageStreamActive, "btn-danger", "btn-success");
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
		// apply new serverinfos
		$(window.hyperion).on("cmd-config-getconfig",function(event){
			leds = event.response.info.leds;
			updateLedLayout();
		});
	});

	function printLedsToCanvas(colors)
	{
		// toggle leds, do not print
		if(toggleLeds)
			return;

		var useColor = false;
		var cPos = 0;
		ledsCanvasNodeCtx.clear();
		if(typeof colors != "undefined")
			useColor = true;

		// check size of ledcolors with leds length
		if(colors && colors.length/3 < leds.length)
			return;

		for(var idx=0; idx<leds.length; idx++)
		{
			var led = leds[idx];
			// can be used as fallback when Path2D is not available
			//roundRect(ledsCanvasNodeCtx, led.hmin * canvas_width, led.vmin * canvas_height, (led.hmax-led.hmin) * canvas_width, (led.vmax-led.vmin) * canvas_height, 4, true, colors[idx])
			//ledsCanvasNodeCtx.fillRect(led.hmin * canvas_width, led.vmin * canvas_height, (led.hmax-led.hmin) * canvas_width, (led.vmax-led.vmin) * canvas_height);

			ledsCanvasNodeCtx.fillStyle = (useColor) ?  "rgba("+colors[cPos]+","+colors[cPos+1]+","+colors[cPos+2]+",0.75)"  : "hsla("+(idx*360/leds.length)+",100%,50%,0.75)";
			ledsCanvasNodeCtx.fill(twoDPaths[idx]);
			ledsCanvasNodeCtx.stroke(twoDPaths[idx]);

			if(toggleLedsNum)
			{
				//ledsCanvasNodeCtx.shadowOffsetX = 1;
				//ledsCanvasNodeCtx.shadowOffsetY = 1;
				//ledsCanvasNodeCtx.shadowColor = "black";
				//ledsCanvasNodeCtx.shadowBlur = 4;
				ledsCanvasNodeCtx.fillStyle = "white";
				ledsCanvasNodeCtx.textAlign = "center";
				ledsCanvasNodeCtx.fillText(((led.name) ? led.name : idx), (led.hmin * canvas_width) + ( ((led.hmax-led.hmin) * canvas_width) / 2), (led.vmin * canvas_height) + ( ((led.vmax-led.vmin) * canvas_height) / 2));
			}

			// increment colorsPosition
			cPos += 3;
		}
	}

	function updateLedLayout()
	{
		//calculate body size
		canvas_height = $('#ledsim_dialog').outerHeight()-$('#ledsim_text').outerHeight()-$('[data-role=footer]').outerHeight()-$('[data-role=header]').outerHeight()-40;
		canvas_width = $('#ledsim_dialog').outerWidth()-30;

		$('#leds_canvas').html("");
		var leds_html = '<canvas id="image_preview_canv" width="'+canvas_width+'" height="'+canvas_height+'"  style="position: absolute; left: 0; top: 0; z-index: 99998;"></canvas>';
		leds_html += '<canvas id="leds_preview_canv" width="'+canvas_width+'" height="'+canvas_height+'"  style="position: absolute; left: 0; top: 0; z-index: 99999;"></canvas>';

		$('#leds_canvas').html(leds_html);

		imageCanvasNodeCtx = document.getElementById("image_preview_canv").getContext("2d");
		ledsCanvasNodeCtx = document.getElementById("leds_preview_canv").getContext("2d");
		create2dPaths();
		printLedsToCanvas();
		resetImage();
	}

	// ------------------------------------------------------------------
	$('#leds_toggle_num').off().on("click", function() {
		toggleLedsNum = !toggleLedsNum
		toggleClass('#leds_toggle_num', "btn-danger", "btn-success");
	});
	// ------------------------------------------------------------------

	$('#leds_toggle').off().on("click", function() {
		toggleLeds = !toggleLeds
		ledsCanvasNodeCtx.clear();
		toggleClass('#leds_toggle', "btn-success", "btn-danger");
	});

	// ------------------------------------------------------------------
	$('#leds_toggle_live_video').off().on("click", function() {
		setClassByBool('#leds_toggle_live_video',window.imageStreamActive,"btn-success","btn-danger");
		if ( window.imageStreamActive )
		{
			requestLedImageStop();
			resetImage();
		}
		else
		{
			requestLedImageStart();
		}
	});

	// ------------------------------------------------------------------
	$(window.hyperion).on("cmd-ledcolors-ledstream-update",function(event){
		if (!modalOpened)
		{
			requestLedColorsStop();
		}
		else
		{
			printLedsToCanvas(event.response.result.leds)
		}
	});

	// ------------------------------------------------------------------
	$(window.hyperion).on("cmd-ledcolors-imagestream-update",function(event){
		setClassByBool('#leds_toggle_live_video', window.imageStreamActive, "btn-danger", "btn-success");
		if (!modalOpened)
		{
			if ($('#leds_prev_toggle_live_video').length > 0)
				return;
			requestLedImageStop();
		}
		else
		{
			var imageData = (event.response.result.image);

			var image = new Image();
			image.onload = function() {
			    imageCanvasNodeCtx.drawImage(image, 0, 0, canvas_width, canvas_height);
			};
			image.src = imageData;
		}
	});

	$("#btn_open_ledsim").off().on("click", function(event) {
		dialog.open();
	});

	// ------------------------------------------------------------------
	$(window.hyperion).on("cmd-settings-update",function(event){

		var obj = event.response.data
		if ( obj.leds) {	
		  	console.log("ledsim: cmd-settings-update", event.response.data);			
			Object.getOwnPropertyNames(obj).forEach(function(val, idx, array) {
				window.serverInfo[val] = obj[val];
		  	});
			leds = window.serverConfig.leds
			updateLedLayout();
		}
	});

	function resetImage(){
	  if (getStorage("darkMode", false) == "on") {
			imageCanvasNodeCtx.clear();
	  } else {
			imageCanvasNodeCtx.fillStyle = "rgb(225,225,225)"
			imageCanvasNodeCtx.fillRect(0, 0, canvas_width, canvas_height);
	  }

		var image = document.getElementById('navbar_brand_logo');
		imageCanvasNodeCtx.drawImage(image, canvas_width / 2 - image.width / 2, canvas_height / 2 - image.height / 2, image.width, image.height);
	}
});
