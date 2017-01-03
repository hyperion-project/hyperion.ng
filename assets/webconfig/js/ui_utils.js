
function debugMessage(msg)
{
	if (debugMessagesActive)
	{
		console.log(msg);
	}
}

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
		$('#modal_dialog .modal-footer-button').append('<button type="button" class="btn btn-danger" data-dismiss="modal">'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "uilock"){
		$('#modal_dialog .modal-bodyicon').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#modal_dialog .modal-footer-button').html('<b>'+$.i18n('InfoDialog_nowrite_foottext')+'</b>');
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


function createJsonEditor(container,schema,setconfig,usePanel)
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
	
	if(usePanel)
	{
		$('#'+container+' .well').first().removeClass('well well-sm');
		$('#'+container+' h3').remove();
		$('#'+container+' .well').first().removeClass('well well-sm');
	}

	if (setconfig)
	{
		for(var key in editor.root.editors)
		{
			editor.getEditor("root."+key).setValue( parsedConfJSON[key] );
		}
	}

	return editor;
}

function createTableTh(th1, th2){
	var elth1 = document.createElement('th');
	var elth2 = document.createElement('th');
	var tr = document.createElement('tr');

	elth1.innerHTML = th1;
	elth2.innerHTML = th2;
	tr.appendChild(elth1);
	tr.appendChild(elth2);

	return tr;
}
	
function createTableTd(td1, td2){
	var eltd1 = document.createElement('td');
	var eltd2 = document.createElement('td');
	var tr = document.createElement('tr');

	eltd1.innerHTML = td1;
	eltd2.innerHTML = td2;
	tr.appendChild(eltd1);
	tr.appendChild(eltd2);

	return tr;
}
	
function createHelpTable(list, phead){
	var table = document.createElement('table');
	var thead = document.createElement('thead');
	var tbody = document.createElement('tbody');
	
	table.className = 'table table-hover borderless';
	
	thead.appendChild(createTableTh($.i18n('conf_helptable_option'), $.i18n('conf_helptable_expl')));
		for (key in list){
			if(list[key].access != 'system'){
				text = list[key].title.replace('title', 'expl');
				tbody.appendChild(createTableTd($.i18n(list[key].title), $.i18n(text)));
			}
		}
	table.appendChild(thead);
	table.appendChild(tbody);

	return createPanel(phead, table);
}

function createPanel(head, body, footer, type){
	var p = document.createElement('div');
	var phead = document.createElement('div');
	var pbody = document.createElement('div');
	var pfooter = document.createElement('div');
	
	if(typeof type == 'undefined')
		type = 'panel-default';
	
	p.className = 'panel '+type;
	phead.className = 'panel-heading';
	pbody.className = 'panel-body';
	pfooter.className = 'panel-footer';
	
	phead.innerHTML = head;
	
	if(typeof body != 'undefined')
		pbody.appendChild(body);
	
	pfooter.innerHTML = footer;
	
	p.appendChild(phead);
	p.appendChild(pbody);
	
	if(typeof footer != 'undefined')
		p.appendChild(pfooter);
	
	return p;
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
	$('[data-i18n]').i18n();
}

function encode_utf8(s)
{
	return unescape(encodeURIComponent(s));
}
