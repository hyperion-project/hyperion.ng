	var conf_editor = null;
	var createdCont = false;
	performTranslation();
	requestLoggingStart();

$(document).ready(function() {
	
	var messages;
	
	$('#conf_cont').append(createOptPanel('fa-reorder', $.i18n("edt_conf_log_heading_title"), 'editor_container', 'btn_submit'));
	if(showOptHelp)
	{
		$('#conf_cont').append(createHelpTable(schema.logger.properties, $.i18n("edt_conf_log_heading_title")));
		createHintH("intro", $.i18n('conf_logging_label_intro'), "log_head");
	}
	
	conf_editor = createJsonEditor('editor_container', {
		logger : schema.logger
	}, true, true);
	
	conf_editor.on('change',function() {
		conf_editor.validate().length ? $('#btn_submit').attr('disabled', true) : $('#btn_submit').attr('disabled', false);
	});
	
	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});

	function uploadLog()
	{
		var reportUrl = 'https://glot.io/snippets/';
		var log = "";
		var config = JSON.stringify(serverConfig, null, "\t").replace(/"/g, '\\"');
		var prios = serverInfo.info.priorities;
		var comps = serverInfo.info.components;
		var info;
		
		//create log
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
		//create general info
		info = "######## GENERAL ######## \n";
		info += 'Build:      '+serverInfo.info.hyperion[0].build+'\n';
		info += 'Build time: '+serverInfo.info.hyperion[0].time+'\n';
		info += 'Version:    '+serverInfo.info.hyperion[0].version+'\n';
		info += 'UI Lang:    '+storedLang+'\n';
		info += 'UI Access:  '+storedAccess+'\n';
		info += 'Avail Capt: '+serverInfo.info.grabbers.available+'\n\n';
		
		//create prios
		info += "######## PRIORITIES ######## \n";
		for(var i = 0; i<prios.length; i++)
		{
			info += prios[i].priority;
			if(prios[i].visible)
				info += ' VISIBLE!';
			else
				info += '         ';
			info += ' ('+prios[i].component+') Owner: '+prios[i].owner+'\n';
		}
		info += '\npriorities_autoselect: '+serverInfo.info.priorities_autoselect+'\n\n';
		
		//create comps
		info += '######## COMPONENTS ######## \n'
		for(var i = 0; i<comps.length; i++)
		{
			info += comps[i].enabled+' - '+comps[i].name+'\n';
		}
		
		$.ajax({
			url: 'https://snippets.glot.io/snippets',
	//		headers: { "Authorization": "Token 9ed92d37-36ca-4430-858f-47b6a3d4d535", "Access-Control-Allow-Origin": "*", "Access-Control-Allow-Methods": "GET,HEAD,OPTIONS,POST,PUT", "Access-Control-Allow-Headers": "Origin, X-Requested-With, Content-Type, Accept, Authorization" },
			crossDomain: true,
			contentType: 'application/json',
			type: 'POST',
			timeout: 7000,
			data: '{"language":"plaintext","title":"Hyperion '+currentVersion+' Report ('+serverConfig.general.name+' ('+serverInfo.info.ledDevices.active+'))","public":false,"files":[{"name":"Info","content":"'+info+'"},{"name":"Hyperion Log","content":"'+log+'"},{"name":"Hyperion Config","content":"'+config+'"}]}'
		})
		.done( function( data, textStatus, jqXHR ) {
			reportUrl += data.id;
			if(textStatus == "success")
			{
				$('#upl_link').html($.i18n('conf_logging_yourlink')+': <a href="'+reportUrl+'" target="_blank">'+reportUrl+'</a>');
				$("html, body").animate({ scrollTop: 9999 }, "fast");
			}
			else
			{
				$('#btn_logupload').attr("disabled", false);
				$('#upl_link').html('<span style="color:red">'+$.i18n('conf_logging_uplfailed')+'<span>');
			}
		})
		.fail( function( jqXHR, textStatus ) {
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
				$('#log_content').html('<pre><div id="logmessages" style="overflow:scroll;max-height:400px"></div></pre><button  class="btn btn-primary" id="btn_logupload">'+$.i18n('conf_logging_btn_pbupload')+'</button><button class="btn btn-success" id="btn_autoscroll" style="margin-left:10px;">'+$.i18n('conf_logging_btn_autoscroll')+'</button><div id="upl_link" style="margin-top:10px;font-weight:bold;"></div>');
				createdCont = true;
				
				$('#btn_autoscroll').off().on('click',function() {
					toggleClass('#btn_autoscroll', "btn-success", "btn-danger");
				});
				$('#btn_logupload').off().on('click',function() {
					uploadLog();
					$(this).attr("disabled", true);
					$('#upl_link').html($.i18n('conf_logging_uploading'))
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
