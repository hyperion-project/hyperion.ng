
$(document).ready(function() {
	requestLoggingStart();
	if (!loggingHandlerInstalled)
	{
		loggingHandlerInstalled = true;
		$(hyperion).on("cmd-logging-update",function(event){
			if ($("#logmessages").length == 0)
			{
				requestLoggingStop();
			}
			else
			{
				messages = (event.response.result.messages);
				for(var idx=0; idx<messages.length; idx++)
				{
					msg = messages[idx];
					$("#logmessages").html($("#logmessages").html()+"\n"+msg);
					
				}
			}
		});
	}
});
