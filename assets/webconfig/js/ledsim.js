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
		if (!modalOpened)
		{
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
		Object.getOwnPropertyNames(obj).forEach(function(val, idx, array) {
			window.serverInfo[val] = obj[val];
	  	});
		leds = window.serverConfig.leds
		updateLedLayout();
	});

	function resetImage(){
		imageCanvasNodeCtx.fillStyle = "rgb(225,225,225)"
		imageCanvasNodeCtx.fillRect(0, 0, canvas_width, canvas_height);
		var image = new Image();
		image.onload = function() {
			imageCanvasNodeCtx.drawImage(image, canvas_width*0.3, canvas_height*0.38);
		};
	 	image.src ='data:image/png;base64,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LV199dXo37//58ePH/9pYWHhk0Z5bm7uXdu2bdvCGNuW6B92paCsC4vbiRLBWQDoeEodHocydzpvpaY3sGnq1AlXrbaSJwz/WQvKjjjH2Lp1a1dtmySRoiiUlpbGR44cyadNm2b3foiz/2932xN6gfH9tCuuuKLmyJEj9xQVFf1NkqQsvZP94Fe/+tXm0aNHvzhixAhWVlZ2RgaLyy+/nNra2vYR0QnGWBYAKTMzc8qdd9456FNMvgwAABKUSURBVMYbbzyFbjrI28na+neeCD1gFwBqamqqWLduXbUuRnT7vV577TUA4E8++eQL995771y/33+lzhX0Hz58+M+feOKJhbfddltDvH8tOfQiikdd4gWqhihz2oDaavUyDpslDgw5FvH9gc11chvTSLJrp9gqWvRwidBZ2UTUyfK7/oeAs0vas2cPBg4cuLmmpmaZWD5ixIgVr7/++tiysrIzGtxs3bp1xzjnW4zv4vP5Si699NKrJEnyjh49+ox+HM65jFi0hjRDSbV///4tn332WS1jrEcAnTRpEgDQI4880rply5almqYdFWTVK6+44orrAUi6g0sSAJXiKomcTjWDSTtr53trV2bpjysqkZA4wgMXlA+2czgTyNeOJpJLkmRHjZ14PYkgxX9aMsJT9u/f/6lgMPiS8M6ZX/va1/5nxowZmWdKgVNdXY1rr71WPXXq1HOITcIGABQWFv74o48+mrpnzx5at25djymnHjgbjLH7AHQ4sCiKUn355Zf/VVfi9Ihab9u2rWP/sssu233ixIkHxfPFxcUPrFmzZvz+/fu5PQVlJrbWXiZ1OsHaLoaQVRQFOARDF4DaRC6wXbvFanaITo3h4H261G8FJJ1lhgPK20Xm/N+YZH0mrBIbNmx4QFXVDsdSn8837aWXXroFgMwYM6hGt1O/fv0wffp05OXlfdzW1vasMBh4J0+e/MaHH3444corr+zxt9K5lZsA3CeWL1++/N5QKBRkjLXjDNtei4uLVwWDweeFd0q78sorX927d28O51yzBSgJwLQJBBRvcnVCQNmwx/HqgQ07SZxzZmKbrahtF/DX1dVZUnid9XW6ZGDcZMQEgvWqZ5achsHW/qdST/P3WrlyZXlNTc0jBnVhjHn69u3746NHj04CIIlUoztp+fLl2LhxIwAgEAj8RFGU7UKHTr344ov/dvjw4YvEb3XVVVc5ppzGdyaiGwCsEM9/8sknv7v33nt3yrIcIqLQmQJonz59AAC33nor+8lPfrJYUZRdgpZ6wLx585ZrmuaxBKimt0LVswZLh81kl/DrdB8/bYDjDrW4ZCODMhN7a2VqsWTBDx48CDsWVxiNE2mp48qghkklnpxpBc5kgMn+zSj++9//Tq+++uqfI5FIR6wgl8uV36dPn0eWLFmSOXDgwB61b8mSJQCAuXPnAgBft27d9/SwJAZIC4qLi98Ih8PX68IY1q5da5hLbEEpADOTiH6BmJN/BygqKyvXX3fddW8ACGua1ngmlVENDQ0AgN///vf07LPPniorK7uTc24EjZJSUlK+4na7L7AEqFkQ1Kz1904WjLVVEiHx6mN21JBby/Xcrh5bJdQPfvADy8nkNu2DA3bXDBwoikKqqnIn4OyuzFlfX68gNrOj2zxeT9jDG2+8EYsXLw6/8cYbSzRNOy6YKi6566677jpy5EiPOYLs7Gy88847ICK6+uqrKw4ePHibOI1LkqR8j8fzTDQa/Wtra+uMsrIyn05xLf+L/s4ZRHQ9gP8HYBkE18uGhoYdP//5z3938ODBWsScQEI4iyassWPHbmpubv4dOsfxspwc7rILX+C2lj/tNLnA6VkmZj6f4CyuUaIl4DtRPQsZtDuO/KI3UzxzSEKAAqBQKAQTNba8x/hO3aGeK1asGDl+/PiMKVOmtIvR7ZMwn7Qyxg51t3etWrUKAPCd73zn2Pnnn7/oggsueNs4l5ubu3jr1q2bGGPvDBkyBBUVFd16xqlTp0RwaSNGjFj/8ccfLxw/fvwzbre7SD/ndblcX3e5XDOHDRtWpmna+wB2McYqEVvdTgKQhdjc0gsBzAXQH6ao9DU1NR9+73vfe2T9+vXHAdQhZvs9K26FQqzc6PLly3+7bNmyOR6PZ0K8e1yaBepsAqc6XQS3E5hEdz4H99sBVSSfVnU6Xdm6y+RvVVW54eWDxAsUU6KPrweOtgW8GDsoWQrDGHP/8Ic//JMYSSHJ5Caif+paS9YTKjF69Gg2ZsyYdxoaGn6dnZ292JBHL7zwwsevvvrqS1577bVaU6dMOnm9XkQiERowYAAvLS19b9GiRV999NFHH8vIyJgtqDIDjLGJjLGJ+nFYYE99sI+KwHfs2LGytLR0taqqjYyxen1pSBVnKYnf4eGHH67/+te/fnNpaelWxFmbRjJYWlEGVRMriAAHc0L1zsiROIqf5TP0SO5k0uBynY3s6RQ4IiKy8CRKBE7bTh2JRMSpaJbg7Abr10kGlyQpwBhLg75uaJI5RVGUbJ2KuM31J8P67tmzhwDQG2+88WtFUTpmCbnd7qFPP/30vYZ815NwK0bkwqNHj/Jp06bxlStXVmRmZl5XVla2NBQK2YUo8CFm00yzAicRRRsaGrY/88wzP5o4ceJKVVUbGGMniciJ3BlPgZlUuvvuu9lFF130aV1d3e3xntMBUHPm1iBNZANl+qwAQ3ngMYXJTMbEwvXIaX7h56cxxpge8tEp5SRB3gsIbQu43W5JVVWzqSUROMW6mF6X3+fzSZqmoa2tzVL73F0ziiRJgTM5iuueQGmGzMMYyzbOeTyetGQ73qJFi2qbm5vvJSJDQ8Oys7N/dPLkye/feOONVpQhS3ie43f76KOPKDU1VQFwauTIkY8/9dRT127ZsuXumpqad0Oh0HF95o21bUhVW4LB4MGKioo1zz777JKvfvWrixctWvQBYpPYT+iUM6FSSPxWbrc7oK8/2630q1/9imbOnMnGjBmzqqWl5UXxMR6PJ02WZdZJBk1gU3BCpSQAwVOnTi2vqqrq297ennr48OGDbrebxYm2HpeNZIxRU1PTurq6uoa6ujp3JBLR6uvrw0OGDImndbUsIyJfe3v7M1VVVf86efJkemtra2TXrl2nsrKy4ilzLOVhIkJLS8sLNTU1h2pqatJDoRBfs2ZNtcvlourq6rhsbbKptrb2xfr6+qra2lqvpmlST8DJGGPNzc11iMWsbdUVJLcfP368qKmpKa0hpm70JglSGjRo0PubNm26KRQKjQ6FQh5ZlqXm5mb6/PPP0xCLJdTxHRsbG+88duzYgKampoy6urr6ZNrf1tYGfeYM3XXXXXtkWT5SWlq6pri4uHDGjBmD8vLyclNSUtIZYx7d7BUJBoPN+/fvr928efPx8vLy5mPHjrUjFvGhhYjadGDGfd8xY8bg888/R3Nz89LKysphdXV16S0tLW07d+40ZkF1S1zYsGEDAYi+/fbb9w8ePHhfMBjMVlVVqqysPF5RUREFQF0AaiODOmIhJUkKHTp06G/vvvtuZk1Njc/r9WLkyJHtKSkpGhK7zlnZItnx48e3v/DCC/srKirSVVVlgwcPDhcUFKgJnOWtBpaUHTt2fLhs2bJ/HT16NItzLhcXF0dGjhypeDwecgpOYwCdMWPGP3fu3LkXscWK3R6PRykoKIiaPJbIrE1MNuXn528CsFt/jnwmtIiSJEU55/zqq69mOTk5zwHoq7PAYIyFicixHFZQUABVVdULL7xwDYAPcTqUi4bT0/+5oKF9QXiexBhrJyLHDum6SKIhNpEi+tFHH7UCqHvppZf2Cay7MTmdCRxbVJdPwwCMd3TEf3/++ed48cUXWWZm5l8A9BH+RY/iJOXl5WHWrFl8wYIFRwD8QecuDK4jCIDYCUIKB4oBjOTASAJGMeSOyMXaYV5clAKgFsBtAI7oH9uWEhIRBYNBVlVV5Wpra2OSJFFWVlY0Pz8/Ki4+hPgeQJ3Ot7a24sCBA+6amhpJVVVKT0/nQ4cOVfLy8lRhDUxHVLShoYFt375dLisrc4XDYRYIBNQJEyYokyZNUoVAYYnAyYkIW7duZU8++aRcUVHhaW9vZ5qmqf369VMeeughtbS0tJPM2VOuVOh0Es5MiuL0fFIJp6MlAKcjK/JuttMljPFWdYnXGWBTeqg5ZegcPFYSyrmFBZF6+C+MdzTeTzsD/8Sly+3Gt9MAKOwYIYWAYjIBtB/WDvN1Bugh/eXjmUc6fG8NCmewt4LW0axMiuvFI0zoJiIiWZbBOSeL9T5FYNmxzhSNRjvmlDLGyOVykR6gGjZKMFgBV1EUYoyhsbGRKisrkZqaSllZWejbt28n171zwDuoN/0HJ5dqQg0XyKQDJZHl4j0WGthklqHvVL9QHwksWjyvobh1ut1uOxY7nkNCl2OPxwMAlJubi9zc3C4O773g7E1nBKDmkOwWAHUUPc+GkiWSMYHELoRWoEsWnIlU5fHMKHbHXbYiIHvB2ZvOGAVNANBE4LEDS7xZJonmXXKbuhPdn0jJ42RRo25te6lmb/p3A9TJtDAnbG0iZY6dgiZpu6dDzTEcsLVxI+L3grM3nVWAmlciMgGUCZ2ZOWRxndg9ndhFeQLgJhsFwammthecvek/i4LGURLZRT5IRFntANYdcCbraxs31hESx0cC4jvOM9EX9j8dnOfQfNPe5JCCMopN4zYoqBexpQNkJJgYbQO4ePIgR+J4tE6ngCWzwFNSfrboOqvmXOntxBhr6e3qXwIKqvvgktwZoBmIBVUK2bC7tp3YivKgq2Y1kauUGTx2ywBY1UMO25lwQaFz9P+SpoIAzOvt6ucoQDWANIATEOWxrBBYlNBha/QCmNT7qc7NxGMTkHvTuQpQ3VOYa0CEgDYNaJHBT4VR29aOwzIHSIgiRgBYPK2JldDKYR/Kr2sSvdlELtu8z0zXG95eLE6WQADS4JJy4HFBkLlVEE6AK7zTc6WudNSpe0OiWAyJPIednAcSWm+V6FlfFaw3nW0KygFFA8IEtHCgPopTlf/CD2XAk60BjIMMZ0aKHVtPUTNyGLF1CVr1rZjb9RxCzAFTE8DTeSEw86JahgukR8hefevT98WtsR/LDD5ocGl9kOFZiQsG5cDjaoIWfRvB+muR2c8FhlcROvVLhCqz4HFxuNnpdkgAZ7GXE9cBN46t1gY3rxsuHmumcnEirhrnfvP19pkMeYXxXoCe6zKoxoEIB4IcOMWBFA2aK4QTYRVII8Bt1QcM2VXsW4oOvFY9t+jboABWYxu1pJwuEzjdQvZYgDPFBMgUIYtlUQaE1Exkel7H+bMnIsOvgLR7UPnxi2ipzYF7/GVIPe/HSM2LQqq/B23/AiRvJ6uSoQFlnfnHTgK8ZgFGxQTcqA2Q7cCZ6HozYKPCj4m1T+vt5uc+QNs50EKAVwUkHitrIcCnxdwBGbdgV8VjVQBnUKCSbfo2oufTUxuMOEnmSQh2VNNtAqfXRDXF7BGALQMA74+MlE2Y851BSM3iIP48atevQu32CNx0D2qPDkHh/IFwFy2Bf3g65GO3Ibw1pi/jp1EpCc028+0uxNcHi4yC+MqKvo0K45OqNz2agCprNhS482jaC9BzGaA8xrZGtBh7Cw1QOdCuAQEOeDRA1lnbjjk7qg1L22aillasbbiDrZVNIDVTUNm07zKVSxbXiGHxjS3RpSjIXoUZtxbDX8hB0W0I/v1O7H8lAi8Ahu1ojS5Dw/EnkLskDVL/m+CZ6QM7+hNEN7YZSGM6FZUFwdUlUE/ZNHK5HcqlENgJZiNOW5VJ+o9gNudOZ+rt5ucwQK9iwJ8JKoBWDnBdHg1ynXpqALMDZ9QEzlYdhGaAhrqA07zOhBl0ZnnUDOJ4YI3VzyCBcKTtKSwYPR/jbs2G53wClAq0rSzF5lU6GywDBAku9iec3J8B94GHkf1LP9gFN8Lzf86HS7sWkecOg6sMkocYqFPzufBoUfslm1gM25iCCf4OWYATpnHIDFZjvxegX4rUIVG9QGA8BkiXBrj1fUmMUSTKnapAFVtNAG0TtiE9xxaQNPN4ThRDogzqFlhYY5tiUhQZ/hS/aC1H7ZUD0edxN6R8AsI1CN9egL++DqSDIV0ioT4ZHmhQtd+jMO9GBB7zxMI0ohlYcy+0258AgoDs7jRKqTayoyGQW5VbbbU4siVPcF/UxNpGLRQG77BeU8u5SkH/TMB1DAjHBK6oCkQJCGsmImD+54oOuoiwNbIi6EYUoQ/FN4GYeTUpTplIfWEqf4DAAI349yWw3yAWnqKFgBveQd06IMRdyINqsl9oIHjgx634W+NCfPu7AHsSwH9lAFc9Djn7UeA7KYxV5WrEap2+hhTndcRxCiZqzIXyqOl6s6zL4SzW3KUEvNfr6teb/g3JWNlb388gol8SkaIHYThORJc4rYufrkcmoseFesqIaDoRyeZn9qbe1JvigFPYH01E7wkrFP6diIqSBZSpzu8Q0Um9vhAR3U9EGVbX9qbe1JvsgXQDER0VwPkIEQW6CyJT3aVEtFOoez0Rje4FaW/qTYmBlElELxIR18HTTETXnAnwmECaQkRPCiANEdFig+XtTb3pbKT/D85E97Jm8YYnAAAAAElFTkSuQmCC';
	}
});
