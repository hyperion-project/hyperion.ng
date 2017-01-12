$(document).ready( function() {
	//clear priority if people reload the page or lost connection while a wizard was active
	var wizardStatus = localStorage.getItem("wizardactive");

	$(hyperion).one("cmd-config-getschema", function(event) {	
		if(wizardStatus)
			requestPriorityClear();
	});
	
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

	function startWizardRGB()
	{
		$("#wizard_modal").modal({
			backdrop : "static",
			keyboard: false,
			show: true
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

				var old_rgb_order = parsedConfJSON.device.colorOrder;
				
				if(old_rgb_order == rgb_order)
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
	
		$("#wiz_switchtime_select").html('');
		$("#wiz_switchtime_select").append(createSelOpt('5','5'),createSelOpt('10','10'),createSelOpt('15','15'),createSelOpt('30','30'));
		$("#wiz_switchtime_select").trigger('change');
	
		$("#wiz_r_select").html('');
		$("#wiz_r_select").append(createSelOpt("null", ""),createSelOpt('r', $.i18n('general_col_red')),createSelOpt('g', $.i18n('general_col_green')),createSelOpt('b', $.i18n('general_col_blue')));
		$("#wiz_g_select").html($("#wiz_r_select").html());
		$("#wiz_r_select").trigger('change');
		
		requestSetColor('255','0','0');
		localStorage.wizardactive = true;
	}

	$('#btn_wizard_byteorder').off().on('click',function() {
		startWizardRGB();
	});
	
	$('#btn_wiz_cont').off().on('click',function() {
		beginWizardRGB();
		$('#wizp1').toggle(false);
		$('#wizp2').toggle(true);
	});
	
	$('#btn_wiz_abort').off().on('click',function() {
		$("#wizard_modal").modal('hide');
		$("#wiz_canv_color").css('background-color','rgb(255, 0, 0)');
		clearInterval(colorIntveralId);
		requestPriorityClear();
		localStorage.wizardactive = false;
		$('#wizp1').toggle(true);
		$('#wizp2').toggle(false);
		$('#btn_wiz_save').toggle(true);
		$('#btn_wiz_checkok').toggle(false);
	});
	
	$('#btn_wiz_cancel').off().on('click',function() {
		$("#wizard_modal").modal('hide');
	});
	
	$('#btn_wiz_checkok').off().on('click',function() {
		showInfoDialog('success', $.i18n('infoDialog_wizrgb_title'), $.i18n('infoDialog_wizrgb_text'));
	});
	
	$('#btn_wiz_save').off().on('click',function() {
		var devConf = parsedConfJSON.device;
		devConf.colorOrder = new_rgb_order;
		requestWriteConfig(devConf);
		initRestart();
	});

});