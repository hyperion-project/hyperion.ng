
function bindNavToContent(containerId, fileName, loadNow)
{
	$("#page-content").off();
	$(containerId).on("click", function() {
		$("#page-content").load("/content/"+fileName+".html");
	});
	if (loadNow)
	{
		$(containerId).trigger("click");
	}
}

function loadContentTo(containerId, fileName)
{
	$(containerId).load("/content/"+fileName+".html");
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

function showInfoDialog(type,header,message,btnid)
{
	if (type != 'select')
		$('#modal_select').toggle(false);
	else
		$('#modal_select').toggle(true);
	
	$('#modal_dialog .modal-bodytitle').html(header);
	$('#modal_dialog .modal-bodycontent').html(message);
	
	if (type=="success"){
		$('#modal_dialog .modal-bodyicon').html('<i class="fa fa-check modal-icon-check">');
		$('#modal_dialog .modal-footer-button').html('<button type="button" class="btn btn-success" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="warning"){
		$('#modal_dialog .modal-bodyicon').html('<i class="fa fa-warning modal-icon-warning">');
		$('#modal_dialog .modal-footer-button').html('<button type="button" class="btn btn-warning" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="error"){	
		$('#modal_dialog .modal-bodyicon').html('<i class="fa fa-warning modal-icon-error">');
		$('#modal_dialog .modal-footer-button').html('<button type="button" class="btn btn-danger" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}	
	else if (type == "select"){
		$('#modal_dialog .modal-bodyicon').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#modal_dialog .modal-footer-button').html('<button type="button" id="'+btnid+'" class="btn btn-success" data-dismiss="modal">'+$.i18n('general_btn_save')+'</button>');
		$('#modal_dialog .modal-footer-button').append('<button type="button" class="btn btn-danger" data-dismiss="modal">'+$.i18n('general_btn_abort')+'</button>');
	}
	
	$("#modal_dialog").modal({
		backdrop : "static",
		keyboard: false,
		show: true
	});
}

function isJsonString(str)
{
	try
	{
		JSON.parse(str);
	}
	catch (e)
	{
		return e;
	}
	return "";
}


function createJsonEditor(container,schema,setconfig)
{
	$('#'+container).off();
	$('#'+container).html("");

	var editor = new JSONEditor(document.getElementById(container),
	{
		theme: 'bootstrap3',
		iconlib: "fontawesome4",
		disable_collapse: 'true',
		form_name_root: 'sa',
		disable_edit_json: 'true',
		disable_properties: 'true',
		disable_array_reorder: 'true',
		no_additional_properties: 'true',
		schema: {
			title:'',
			properties: schema
		}
	});

	$('#editor_container .well').css("background-color","white");
	$('#editor_container .well').css("border","none");
	$('#editor_container .well').css("box-shadow","none");
	$('#editor_container .btn').addClass("btn-primary");
	$('#editor_container h3').first().remove();

	if (setconfig)
	{
		for(var key in editor.root.editors)
		{
			editor.getEditor("root."+key).setValue( parsedConfJSON[key] );
		}
	}

	return editor;
}

function createSelGroup(group)
{
	var el = document.createElement('optgroup');
	el.setAttribute('label', group);
	return el;
}
		
function createSelOpt(opt, title)
{
	var el = document.createElement('option');
	el.setAttribute('value', opt);
	if (typeof title == 'undefined')	
		el.innerHTML = opt;
	else
		el.innerHTML = title;
	return el;
}

function createSel(array, group)
{
	if (array.length != "0")
	{
		var el = createSelGroup(group);
		for(var i=0; i<array.length; i++)
		{
			var opt = createSelOpt(array[i])
			el.appendChild(opt);
		}
		return el;
	}
}

function performTranslation()
{
	$('#wrapper').i18n();
}

function encode_utf8(s)
{
	return unescape(encodeURIComponent(s));
}
