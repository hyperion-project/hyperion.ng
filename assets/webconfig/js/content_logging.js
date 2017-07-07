	var conf_editor = null;
	var createdCont = false;
	performTranslation();
	requestLoggingStart();

$(document).ready(function() {
	
	var messages;
	var reportUrl = 'https://report.hyperion-project.org/#';
	
	$('#conf_cont').append(createOptPanel('fa-reorder', $.i18n("edt_conf_log_heading_title"), 'editor_container', 'btn_submit'));
	if(showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(schema.logger.properties, $.i18n("edt_conf_log_heading_title")));
		createHintH("intro", $.i18n('conf_logging_label_intro'), "log_head");
	}
	$("#log_upl_pol").append('<span style="color:grey;font-size:80%">'+$.i18n("conf_logging_uplpolicy")+' '+buildWL("user/support#report_privacy_policy",$.i18n("conf_logging_contpolicy")));
	
	conf_editor = createJsonEditor('editor_container', {
		logger : schema.logger
	}, true, true);
	
	conf_editor.on('change',function() {
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});
	
	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});

	$('#btn_logupload').off().on('click',function() {
		uploadLog();
		$(this).attr("disabled", true);
		$('#upl_link').html($.i18n('conf_logging_uploading'))
	});
	
	//show prev uploads
	var ent;

	if(getStorage("prev_reports"))
	{
		ent = JSON.parse(getStorage("prev_reports"));
		$('#prev_reports').append('<h4 style="margin-top:30px">'+$.i18n('conf_logging_lastreports')+'</h4>');
		for(var i = 0; i<ent.length; i++)
		{
			$('#prev_reports').append('<p><a href="'+reportUrl+ent[i].id+'" target="_blank">'+ent[i].title+'('+ent[i].time+')</a></p>');
		}
	}
	else
		ent = [];
	
	function updateLastReports(id,time,title)
	{
		if(ent.length > 4)
			ent.pop();
		ent.unshift({"id": id ,"time": time,"title": title})
		setStorage("prev_reports",JSON.stringify(ent));
	}
	
	function uploadLog()
	{
		var log = "";
		var config = JSON.stringify(serverConfig, null).replace(/"/g, '\"');
		var prios = serverInfo.priorities;
		var comps = serverInfo.components;
		var sys = sysInfo.system;
		var shy = sysInfo.hyperion;
		var info;
		
		//create log
		if(messages)
		{
			for(var i = 0; i<messages.length; i++)
			{
				app_name = messages[i].appName;
				logger_name = messages[i].loggerName;
				function_ = messages[i].function;
				line = messages[i].line;
				file_name = messages[i].fileName;
				msg = messages[i].message;
				level_string = messages[i].levelString;
				debug = "";
			
				if(level_string == "DEBUG") {
					debug = "<"+file_name+":"+line+":"+function_+"()> ";
				}
				
				log += "["+app_name+" "+logger_name+"] <"+level_string+"> "+debug+msg+"\n";
			}
		}
		else
			log = "Log was empty!";

		//create general info
		info = "### GENERAL ### \n";
		info += 'Build:       '+shy.build+'\n';
		info += 'Build time:  '+shy.time+'\n';
		info += 'Version:     '+shy.version+'\n';
		info += 'UI Lang:     '+storedLang+' (BrowserL: '+navigator.language+')\n';
		info += 'UI Access:   '+storedAccess+'\n';
		info += 'Log lvl:     '+serverConfig.logger.level+'\n';
		info += 'Avail Capt:  '+serverInfo.grabbers.available+'\n\n';
		info += 'Distribution:'+sys.prettyName+'\n';
		info += 'Arch:        '+sys.architecture+'\n';
		info += 'Kernel:      '+sys.kernelType+' ('+sys.kernelVersion+' (WS: '+sys.wordSize+'))\n';
		info += 'Browser/OS:  '+navigator.userAgent+'\n\n';
		
		//create prios
		info += "### PRIORITIES ### \n";
		for(var i = 0; i<prios.length; i++)
		{
			info += prios[i].priority;
			if(prios[i].visible)
				info += ' VISIBLE!';
			else
				info += '         ';
			info += ' ('+prios[i].component+') Owner: '+prios[i].owner+'\n';
		}
		info += '\npriorities_autoselect: '+serverInfo.priorities_autoselect+'\n\n';
		
		//create comps
		info += '### COMPONENTS ### \n'
		for(var i = 0; i<comps.length; i++)
		{
			info += comps[i].enabled+' - '+comps[i].name+'\n';
		}
		
		//escape data
		info = JSON.stringify(info);
		log = JSON.stringify(log);
		config = JSON.stringify(config);
		var title = 'Hyperion '+currentVersion+' Report ('+serverConfig.general.name+' ('+serverInfo.ledDevices.active+'))';

		$.ajax({
			url: 'https://api.hyperion-project.org/report.php',
			crossDomain: true,
			contentType: 'application/json',
			type: 'POST',
			timeout: 7000,
			data: '{"title":"'+title+'","info":'+info+',"log":'+log+',"config":'+config+'}'
		})
		.done( function( data, textStatus, jqXHR ) {
			reportUrl += data.id;
			if(textStatus == "success")
			{
				$('#upl_link').html($.i18n('conf_logging_yourlink')+': <a href="'+reportUrl+'" target="_blank">'+reportUrl+'</a>');
				$("html, body").animate({ scrollTop: 9999 }, "fast");
				updateLastReports(data.id,data.time,title);
			}
			else
			{
				$('#btn_logupload').attr("disabled", false);
				$('#upl_link').html('<span style="color:red">'+$.i18n('conf_logging_uplfailed')+'<span>');
			}
		})
		.fail( function( jqXHR, textStatus ) {
			console.log(jqXHR,textStatus);
			$('#btn_logupload').attr("disabled", false);
			$('#upl_link').html('<span style="color:red">'+$.i18n('conf_logging_uplfailed')+'<span>');
		});
	}

	if (!loggingHandlerInstalled)
	{
		loggingHandlerInstalled = true;
		$(hyperion).on("cmd-logging-update",function(event){
			messages = (event.response.result.messages);
			if(messages.length != 0 && !createdCont)
			{
				$('#log_content').html('<pre><div id="logmessages" style="overflow:scroll;max-height:400px"></div></pre><button class="btn btn-success" id="btn_autoscroll"><i class="fa fa-long-arrow-down fa-fw"></i>'+$.i18n('conf_logging_btn_autoscroll')+'</button>');
				createdCont = true;
				
				$('#btn_autoscroll').off().on('click',function() {
					toggleClass('#btn_autoscroll', "btn-success", "btn-danger");
				});
			}
			for(var idx=0; idx<messages.length; idx++)
			{
				app_name = messages[idx].appName;
				logger_name = messages[idx].loggerName;
				function_ = messages[idx].function;
				line = messages[idx].line;
				file_name = messages[idx].fileName;
				msg = messages[idx].message;
				level_string = messages[idx].levelString;
				
				var debug = "";
				
				if(level_string == "DEBUG") {
					debug = "&lt;"+file_name+":"+line+":"+function_+"()&gt; ";
				}
				
				$("#logmessages").html($("#logmessages").html()+"\n <code>"+"["+app_name+" "+logger_name+"] &lt;"+level_string+"&gt; "+debug+msg+"</code>");
			}
			if($("#btn_autoscroll").hasClass('btn-success')){
				$('#logmessages').stop().animate({
					scrollTop: $('#logmessages')[0].scrollHeight
				}, 800);
			}
		});
	}
	
	removeOverlay();
});
