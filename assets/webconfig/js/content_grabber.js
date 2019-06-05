$(document).ready( function() {
	performTranslation();
	var conf_editor_v4l2 = null;
	var conf_editor_fg = null;
	var conf_editor_instCapt = null;

	function hideEl(el)
	{
		for(var i = 0; i<el.length; i++)
		{
			$('[data-schemapath*="root.framegrabber.'+el[i]+'"]').toggle(false);
		}
	}

	if(window.showOptHelp)
	{
		//fg
		$('#conf_cont').append(createRow('conf_cont_instCapt'));
		$('#conf_cont_instCapt').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
		$('#conf_cont_instCapt').append(createHelpTable(window.schema.instCapture.properties, $.i18n("edt_conf_instCapture_heading_title")));

		//fg
		$('#conf_cont').append(createRow('conf_cont_fg'));
		$('#conf_cont_fg').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
		$('#conf_cont_fg').append(createHelpTable(window.schema.framegrabber.properties, $.i18n("edt_conf_fg_heading_title")));

		//v4l
		$('#conf_cont').append(createRow('conf_cont_v4l'));
		$('#conf_cont_v4l').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
		$('#conf_cont_v4l').append(createHelpTable(window.schema.grabberV4L2.properties, $.i18n("edt_conf_v4l2_heading_title")));
	}
	else
	{
		$('#conf_cont').addClass('row');
		$('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_instCapture_heading_title"), 'editor_container_instCapt', 'btn_submit_instCapt'));
		$('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_fg_heading_title"), 'editor_container_fg', 'btn_submit_fg'));
		$('#conf_cont').append(createOptPanel('fa-camera', $.i18n("edt_conf_v4l2_heading_title"), 'editor_container_v4l2', 'btn_submit_v4l2'));
	}
	//instCapt
	conf_editor_instCapt = createJsonEditor('editor_container_instCapt', {
		instCapture: window.schema.instCapture
	}, true, true);

	conf_editor_instCapt.on('change',function() {
		conf_editor_instCapt.validate().length ? $('#btn_submit_instCapt').attr('disabled', true) : $('#btn_submit_instCapt').attr('disabled', false);
	});

	$('#btn_submit_instCapt').off().on('click',function() {
		requestWriteConfig(conf_editor_instCapt.getValue());
	});


	//fg
	conf_editor_fg = createJsonEditor('editor_container_fg', {
		framegrabber: window.schema.framegrabber
	}, true, true);

	conf_editor_fg.on('change',function() {
		conf_editor_fg.validate().length ? $('#btn_submit_fg').attr('disabled', true) : $('#btn_submit_fg').attr('disabled', false);
	});

	$('#btn_submit_fg').off().on('click',function() {
		requestWriteConfig(conf_editor_fg.getValue());
	});

	//vl4
	conf_editor_v4l2 = createJsonEditor('editor_container_v4l2', {
		grabberV4L2 : window.schema.grabberV4L2
	}, true, true);

	conf_editor_v4l2.on('change',function() {
		conf_editor_v4l2.validate().length ? $('#btn_submit_v4l2').attr('disabled', true) : $('#btn_submit_v4l2').attr('disabled', false);
	});

	$('#btn_submit_v4l2').off().on('click',function() {
		requestWriteConfig(conf_editor_v4l2.getValue());
	});

	//create introduction
	if(window.showOptHelp)
	{
		createHint("intro", $.i18n('conf_grabber_fg_intro'), "editor_container_fg");
		createHint("intro", $.i18n('conf_grabber_v4l_intro'), "editor_container_v4l2");
	}

	//hide specific options
	conf_editor_fg.on('ready',function() {
		var grabbers = window.serverInfo.grabbers.available;

		if(grabbers.indexOf('dispmanx') > -1)
			hideEl(["device","pixelDecimation"]);
		else if(grabbers.indexOf('x11') > -1)
			hideEl(["device","width","height"]);
		else if(grabbers.indexOf('osx')  > -1 )
			hideEl(["device","pixelDecimation"]);
		else if(grabbers.indexOf('amlogic')  > -1)
			hideEl(["pixelDecimation"]);
	});

	removeOverlay();
});
