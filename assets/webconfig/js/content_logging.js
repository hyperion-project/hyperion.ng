
var conf_editor = null;

$(document).ready(function() {
	performTranslation();
	requestLoggingStart();
	
	schema = parsedConfSchemaJSON.properties;
	conf_editor = createJsonEditor('editor_container', {
		logger : schema.logger
	}, true);

	$('#editor_container h3').remove();
	
	$('#btn_submit').off().on('click',function() {
		requestWriteConfig(conf_editor.getValue());
	});
	
	$('#btn_autoscroll').off().on('click',function() {
		toggleClass('#btn_autoscroll', "btn-success", "btn-danger");
	});
	
	
	if (!loggingHandlerInstalled)
	{
		loggingHandlerInstalled = true;
		$(hyperion).on("cmd-logging-update",function(event){
			if ($("#logmessages").length == 0 && loggingStreamActive)
			{
				requestLoggingStop();
			}
			else
			{
				messages = (event.response.result.messages);
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
			}
		});
	}
});
