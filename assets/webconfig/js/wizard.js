$(document).ready( function() {
	//clear priority and other tasks if people reload the page or lost connection while a wizard was active
	$(hyperion).one("ready", function(event) {	
		if(getStorage("wizardactive") === 'true')	
		{
			requestPriorityClear();
			setStorage("wizardactive", false);
			if(getStorage("kodiAddress" != null))
			{
				kodiAddress = getStorage("kodiAddress");
				sendToKodi("stop");
			}
		}
	});
	
	function resetWizard()
	{
		$("#wizard_modal").modal('hide');
		clearInterval(wIntveralId);
		requestPriorityClear();
		setStorage("wizardactive", false);
		$('#wizp1').toggle(true);
		$('#wizp2').toggle(false);
		//cc
		if(withKodi)
			sendToKodi("stop");
		step = 0;
	}
	
	//rgb byte order wizard
	var wIntveralId;
	var new_rgb_order;

	function changeColor()
	{
		var color = $("#wiz_canv_color").css('background-color');

		if (color == 'rgb(255, 0, 0)')
		{
			$("#wiz_canv_color").css('background-color','rgb(0, 255, 0)');
			requestSetColor('0','255','0');
		}
		else
		{
			$("#wiz_canv_color").css('background-color','rgb(255, 0, 0)');
			requestSetColor('255','0','0');
		}
	}

	function startWizardRGB()
	{
		//create html
		$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_rgb_title'));
		$('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_rgb_title')+'</h4><p>'+$.i18n('wiz_rgb_intro1')+'</p><p style="font-weight:bold;">'+$.i18n('wiz_rgb_intro2')+'</p>');
		$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
		$('#wizp2_body').html('<p style="font-weight:bold">'+$.i18n('wiz_rgb_expl')+'</p>');
		$('#wizp2_body').append('<div class="form-group"><label>'+$.i18n('wiz_rgb_switchevery')+'</label><div class="input-group" style="width:100px"><select id="wiz_switchtime_select" class="form-control"></select><div class="input-group-addon">'+$.i18n('edt_append_s')+'</div></div></div>');
		$('#wizp2_body').append('<canvas id="wiz_canv_color" width="100" height="100" style="border-radius:60px;background-color:red; display:block; margin: 10px 0;border:4px solid grey;"></canvas><label>'+$.i18n('wiz_rgb_q')+'</label>');
		$('#wizp2_body').append('<table class="table borderless" style="width:200px"><tbody><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qrend')+'</label></td><td class="itd"><select id="wiz_r_select" class="form-control wselect"></select></td></tr><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qgend')+'</label></td><td class="itd"><select id="wiz_g_select" class="form-control wselect"></select></td></tr></tbody></table>');
		$('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_save"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saverestart')+'</button><button type="button" class="btn btn-primary" id="btn_wiz_checkok" style="display:none" data-dismiss="modal"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_ok')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>')
		
		//open modal
		$("#wizard_modal").modal({
			backdrop : "static",
			keyboard: false,
			show: true
		});
		
		//listen for continue
		$('#btn_wiz_cont').off().on('click',function() {
			beginWizardRGB();
			$('#wizp1').toggle(false);
			$('#wizp2').toggle(true);
		});
	}

	function beginWizardRGB()
	{	
		$("#wiz_switchtime_select").off().on('change',function() {
			clearInterval(wIntveralId);
			var time = $("#wiz_switchtime_select").val();
			wIntveralId = setInterval(function() { changeColor(); }, time*1000);
		});

		$('.wselect').change(function () {
			var rgb_order = parsedConfJSON.device.colorOrder.split("");
			var redS = $("#wiz_r_select").val();
			var greenS = $("#wiz_g_select").val();
			var blueS = rgb_order.toString().replace(/,/g,"").replace(redS, "").replace(greenS,"");
		
			for (var i = 0; i<rgb_order.length; i++)
			{
				if (redS == rgb_order[i])
					$('#wiz_g_select option[value='+rgb_order[i]+']').attr('disabled',true);		
				else
					$('#wiz_g_select option[value='+rgb_order[i]+']').attr('disabled',false);
				if (greenS == rgb_order[i])
					$('#wiz_r_select option[value='+rgb_order[i]+']').attr('disabled',true);
				else
					$('#wiz_r_select option[value='+rgb_order[i]+']').attr('disabled',false);	
			}
			
			if(redS != 'null' && greenS != 'null')
			{
				$('#btn_wiz_save').attr('disabled',false);

				for (var i = 0; i<rgb_order.length; i++)
				{
					if(rgb_order[i] == "r")
						rgb_order[i] = redS;
					else if(rgb_order[i] == "g")
						rgb_order[i] = greenS;
					else
						rgb_order[i] = blueS;
				}
				
				rgb_order = rgb_order.toString().replace(/,/g,"");
				
				if(redS == "r" && greenS == "g")
				{
					$('#btn_wiz_save').toggle(false);
					$('#btn_wiz_checkok').toggle(true);
				}
				else
				{
					$('#btn_wiz_save').toggle(true);
					$('#btn_wiz_checkok').toggle(false);
				}
				new_rgb_order = rgb_order
			}
			else
				$('#btn_wiz_save').attr('disabled',true);
		});
	
		$("#wiz_switchtime_select").append(createSelOpt('5','5'),createSelOpt('10','10'),createSelOpt('15','15'),createSelOpt('30','30'));
		$("#wiz_switchtime_select").trigger('change');
	
		$("#wiz_r_select").append(createSelOpt("null", ""),createSelOpt('r', $.i18n('general_col_red')),createSelOpt('g', $.i18n('general_col_green')),createSelOpt('b', $.i18n('general_col_blue')));
		$("#wiz_g_select").html($("#wiz_r_select").html());
		$("#wiz_r_select").trigger('change');
		
		requestSetColor('255','0','0');
		setTimeout(requestSetSource, 100, 'auto');
		setStorage("wizardactive", true);

		$('#btn_wiz_abort').off().on('click', resetWizard);

		$('#btn_wiz_checkok').off().on('click',function() {
			showInfoDialog('success', "", $.i18n('infoDialog_wizrgb_text'));
			resetWizard();
		});

		$('#btn_wiz_save').off().on('click',function() {
			resetWizard();
			parsedConfJSON.device.colorOrder = new_rgb_order;
			requestWriteConfig({"device" : parsedConfJSON.device});
			setTimeout(initRestart, 100);
		});
	}
	
	$('#btn_wizard_byteorder').off().on('click',startWizardRGB);
	
	//color calibration wizard
	var kodiAddress = document.location.hostname+':8080';
	var wiz_editor;
	var colorLength;
	var cobj;
	var step = 0;
	var withKodi = false;
	var profile = 0;
	var websAddress;
	var imgAddress;
	var vidAddress = "https://sourceforge.net/projects/hyperion-project/files/resources/vid/";
	var picnr = 0;
	var availVideos = ["Sweet_Cocoon","Caminandes_2_GranDillama","Caminandes_3_Llamigos"];
	
	if(getStorage("kodiAddress" != null))
		kodiAddress = getStorage("kodiAddress");
	
	function switchPicture(pictures)
	{	
		if(typeof pictures[picnr] === 'undefined')
			picnr = 0;
		
		sendToKodi('playP',pictures[picnr]);
		picnr++;
	}
	
	function sendToKodi(type, content, cb)
	{
		var command;
		
		if(type == "playP")
			content = imgAddress+content+'.png';
		if(type == "playV")
			content = vidAddress+content;
		
		if(type == "msg")
			command = '{"jsonrpc":"2.0","method":"GUI.ShowNotification","params":{"title": "'+$.i18n('wiz_cc_title')+'", "message": "'+content+'", "image":"info", "displaytime":5000 },"id":"1"}';
		else if (type == "stop")
			command = '{"jsonrpc":"2.0","method":"Player.Stop","params":{"playerid": 2},"id":"1"}';
		else if (type == "playP" || type == "playV")
			command = '{"jsonrpc":"2.0","method":"Player.Open","params":{"item":{"file":"' + content + '"}},"id":"1"}';
		else if (type == "rotate")
			command = '{"jsonrpc":"2.0","method":"Player.Rotate","params":{"playerid": 2},"id":"1"}';

		$.ajax({
			url: 'http://' + kodiAddress + '/jsonrpc',
			dataType: 'jsonp',
			jsonpCallback: 'jsonCallback',
			type: 'POST',
			async: true,
			timeout: 2000,
			data: 'request=' + encodeURIComponent( command )
		})
		.done( function( data, textStatus, jqXHR ) {
			if ( jqXHR.status == 200 && data['result'] == 'OK' && type == "msg")
				cb("success");
		})
		// Older Versions Of Kodi/XBMC Tend To Fail Due To CORS But Typically If A '200' Is Returned Then It Has Worked!
		.fail( function( jqXHR, textStatus ) {
			if ( jqXHR.status != 200 && type == "msg")
				cb("error")
		});	
	}
	
	function performAction()
	{
		var h;
		
		if(step == 1)
		{
			$('#wiz_cc_desc').html($.i18n('wiz_cc_chooseid'));
			updateWEditor(["id"]);
			$('#btn_wiz_back').attr("disabled", true)
		}
		else
			$('#btn_wiz_back').attr("disabled", false)
		
		if(step == 2)
		{
			updateWEditor(["white"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_white_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_white_title'));
				sendToKodi('playP',"white");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_white_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 3)
		{
			updateWEditor(["gammaRed","gammaGreen","gammaBlue"]);
			h = '<p>'+$.i18n('wiz_cc_adjustgamma')+'</p>';
			if(withKodi)
			{
				sendToKodi('playP',"HGradient");
				h +='<button id="wiz_cc_btn_sp" class="btn btn-primary">'+$.i18n('wiz_cc_btn_switchpic')+'</button>';
			}
			else
				h += '<p>'+$.i18n('wiz_cc_lettvshowm', "gey_1, grey_2, grey_3, HGradient, VGradient")+'</p>';
			$('#wiz_cc_desc').html(h);
			$('#wiz_cc_btn_sp').off().on('click', function(){
					switchPicture(["VGradient","grey_1","grey_2","grey_3","HGradient"]);
			});
		}
		if(step == 4)
		{
			updateWEditor(["red"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_red_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_red_title'));	
				sendToKodi('playP',"red");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_red_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 5)
		{
			updateWEditor(["green"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_green_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_green_title'));	
				sendToKodi('playP',"green");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_green_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 6)
		{
			updateWEditor(["blue"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_blue_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_blue_title'));
				sendToKodi('playP',"blue");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_blue_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 7)
		{
			updateWEditor(["cyan"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_cyan_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_cyan_title'));
				sendToKodi('playP',"cyan");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_cyan_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 8)
		{
			updateWEditor(["magenta"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_magenta_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_magenta_title'));
				sendToKodi('playP',"magenta");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_magenta_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 9)
		{
			updateWEditor(["yellow"]);
			h = $.i18n('wiz_cc_adjustit',$.i18n('edt_conf_color_yellow_title'));
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_yellow_title'));
				sendToKodi('playP',"yellow");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_yellow_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 10)
		{
			updateWEditor(["brightnessMin"]);
			h = $.i18n('wiz_cc_minBright');
			if(withKodi)
			{
				h += '<br/>'+$.i18n('wiz_cc_kodishould',$.i18n('edt_conf_color_black_title'));
				sendToKodi('playP',"black");
			}
			else
				h += '<br/>'+$.i18n('wiz_cc_lettvshow',$.i18n('edt_conf_color_black_title'));
			$('#wiz_cc_desc').html(h);
		}
		if(step == 11)
		{
			updateWEditor([""], true);
			h = '<p>'+$.i18n('wiz_cc_testintro')+'</p>';
			if(withKodi)
			{
				h += '<p>'+$.i18n('wiz_cc_testintrok')+'</p>';
				sendToKodi('stop');
				for(var i = 0; i<availVideos.length; i++)
				{
					var txt = availVideos[i].replace(/_/g," ");
					h +='<div><button id="'+availVideos[i]+'" class="btn btn-sm btn-primary videobtn"><i class="fa fa-fw fa-play"></i> '+txt+'</button></div>';
				}
				h +='<div><button id="stop" class="btn btn-sm btn-danger videobtn" style="margin-bottom:15px"><i class="fa fa-fw fa-stop"></i> '+$.i18n('wiz_cc_btn_stop')+'</button></div>';
			}
			else
				h += '<p>'+$.i18n('wiz_cc_testintrowok')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/vid/" target="_blank">'+$.i18n('wiz_cc_link')+'</a></p>';
			h += '<p>'+$.i18n('wiz_cc_summary')+'</p>';
			$('#wiz_cc_desc').html(h);
			
			$('.videobtn').off().on('click', function(e){
				if(e.target.id == "stop")
					sendToKodi("stop");
				else
					sendToKodi("playV",e.target.id+'.mp4');
				
				$(this).attr("disabled", true);
				setTimeout(function(){$('.videobtn').attr("disabled", false)},10000);
			});
			
			$('#btn_wiz_next').attr("disabled", true);
			$('#btn_wiz_save').toggle(true);
		}
		else
		{
			$('#btn_wiz_next').attr("disabled", false);
			$('#btn_wiz_save').toggle(false);
		}
	}
	
	function updateWEditor(el, all)
	{
		for (var key in cobj)
		{
			if(all === true || el[0] == key || el[1] == key || el[2] == key)
				$('#editor_container_wiz [data-schemapath*=".'+profile+'.'+key+'"]').toggle(true);
			else
				$('#editor_container_wiz [data-schemapath*=".'+profile+'.'+key+'"]').toggle(false);
		}
	}
	
	function startWizardCC()
	{
		//create html
		$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_cc_title'));
		$('#wizp1_body').html('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_cc_title')+'</h4><p>'+$.i18n('wiz_cc_intro1')+'</p><label>'+$.i18n('wiz_cc_kwebs')+'</label><input class="form-control" style="width:170px;margin:auto" id="wiz_cc_kodiip" type="text" placeholder="'+kodiAddress+'" value="'+kodiAddress+'" /><span id="kodi_status"></span><span id="multi_cali"></span>');
		$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont" disabled="disabled"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
		$('#wizp2_body').html('<div id="wiz_cc_desc" style="font-weight:bold"></div><div id="editor_container_wiz"></div>');
		$('#wizp2_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_back"><i class="fa fa-fw fa-chevron-left"></i>'+$.i18n('general_btn_back')+'</button><button type="button" class="btn btn-primary" id="btn_wiz_next">'+$.i18n('general_btn_next')+'<i style="margin-left:4px;"class="fa fa-fw fa-chevron-right"></i></button><button type="button" class="btn btn-warning" id="btn_wiz_save" style="display:none"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saverestart')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>')
		
		//open modal
		$("#wizard_modal").modal({
			backdrop : "static",
			keyboard: false,
			show: true
		});
		
		$('#wiz_cc_kodiip').off().on('change',function() {
			kodiAddress = $(this).val();
			setStorage("kodiAddress", kodiAddress);
			sendToKodi("msg", $.i18n('wiz_cc_kodimsg_start'), function(cb){
				if(cb == "error")
				{
					$('#kodi_status').html('<p style="color:red;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodidiscon')+'</p><p>'+$.i18n('wiz_cc_kodidisconlink')+' <a href="https://sourceforge.net/projects/hyperion-project/files/resources/Hyperion_calibration_pictures.zip/download" target="_blank">'+$.i18n('wiz_cc_link')+'</p>');
					withKodi = false;
				}
				else
				{
					$('#kodi_status').html('<p style="color:green;font-weight:bold;margin-top:5px">'+$.i18n('wiz_cc_kodicon')+'</p>');
					withKodi = true;
				}
				
				$('#btn_wiz_cont').attr('disabled', false);
			});
		});
		
		//listen for continue
		$('#btn_wiz_cont').off().on('click',function() {
			beginWizardCC();
			$('#wizp1').toggle(false);
			$('#wizp2').toggle(true);
		});
		
		$('#wiz_cc_kodiip').trigger("change")
		colorLength = parsedConfJSON.color.channelAdjustment;
		cobj = schema.color.properties.channelAdjustment.items.properties;
		websAddress = document.location.hostname+':'+parsedConfJSON.webConfig.port;
		imgAddress = 'http://'+websAddress+'/img/cc/';
		setStorage("wizardactive", true);
		
		//check profile count
		if(colorLength.length > 1)
		{
			$('#multi_cali').html('<p style="font-weight:bold;">'+$.i18n('wiz_cc_morethanone')+'</p><select id="wiz_select" class="form-control" style="width:200px;margin:auto"></select>');
			for(var i = 0; i<colorLength.length; i++)
				$('#wiz_select').append(createSelOpt(i,i+1+' ('+colorLength[i].id+')'));
			
			$('#wiz_select').off().on('change', function(){
				profile = $(this).val();
			});
		}
	
		//prepare editor
		wiz_editor = createJsonEditor('editor_container_wiz', {
			color : schema.color
		}, true, true);
		
		$('#editor_container_wiz h4').toggle(false);
		$('#editor_container_wiz .btn-group').toggle(false);
		$('#editor_container_wiz [data-schemapath="root.color.imageToLedMappingType"]').toggle(false);
		for(var i = 0; i<colorLength.length; i++)
			$('#editor_container_wiz [data-schemapath*="root.color.channelAdjustment.'+i+'."]').toggle(false);
	}
	
	function beginWizardCC()
	{
		
		$('#btn_wiz_next').off().on('click',function() {
			step++;
			performAction();
		});
		
		$('#btn_wiz_back').off().on('click',function() {
			step--;
			performAction();
		});
		
		$('#btn_wiz_abort').off().on('click', resetWizard);

		$('#btn_wiz_save').off().on('click',function() {
			console.log(wiz_editor.getValue())
			//requestWriteConfig(wiz_editor.getValue());
		//	resetWizard();
			//setTimeout(initRestart, 200);
		});

		wiz_editor.on("change", function(e){
			var val = wiz_editor.getEditor('root.color.channelAdjustment.'+profile+'').getValue();
			var temp = JSON.parse(JSON.stringify(val));
			delete temp.leds
			requestAdjustment(JSON.stringify(temp),"",true);
		});
		
		step++
		performAction();
	}
	
	$('#btn_wizard_colorcalibration').off().on('click', startWizardCC);

});