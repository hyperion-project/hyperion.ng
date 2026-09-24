var VIDEO_WIDTH = 640,
		VIDEO_HEIGHT = 480,
		PORTRAIT,
		FIX = 2,
		CURRENT_ADJUSTMENT;

var FAST_COLOR = new FastAverageColor();


var is_mobile = mobile_check();

var TV_COLOR_LED = document.querySelector('#tv_color_led');
var WALL_COLOR_LED = document.querySelector('#wall_color_led');
var COLOR_TEXT = document.querySelector('#color_text');

var APPLY_BUTTON = document.querySelector('#calibrate_apply');

var video = document.querySelector('#source');
var CANVAS = document.querySelector('#canvas');
var ctx = CANVAS.getContext('2d');

var TV_PICKER = document.querySelector('#tv');
var WALL_PICKER = document.querySelector('#wall');

var TV_COLOR, WALL_COLOR, CURRENT_RGB = [255,255,255], NEW_RGB = [255,255,255], CALIBRATION_MODE, CALIBRATION_MODE_AUTO = 'white',
		/*
		COLOR_PICKERS = {
			'white': document.querySelector('#tv_white'), 'black': document.querySelector('#tv_black'), 'red': document.querySelector('#tv_red'), 'green': document.querySelector('#tv_green'), 'blue': document.querySelector('#tv_blue'), 'cyan': document.querySelector('#tv_cyan'), 'magenta': document.querySelector('#tv_magenta'), 'yellow': document.querySelector('#tv_yellow'), 'wall': WALL_PICKER
		},
		COLOR_LEDS = {
			'white': document.querySelector('#led_white'), 'black': document.querySelector('#led_black'), 'red': document.querySelector('#led_red'), 'green': document.querySelector('#led_green'), 'blue': document.querySelector('#led_blue'), 'cyan': document.querySelector('#led_cyan'), 'magenta': document.querySelector('#led_magenta'), 'yellow': document.querySelector('#led_yellow'), 'wall': document.querySelector('#led_wall')
		},
		*/
		 TV_COLORS = {};

function hyperion_api(data, callback) {
	$.ajax('/json-rpc', {
			data : JSON.stringify(data || {command:'serverinfo'}),
			//data : data || {command:'serverinfo'},
			//contentType : 'application/json',
			type : 'POST',
			success: function(json) {
				console.log(json);
				if (callback) callback(json);
			}
	});
}

function update_hyperion_adjustments() {
	hyperion_api(null, function(json){
		CURRENT_ADJUSTMENT = json.info.adjustment[0];
		CURRENT_RGB = CURRENT_ADJUSTMENT[CALIBRATION_MODE_AUTO];
		console.log(json.info.adjustment[0]);
	});
}

var NEW_ADJUSTMENTS = {
	gammaRed: 1,
	gammaGreen: 1,
	gammaBlue: 1,
	white: [255,255,255], // rgb(253,206,103)
	red: [255,32,32], // rgb(255,23,3)
	green: [32,255,32], // rgb(47,255,0)
	blue: [32,32,255], // rgb(32,14,230)
	cyan: [128,255,255], // rgb(4,255,90)
	magenta: [255,1,255], // rgb(82,3,255)
	yellow: [255,255,1], // rgb(255,206,1)
};

$(function(){

	update_hyperion_adjustments();


	$('#calibrate_init').on('click', function(){

		CALIBRATION_MODE = null;

		hyperion_api({
			command: "adjustment",
			adjustment: {

				gammaRed: 1,
				gammaGreen: 1,
				gammaBlue: 1,
				white: [255,255,255], // rgb(253,206,103)
				red: [255,32,32], // rgb(255,23,3)
				green: [32,255,32], // rgb(47,255,0)
				blue: [32,32,255], // rgb(32,14,230)
				cyan: [128,255,255], // rgb(4,255,90)
				magenta: [255,1,255], // rgb(82,3,255)
				yellow: [255,255,1], // rgb(255,206,1)

			},
		});

		$(this).addClass('auto');

		return false;
	});

	$('#calibrate_white').on('click', function(){

		CALIBRATION_MODE = 'white';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.white,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});


	$('#calibrate_red').on('click', function(){

		CALIBRATION_MODE = 'red';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.red,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_green').on('click', function(){

		CALIBRATION_MODE = 'green';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.green,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_blue').on('click', function(){

		CALIBRATION_MODE = 'blue';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.blue,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_cyan').on('click', function(){

		CALIBRATION_MODE = 'cyan';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.cyan,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_magenta').on('click', function(){

		CALIBRATION_MODE = 'magenta';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.magenta,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_yellow').on('click', function(){

		CALIBRATION_MODE = 'yellow';

		hyperion_api({
			command: 'color',
			color: CURRENT_ADJUSTMENT.yellow,
			priority: 50,
			origin: 'Calibrator',
		}, function(){
		});

		return false;
	});

	$('#calibrate_off').on('click', function(){

		CALIBRATION_MODE = null;

		hyperion_api({
			command: 'clear',
			priority: 50,
		}, function(){

		});

		return false;
	});

	$('#calibrate_apply').on('click', function(){

		hyperion_api({
			command: "adjustment",
			adjustment: {
				[CALIBRATION_MODE_AUTO]: NEW_RGB, // rgb(255,220,90)
			},
		}, function() {

			update_hyperion_adjustments();

		});

		return false;
	});

});


//var snap = document.querySelector('.snap');

let portrait = window.matchMedia("(orientation: portrait)");

PORTRAIT = portrait.matches;

portrait.addEventListener("change", function(e) {
		if(e.matches) {
				// Portrait mode
				PORTRAIT = true;
		} else {
				// Landscape
				PORTRAIT = false;
		}

		get_video();
		//update_canvas();
})

function mobile_check() {
	let check = false;
	(function(a){if(/(android|bb\d+|meego).+mobile|avantgo|bada\/|blackberry|blazer|compal|elaine|fennec|hiptop|iemobile|ip(hone|od)|iris|kindle|lge |maemo|midp|mmp|mobile.+firefox|netfront|opera m(ob|in)i|palm( os)?|phone|p(ixi|re)\/|plucker|pocket|psp|series(4|6)0|symbian|treo|up\.(browser|link)|vodafone|wap|windows ce|xda|xiino|android|ipad|playbook|silk/i.test(a)||/1207|6310|6590|3gso|4thp|50[1-6]i|770s|802s|a wa|abac|ac(er|oo|s\-)|ai(ko|rn)|al(av|ca|co)|amoi|an(ex|ny|yw)|aptu|ar(ch|go)|as(te|us)|attw|au(di|\-m|r |s )|avan|be(ck|ll|nq)|bi(lb|rd)|bl(ac|az)|br(e|v)w|bumb|bw\-(n|u)|c55\/|capi|ccwa|cdm\-|cell|chtm|cldc|cmd\-|co(mp|nd)|craw|da(it|ll|ng)|dbte|dc\-s|devi|dica|dmob|do(c|p)o|ds(12|\-d)|el(49|ai)|em(l2|ul)|er(ic|k0)|esl8|ez([4-7]0|os|wa|ze)|fetc|fly(\-|_)|g1 u|g560|gene|gf\-5|g\-mo|go(\.w|od)|gr(ad|un)|haie|hcit|hd\-(m|p|t)|hei\-|hi(pt|ta)|hp( i|ip)|hs\-c|ht(c(\-| |_|a|g|p|s|t)|tp)|hu(aw|tc)|i\-(20|go|ma)|i230|iac( |\-|\/)|ibro|idea|ig01|ikom|im1k|inno|ipaq|iris|ja(t|v)a|jbro|jemu|jigs|kddi|keji|kgt( |\/)|klon|kpt |kwc\-|kyo(c|k)|le(no|xi)|lg( g|\/(k|l|u)|50|54|\-[a-w])|libw|lynx|m1\-w|m3ga|m50\/|ma(te|ui|xo)|mc(01|21|ca)|m\-cr|me(rc|ri)|mi(o8|oa|ts)|mmef|mo(01|02|bi|de|do|t(\-| |o|v)|zz)|mt(50|p1|v )|mwbp|mywa|n10[0-2]|n20[2-3]|n30(0|2)|n50(0|2|5)|n7(0(0|1)|10)|ne((c|m)\-|on|tf|wf|wg|wt)|nok(6|i)|nzph|o2im|op(ti|wv)|oran|owg1|p800|pan(a|d|t)|pdxg|pg(13|\-([1-8]|c))|phil|pire|pl(ay|uc)|pn\-2|po(ck|rt|se)|prox|psio|pt\-g|qa\-a|qc(07|12|21|32|60|\-[2-7]|i\-)|qtek|r380|r600|raks|rim9|ro(ve|zo)|s55\/|sa(ge|ma|mm|ms|ny|va)|sc(01|h\-|oo|p\-)|sdk\/|se(c(\-|0|1)|47|mc|nd|ri)|sgh\-|shar|sie(\-|m)|sk\-0|sl(45|id)|sm(al|ar|b3|it|t5)|so(ft|ny)|sp(01|h\-|v\-|v )|sy(01|mb)|t2(18|50)|t6(00|10|18)|ta(gt|lk)|tcl\-|tdg\-|tel(i|m)|tim\-|t\-mo|to(pl|sh)|ts(70|m\-|m3|m5)|tx\-9|up(\.b|g1|si)|utst|v400|v750|veri|vi(rg|te)|vk(40|5[0-3]|\-v)|vm40|voda|vulc|vx(52|53|60|61|70|80|81|83|85|98)|w3c(\-| )|webc|whit|wi(g |nc|nw)|wmlb|wonu|x700|yas\-|your|zeto|zte\-/i.test(a.substr(0,4))) check = true;})(navigator.userAgent||navigator.vendor||window.opera);
	return check;
};

// Fix for iOS Safari from https://leemartin.dev/hello-webrtc-on-safari-11-e8bcb5335295
//video.setAttribute('autoplay', '');
//video.setAttribute('muted', '');
//video.setAttribute('playsinline', '')

function get_video() {

	var varraints = {
		audio: false,
		video: {
			facingMode: 'environment',
			//noiseSuppression: true,
			frameRate: 30,
			//width: 640,
			//height: 480,
			aspectRatio: is_mobile ? (PORTRAIT ? 3/4 : 4/3) : 4/3,
	/*
			advanced: [
				{
					exposureMode: 'manual',
					exposureTime: 4,
				}
			],
	*/
		}
	}

	//varraints = {audio: false, video: true};

	//alert(varraints.video.aspectRatio);

	navigator.mediaDevices.getUserMedia(varraints)
		.then(localMediaStream => {

			//video.pause();

			//var track = localMediaStream.getVideoTracks()[0];


			//console.log('The device supports the following capabilities: ', track.getCapabilities());
/*
						 // set manual exposure mode
						 track.applyvarraints({
								 advanced: [
										 {exposureMode: 'manual'}
								 ]})
								 .then(() => {
										 // set target value for exposure time
										 track.applyvarraints({
												 advanced: [
														 {exposureTime: 8}
												 ]})
												 .then(() => {
														 // success
														 console.log('The new device settings are: ', track.getSettings());
												 })
												 .catch(e => {
														 console.error('Failed to set exposure time', e);
												 });
								 })
								 .catch(e => {
										 console.error('Failed to set manual exposure mode', e);
								 });
 */

//  DEPRECIATION : 
//       The following has been depreceated by major browsers as of Chrome and Firefox.
//       video.src = window.URL.createObjectURL(localMediaStream);
//       Please refer to these:
//       Deprecated  - https://developer.mozilla.org/en-US/docs/Web/API/URL/createObjectURL
//       Newer Syntax - https://developer.mozilla.org/en-US/docs/Web/API/HTMLMediaElement/srcObject
			//console.dir(video);


			if ('srcObject' in video) {
				video.srcObject = localMediaStream;
			} else {
				video.src = URL.createObjectURL(localMediaStream);
			}
			// video.src = window.URL.createObjectURL(localMediaStream);

			video.play();
		})
		.catch(err => {
			console.error(`OH NO!!!!`, err);
		});
}

function update_canvas() {
	CANVAS.width = VIDEO_WIDTH;
	CANVAS.height = VIDEO_HEIGHT;
}

function refresh_canvas() {
	ctx.drawImage(video, 0, 0, VIDEO_WIDTH, VIDEO_HEIGHT);
	

	// take the pixels out
	
	//var pixels = ctx.getImageData(0, 0, VIDEO_WIDTH, VIDEO_HEIGHT);
	// mess with them
	// pixels = redEffect(pixels);

	//pixels = rgbSplit(pixels);
	//ctx.globalAlpha = 0.8;

	// pixels = greenScreen(pixels);
	
	// put them back
	//ctx.putImageData(pixels, 0, 0);

	requestAnimationFrame(function() {
		requestAnimationFrame(refresh_canvas);
	});
}




function takePhoto() {
	// played the sound
	
	//snap.currentTime = 0;
	//snap.play();

	// take the data out of the canvas
	var data = CANVAS.toDataURL('image/jpeg');
	var link = document.createElement('a');
	link.href = data;
	link.setAttribute('download', 'handsome');
	link.innerHTML = `<img src="${data}" alt="Handsome Man" />`;
	strip.insertBefore(link, strip.firstChild);
}

function redEffect(pixels) {
	for (let i = 0; i < pixels.data.length; i+=4) {
		pixels.data[i + 0] = pixels.data[i + 0] + 200; // RED
		pixels.data[i + 1] = pixels.data[i + 1] - 50; // GREEN
		pixels.data[i + 2] = pixels.data[i + 2] * 0.5; // Blue
	}
	return pixels;
}

function rgbSplit(pixels) {
	for (let i = 0; i < pixels.data.length; i+=4) {
		pixels.data[i - 150] = pixels.data[i + 0]; // RED
		pixels.data[i + 500] = pixels.data[i + 1]; // GREEN
		pixels.data[i - 550] = pixels.data[i + 2]; // Blue
	}
	return pixels;
}

video.addEventListener('canplay', function(){

	update_canvas();
	refresh_canvas();

	update_colors();

});

get_video();

function update_colors() {

	/*

	COLOR_TEXT.innerText = Object.keys(TV_COLORS).map(function(key){return key+":\t"+TV_COLORS[key]}).join("\n");

	var tmp_color_pickers = JSON.parse(JSON.stringify(Object.keys(COLOR_PICKERS)));

	function get_picker_color() {
		var color_picker_id = tmp_color_pickers.pop(),
				picker_element = COLOR_PICKERS[color_picker_id];

		FAST_COLOR.getColorAsync(CANVAS, {

			algorithm: 'simple', // simple, sqrt, dominant
			mode: 'speed', // speed, precision
			//step: 1,

			left: picker_element.offsetLeft - picker_element.clientWidth/2,
			top: picker_element.offsetTop - picker_element.clientHeight/2,
			width: picker_element.clientWidth,
			height: picker_element.clientHeight,

		}).then(color => {

				COLOR_LEDS[color_picker_id].style.backgroundColor = color.hex;
				TV_COLORS[color_picker_id] = color.value.slice(0,3);


				if (tmp_color_pickers.length) {
					get_picker_color();
				} else {
					requestAnimationFrame(update_colors);
				}
		});
	}

	get_picker_color();

	return false;
*/

	CURRENT_RGB = CURRENT_ADJUSTMENT[CALIBRATION_MODE_AUTO] || [255,255,255];

	FAST_COLOR.getColorAsync(CANVAS, {
		algorithm: 'simple', // simple, sqrt, dominant
		mode: 'speed', // speed, precision
		//step: 1,

		left: TV_PICKER.offsetLeft - TV_PICKER.clientWidth/2,
		top: TV_PICKER.offsetTop - TV_PICKER.clientHeight/2,
		width: TV_PICKER.clientWidth,
		height: TV_PICKER.clientHeight,

		ignoredColor: [0,0,0,255],

	}).then(color => {
		//console.log(color);
		delete(color.rgba);
		delete(color.hexa);

		TV_COLOR = color;
		TV_COLOR_LED.style.backgroundColor = color.hex;

		FAST_COLOR.getColorAsync(CANVAS, {
			algorithm: 'simple', // simple, sqrt, dominant
			mode: 'speed', // speed, precision
			//step: 1,

			left: WALL_PICKER.offsetLeft - WALL_PICKER.clientWidth/2,
			top: WALL_PICKER.offsetTop - WALL_PICKER.clientHeight/2,
			width: WALL_PICKER.clientWidth,
			height: WALL_PICKER.clientHeight,

			ignoredColor: [0,0,0,255],


		}).then(color => {
			//console.log(color);
			delete(color.rgba);
			delete(color.hexa);

			WALL_COLOR = color;
			WALL_COLOR_LED.style.backgroundColor = color.hex;

			var hsl_tv = rgb_to_hsl(TV_COLOR.value[0], TV_COLOR.value[1], TV_COLOR.value[2]);
			var hsl_wall = rgb_to_hsl(WALL_COLOR.value[0], WALL_COLOR.value[1], WALL_COLOR.value[2]);

			//COLOR_TEXT.innerText = ~~Math.abs(hsl_tv[0] - hsl_wall[0]);

			var tv_rgb = TV_COLOR.value.slice(0,3).map(function(n){return Math.max(n,1)}),
					wall_rgb = WALL_COLOR.value.slice(0,3).map(function(n){return Math.max(n,1)}),
					tv_rgb_min = Math.max(1, Math.min(...tv_rgb)),
					wall_rgb_min = Math.max(1, Math.min(...wall_rgb)),
					tv_rgb_max = Math.max(1, Math.max(...tv_rgb)),
					wall_rgb_max = Math.max(...wall_rgb),
					tv_adjusted_rgb = [(tv_rgb[0]/tv_rgb_min).toFixed(FIX), (tv_rgb[1]/tv_rgb_min).toFixed(FIX), (tv_rgb[2]/tv_rgb_min).toFixed(FIX)],
					wall_adjusted_rgb = [(wall_rgb[0]/wall_rgb_min).toFixed(FIX), (wall_rgb[1]/wall_rgb_min).toFixed(FIX), (wall_rgb[2]/wall_rgb_min).toFixed(FIX)];

			CALIBRATION_MODE_AUTO = null;


				if (tv_rgb_min > 130) CALIBRATION_MODE_AUTO = 'white';
				if (!CALIBRATION_MODE_AUTO && tv_adjusted_rgb[0]/tv_adjusted_rgb[1] > 2 && tv_adjusted_rgb[0]/tv_adjusted_rgb[2] > 2) CALIBRATION_MODE_AUTO = 'red';
				if (!CALIBRATION_MODE_AUTO && tv_adjusted_rgb[1]/tv_adjusted_rgb[0] > 2 && tv_adjusted_rgb[1]/tv_adjusted_rgb[2] > 2) CALIBRATION_MODE_AUTO = 'green';
				if (!CALIBRATION_MODE_AUTO && tv_adjusted_rgb[2]/tv_adjusted_rgb[0] > 2 && tv_adjusted_rgb[2]/tv_adjusted_rgb[1] > 2) CALIBRATION_MODE_AUTO = 'blue';

				if (!CALIBRATION_MODE_AUTO && Math.abs(tv_adjusted_rgb[1]-tv_adjusted_rgb[2]) < 1 && tv_adjusted_rgb[1]/tv_adjusted_rgb[0] > 1.5 && tv_adjusted_rgb[2]/tv_adjusted_rgb[0] > 1.5) CALIBRATION_MODE_AUTO = 'cyan';
				if (!CALIBRATION_MODE_AUTO && tv_adjusted_rgb[0]/tv_adjusted_rgb[1] > 2 && tv_adjusted_rgb[2]/tv_adjusted_rgb[1] > 2) CALIBRATION_MODE_AUTO = 'magenta';
				if (!CALIBRATION_MODE_AUTO && tv_adjusted_rgb[0]/tv_adjusted_rgb[2] > 1.5 && tv_adjusted_rgb[1]/tv_adjusted_rgb[2] > 1.5) CALIBRATION_MODE_AUTO = 'yellow';

				if (CALIBRATION_MODE) CALIBRATION_MODE_AUTO = CALIBRATION_MODE;

				$('#controls button').removeClass('auto');
				$('#calibrate_'+CALIBRATION_MODE_AUTO).addClass('auto');


			var accuracy_multiplier = [];


			if (CALIBRATION_MODE_AUTO == 'white') {
				NEW_RGB[0] = Math.round(255 * wall_rgb_min/wall_rgb[0]);
				NEW_RGB[1] = Math.round(255 * wall_rgb_min/wall_rgb[1]);
				NEW_RGB[2] = Math.round(255 * wall_rgb_min/wall_rgb[2]);
			}

			if (CALIBRATION_MODE_AUTO == 'red') {

				//NEW_RGB[0] = Math.round(CURRENT_RGB[0]);
				NEW_RGB[0] = Math.round(255 * tv_rgb[0] / wall_rgb[0]);
				NEW_RGB[1] = Math.round(CURRENT_RGB[1] * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(CURRENT_RGB[2] * tv_rgb[2] / wall_rgb[2]);
			}

			if (CALIBRATION_MODE_AUTO == 'green') {

				NEW_RGB[0] = Math.round(CURRENT_RGB[0] * tv_rgb[0] / wall_rgb[0]);
				//NEW_RGB[1] = Math.round(CURRENT_RGB[1]);
				NEW_RGB[1] = Math.round(255 * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(CURRENT_RGB[2] * tv_rgb[2] / wall_rgb[2]);
			}

			if (CALIBRATION_MODE_AUTO == 'blue') {
				NEW_RGB[0] = Math.round(CURRENT_RGB[0] * tv_rgb[0] / wall_rgb[0]);
				NEW_RGB[1] = Math.round(CURRENT_RGB[1] * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(255 * tv_rgb[2] / wall_rgb[2]);
			}

			if (CALIBRATION_MODE_AUTO == 'cyan') {

				NEW_RGB[0] = Math.round(128 * tv_rgb[0] / wall_rgb[0]);
				NEW_RGB[1] = Math.round(255 * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(255 * tv_rgb[2] / wall_rgb[2]);

			}

			if (CALIBRATION_MODE_AUTO == 'magenta') {

				NEW_RGB[0] = Math.round(255 * tv_rgb[0] / wall_rgb[0]);
				NEW_RGB[1] = Math.round(Math.max(1, CURRENT_RGB[1]) * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(255 * tv_rgb[2] / wall_rgb[2]);

			}

			if (CALIBRATION_MODE_AUTO == 'yellow') {

				NEW_RGB[0] = Math.round(255 * tv_rgb[0] / wall_rgb[0]);
				NEW_RGB[1] = Math.round(255 * tv_rgb[1] / wall_rgb[1]);
				NEW_RGB[2] = Math.round(CURRENT_RGB[2] * tv_rgb[2] / wall_rgb[2]);

			}

			// calculate current color accuracy

			if (CALIBRATION_MODE_AUTO) {
				var color1 = new Color('rgb(' + tv_rgb.join(',') + ')');
				var color2 = new Color('rgb(' + wall_rgb.join(',') + ')');
				var colorDistance = color1.deltaE76(color2);
				WALL_COLOR_LED.innerText = CALIBRATION_MODE_AUTO + ' accuracy: ' + Math.round(100 - colorDistance) + '%';

				APPLY_BUTTON.innerText = 'APPLY ' + CALIBRATION_MODE_AUTO + ' CALIBRATION';
			} else {
				APPLY_BUTTON.innerText = 'APPLY CALIBRATION';
			}

			// normalize new rgb

			var new_rgb_max = Math.max(...NEW_RGB),
				new_rgb_min = Math.min(...NEW_RGB);

			if (new_rgb_max > 255) {
				NEW_RGB = NEW_RGB.map(function(n){
					return Math.round(n * 255 / new_rgb_max);
				});
			}

			COLOR_TEXT.innerText = [
				'CURRENT COLOR: ' + CURRENT_RGB.join(':'),
				'MODE: ' + CALIBRATION_MODE_AUTO,
				'TV: ' + tv_rgb.join(':'),
				'WALL: ' + wall_rgb.join(':'),
				'TV ADJUSTED: ' + tv_adjusted_rgb.join(':'),
				'WALL ADJUSTED: ' + wall_adjusted_rgb.join(':'),

				NEW_RGB.join(':'),
			].join("\n");

			requestAnimationFrame(function(){
				//requestAnimationFrame(update_colors);
				setTimeout(update_colors, 1111);
			});

		});
	});

}

// COLOR MAGIC

function rgb_to_hsl(r,g,b) {
	r = r / 255;
	g = g / 255;
	b = b / 255;

	var max = Math.max(r, g, b),
			min = Math.min(r, g, b),
			h, s, l = (max + min) / 2;

	if (max == min) {
		h = s = 0; // achromatic
	} else {
		var d = max - min;
		s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

		switch (max) {
			case r:
				h = (g - b) / d + (g < b ? 6 : 0);
				break;
			case g:
				h = (b - r) / d + 2;
				break;
			case b:
				h = (r - g) / d + 4;
				break;
		}

		h /= 6;
	}

	h *= 360;
	s *= 100;
	l *= 100;

	return [h, s, l];
}

function hsl_to_rgb(h,s,l) {
	var h = h / 360;
	var s = s / 100;
	var l = l / 100;

	var r, g, b;

	if (s == 0) {
		r = g = b = l; // achromatic
	} else {
		function hue_to_rgb(p, q, t) {
			if (t < 0) t += 1;
			if (t > 1) t -= 1;
			if (t < 1/6) return p + (q - p) * 6 * t;
			if (t < 1/2) return q;
			if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
			return p;
		}

		var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		var p = 2 * l - q;

		r = hue_to_rgb(p, q, h + 1/3);
		g = hue_to_rgb(p, q, h);
		b = hue_to_rgb(p, q, h - 1/3);
	}

	r *= 255;
	g *= 255;
	b *= 255;

	return [r, g, b];
}

function delta_e(rgb_a, rgb_b) {
	var labA = rgb_to_lab(rgb_a[0], rgb_a[1], rgb_a[2]);
	var labB = rgb_to_lab(rgb_b[0], rgb_b[1], rgb_b[2]);
	var deltaL = labA[0] - labB[0];
	var deltaA = labA[1] - labB[1];
	var deltaB = labA[2] - labB[2];
	var c1 = Math.sqrt(labA[1] * labA[1] + labA[2] * labA[2]);
	var c2 = Math.sqrt(labB[1] * labB[1] + labB[2] * labB[2]);
	var deltaC = c1 - c2;
	var deltaH = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
	deltaH = deltaH < 0 ? 0 : Math.sqrt(deltaH);
	var sc = 1.0 + 0.045 * c1;
	var sh = 1.0 + 0.015 * c1;
	var deltaLKlsl = deltaL / (1.0);
	var deltaCkcsc = deltaC / (sc);
	var deltaHkhsh = deltaH / (sh);
	var i = deltaLKlsl * deltaLKlsl + deltaCkcsc * deltaCkcsc + deltaHkhsh * deltaHkhsh;
	return i < 0 ? 0 : Math.sqrt(i);
}

function rgb_to_lab(r,g,b){
	var r = r / 255, g = g / 255, b = b / 255, x, y, z;
	r = (r > 0.04045) ? Math.pow((r + 0.055) / 1.055, 2.4) : r / 12.92;
	g = (g > 0.04045) ? Math.pow((g + 0.055) / 1.055, 2.4) : g / 12.92;
	b = (b > 0.04045) ? Math.pow((b + 0.055) / 1.055, 2.4) : b / 12.92;
	x = (r * 0.4124 + g * 0.3576 + b * 0.1805) / 0.95047;
	y = (r * 0.2126 + g * 0.7152 + b * 0.0722) / 1.00000;
	z = (r * 0.0193 + g * 0.1192 + b * 0.9505) / 1.08883;
	x = (x > 0.008856) ? Math.pow(x, 1/3) : (7.787 * x) + 16/116;
	y = (y > 0.008856) ? Math.pow(y, 1/3) : (7.787 * y) + 16/116;
	z = (z > 0.008856) ? Math.pow(z, 1/3) : (7.787 * z) + 16/116;
	return [(116 * y) - 16, 500 * (x - y), 200 * (y - z)];
}

