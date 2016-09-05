
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


function toggleClass(obj,class1,class2)
{
	if ( $(obj).hasClass(class1))
	{
		$(obj).removeClass(class1);
		$(obj).addClass(class2);
	}
	else
	{
		$(obj).removeClass(class2);
		$(obj).addClass(class1);
	}
}


function setClassByBool(obj,enable,class1,class2)
{
	if (enable)
	{
		$(obj).removeClass(class1);
		$(obj).addClass(class2);
	}
	else
	{
		$(obj).removeClass(class2);
		$(obj).addClass(class1);
	}
}

