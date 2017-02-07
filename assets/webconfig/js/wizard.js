$(document).ready( function() {
	//clear priority if people reload the page or lost connection while a wizard was active
	$(hyperion).one("cmd-config-getschema", function(event) {
		if(getStorage("wizardactive") === 'true')
			requestPriorityClear();
			setStorage("wizardactive", false);
	});

	function resetWizard()
	{
		$("#wizard_modal").modal('hide');
		clearInterval(colorIntveralId);
		requestPriorityClear();
		setStorage("wizardactive", false);
		$('#wizp1').toggle(true);
		$('#wizp2').toggle(false);
		$('#wizp3').toggle(false);
	}

	//rgb byte order wizard
	var colorIntveralId;
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

	function startWizardPhilipsHue()
	{
		//create html
		$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_hue_title'));
		$('#wizp1_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!" style="margin-bottom:20px"><h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_hue_title')+'</h4><p>'+$.i18n('wiz_hue_intro1')+'</p><p style="font-weight:bold;">'+$.i18n('wiz_hue_intro2')+'</p>');
		$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
		$('#wizp2_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!" style="margin-bottom:20px"><br />');
		$('#wizp2_body').append('<span id="ip_alert" style="display:none; color:red; font-weight: bold;">'+$.i18n('wiz_hue_failure_ip')+'</span>');
		$('#wizp2_body').append('<span id="abortConnection" style="display:none; color:red; font-weight: bold;">'+$.i18n('wiz_hue_failure_connection')+'</span><br />');
		$('#wizp2_body').append('<div class="form-group"><label>'+$.i18n('wiz_hue_ip')+'</label><div class="input-group" style="width:150px"><input type="text" class="input-group" id="ip"></div></div>');
		$('#wizp2_body').append('<div class="form-group"><label>'+$.i18n('wiz_hue_username')+'</label<div class="input-group" style="width:150px"><input type="text" class="input-group" id="user" readonly="readonly"></div></div>');
		$('#wizp2_body').append('<div id="hue_lights" class="row"><table></table></div>');
		$('#wizp2_footer').html('<button type="button" class="btn btn-success" id="wiz_hue_create_user"> <i class="fa fa-floppy-o"></i>'+$.i18n('wiz_hue_create_user')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
		$('#wizp3_body').html('<span>'+$.i18n('wiz_hue_press_link')+'</span> <br /><br /><center><span id="connectionTime"></span><br /><img src="/img/hyperion/ring-alt.svg" /><center>');



		//open modal
		$("#wizard_modal").modal({
			backdrop : "static",
			keyboard: false,
			show: true
		});

		//listen for continue
		$('#btn_wiz_cont').off().on('click',function() {
			beginWizardHue();
			$('#wizp1').toggle(false);
			$('#wizp2').toggle(true);
		});
	}

	function beginWizardHue()
	{
		//listen for continue
		$('#wiz_hue_create_user').off().on('click',function() {
			doWizardHue();
		});
	}

	function doWizardHue()
	{
		console.log("ho");
	    var connectionRetries = 15;
			var data = {"devicetype":"hyperion#"+Date.now()};
			var UserInterval = setInterval(function(){
			$.ajax({
				type: "POST",
				url: 'http://'+$("#ip").val()+'/api',
				processData: false,
				timeout: 1000,
				contentType: 'application/json',
				data: JSON.stringify(data),
				success: function(r) {
						$('#wizp1').toggle(false);
						$('#wizp2').toggle(false);
						$('#wizp3').toggle(true);

						connectionRetries--;
						$("#connectionTime").html(connectionRetries);
						if(connectionRetries == 0) {
							abortConnection(UserInterval);
	          }
						else
						{
							$("#abortConnection").hide();
							$("#ip_alert").hide();
							if (typeof r[0].error != 'undefined') {
								console.log(connectionRetries+": link not pressed");
							}
						  if (typeof r[0].success != 'undefined') {
								$('#wizp1').toggle(false);
								$('#wizp2').toggle(true);
								$('#wizp3').toggle(false);
								$('#user').val(r[0].success.username);

								$( "#hue_lights" ).empty();
								get_hue_lights();
								clearInterval(UserInterval);
						  }
						}
				},
				error: function(XMLHttpRequest, textStatus, errorThrown) {
					$('#wizp1').toggle(false);
					$('#wizp2').toggle(true);
					$('#wizp3').toggle(false);
					$("#ip_alert").show();
					clearInterval(UserInterval);
				 }
			});
	  },1000);
	}

	function get_hue_lights(){
		$.ajax({
			type: "GET",
			url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/lights',
			processData: false,
			contentType: 'application/json',
			success: function(r) {
				for(var lightid in r){
					$('#hue_lights').append('<tr><td>ID: '+lightid+'</td><td>Name: '+r[lightid].name+'</td></tr>');
				}
			}
		});
	}

	function abortConnection(UserInterval){
		clearInterval(UserInterval);
		$('#wizp1').toggle(false);
		$('#wizp2').toggle(true);
		$('#wizp3').toggle(false);
		$("#abortConnection").show();

	}

	function startWizardRGB()
	{
		//create html
		$('#wiz_header').html('<i class="fa fa-magic fa-fw"></i>'+$.i18n('wiz_rgb_title'));
		$('#wizp1_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!" style="margin-bottom:20px"><h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('wiz_rgb_title')+'</h4><p>'+$.i18n('wiz_rgb_intro1')+'</p><p style="font-weight:bold;">'+$.i18n('wiz_rgb_intro2')+'</p>');
		$('#wizp1_footer').html('<button type="button" class="btn btn-primary" id="btn_wiz_cont"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_continue')+'</button><button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
		$('#wizp2_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!" style="margin-bottom:20px"><p style="font-weight:bold">'+$.i18n('wiz_rgb_expl')+'</p>');
		$('#wizp2_body').append('<div class="form-group"><label>'+$.i18n('wiz_rgb_switchevery')+'</label><div class="input-group" style="width:100px"><select id="wiz_switchtime_select" class="form-control"></select><div class="input-group-addon">'+$.i18n('edt_append_s')+'</div></div></div>');
		$('#wizp2_body').append('<canvas id="wiz_canv_color" width="100" height="100" style="border-radius:60px;background-color:red; display:block; margin: 10px 0;border:4px solid grey;"></canvas><label>'+$.i18n('wiz_rgb_q')+'</label>');
		$('#wizp2_body').append('<table class="table borderless" style="width:200px"><tbody><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qrend')+'</label></td><td class="itd"><select id="wiz_r_select" class="form-control wselect"></select></td></tr><tr><td class="ltd"><label>'+$.i18n('wiz_rgb_qgend')+'</label></td><td class="itd"><select id="wiz_g_select" class="form-control wselect"></select></td></tr></tbody></table>');
		$('#wizp2_footer').html('<button type="button" class="btn btn-success" id="btn_wiz_save"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saverestart')+'</button><button type="button" class="btn btn-primary" id="btn_wiz_checkok" style="display:none" data-dismiss="modal"><i class="fa fa-fw fa-check"></i>'+$.i18n('general_btn_ok')+'</button><button type="button" class="btn btn-danger" id="btn_wiz_abort"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>')

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
			clearInterval(colorIntveralId);
			var time = $("#wiz_switchtime_select").val();
			colorIntveralId = setInterval(function() { changeColor(); }, time*1000);
		});

		$('.wselect').change(function () {
			var rgb_order = order = parsedConfJSON.device.colorOrder.split("");
			var redS = $("#wiz_r_select").val();
			var greenS = $("#wiz_g_select").val();

			for (var i = 0; i<order.length; i++)
			{
				if (redS == order[i])
					$('#wiz_g_select option[value='+order[i]+']').attr('disabled',true);
				else
					$('#wiz_g_select option[value='+order[i]+']').attr('disabled',false);
				if (greenS == order[i])
					$('#wiz_r_select option[value='+order[i]+']').attr('disabled',true);
				else
					$('#wiz_r_select option[value='+order[i]+']').attr('disabled',false);
			}

			if(redS != 'null' && greenS != 'null')
			{
				$('#btn_wiz_save').attr('disabled',false);
				var blueS = parsedConfJSON.device.colorOrder.replace(redS, "").replace(greenS, "");

				rgb_order[0] = redS;
				rgb_order[1] = greenS;
				rgb_order[2] = blueS;
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

		$('#btn_wiz_abort').off().on('click',function() {
			resetWizard()
		});

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

	$('#btn_wizard_byteorder').off().on('click',function() {
			startWizardRGB();
	});

	$('#btn_wizard_philipshue').off().on('click',function() {
			startWizardPhilipsHue();
	});

});
