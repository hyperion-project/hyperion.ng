	var conf_editor = null;
	var createdCont = false;
	performTranslation();
	requestLoggingStart();

$(document).ready(function() {
	
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
	
	if (!loggingHandlerInstalled)
	{
		loggingHandlerInstalled = true;
		$(hyperion).on("cmd-logging-update",function(event){
			messages = (event.response.result.messages);
			if(messages.length != 0 && !createdCont)
			{
				$('#log_content').html('<pre><div id="logmessages" style="overflow:scroll;max-height:400px"></div></pre><button  class="btn btn-primary" id="btn_pbupload">'+$.i18n('conf_logging_btn_pbupload')+'</button><button class="btn btn-success" id="btn_autoscroll" style="margin-left:10px;">'+$.i18n('conf_logging_btn_autoscroll')+'</button>');
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
