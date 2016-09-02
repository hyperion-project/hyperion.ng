
function loadNavContent(containerId,fileName)
{
	$(containerId).on("click", function() {
		$("#page-wrapper").load("/content/"+fileName+".html");
	}); 
}
