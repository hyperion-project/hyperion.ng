
function bindNavToContent(containerId, fileName, loadNow)
{
	$("#page-wrapper").off();
	$(containerId).on("click", function() {
		$("#page-wrapper").load("/content/"+fileName+".html");
	});
	if (loadNow)
	{
		$("#page-wrapper").load("/content/"+fileName+".html");
	}
}
