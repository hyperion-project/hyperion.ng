function reload()
{
	location.reload();	
}

function storageComp()
{
	if (typeof(Storage) !== "undefined")
		return true;
	return false;
}

function getStorage(item)
{
	return localStorage.getItem(item);
}

function setStorage(item, value)
{
	localStorage.setItem(item, value);
	return true;
}

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

function showInfoDialog(type,header,message)
{	
	if (type=="success"){
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-check modal-icon-check">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_success_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-success" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="warning"){
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_warning_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-warning" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="error"){	
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-error">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_error_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-danger" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}	
	else if (type == "select"){
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-success" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saveandreload')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "uilock"){
		$('#id_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<b>'+$.i18n('InfoDialog_nowrite_foottext')+'</b>');
	}
	else if (type == "import"){
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
		$('#id_footer').html('<button type="button" id="id_btn_import" class="btn btn-warning" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saverestart')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	
	$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+header+'</h4>');
	$('#id_body').append(message);
	
	if(type == "select")
		$('#id_body').append('<select id="id_select" class="form-control" style="margin-top:10px;width:auto;"></select>');
	
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
		disable_array_delete_all_rows: 'true',
		disable_array_delete_last_row: 'true',
		access: storedAccess,
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

// Creates a table with thead and tbody ids
// @param string hid  : a id for thead
// @param string bid  : a id for tbody
// @param string cont : a container id to html() the table
function createTable(hid, bid, cont)
{
	var table = document.createElement('table');
	var thead = document.createElement('thead');
	var tbody = document.createElement('tbody');
	
	table.className = "table";
	table.style.marginBottom = "0px";
	thead.setAttribute("id", hid);
	tbody.setAttribute("id", bid);
	
	table.appendChild(thead);
	table.appendChild(tbody);
	
	$('#'+cont).html(table);
}

// Creates a table row <tr>
// @param array list :innerHTML content for <td>/<th>
// @param bool head  :if null or false it's body
// @param bool align :if null or false no alignment 
//
// @return : <tr> with <td> or <th> as child(s)
function createTableRow(list, head, align)
{
	var row = document.createElement('tr');
	
	for(var i = 0; i < list.length; i++)
	{
		if(head === true)
			var el = document.createElement('th');
		else
			var el = document.createElement('td');
		
		if(align)
			el.style.verticalAlign = "middle";
		
		el.innerHTML = list[i];
		row.appendChild(el);
	}
	return row;
}

function createRow(id)
{
	var el = document.createElement('div');
	el.className = "row";
	el.setAttribute('id', id);
	return el;
}

function createOptPanel(phicon, phead, bodyid, footerid)
{
	phead = '<i class="fa '+phicon+' fa-fw"></i>'+phead;
	pfooter = document.createElement('button');
	pfooter.className = "btn btn-success";
	pfooter.setAttribute("id", footerid);
	pfooter.innerHTML = '<i class="fa fa-fw fa-save"></i>'+$.i18n('general_button_savesettings');
	
	return createPanel(phead, "", pfooter, "panel-default", bodyid);
}

function createHelpTable(list, phead){
	var table = document.createElement('table');
	var thead = document.createElement('thead');
	var tbody = document.createElement('tbody');
	//console.log(sortProperties(list));
	
	phead = '<i class="fa fa-fw fa-info-circle"></i>'+phead+' '+$.i18n("conf_helptable_expl");
	
	table.className = 'table table-hover borderless';
	
	thead.appendChild(createTableRow([$.i18n('conf_helptable_option'), $.i18n('conf_helptable_expl')], true, false));
		for (key in list)
		{
			if(list[key].access != 'system')
			{
				var text = list[key].title.replace('title', 'expl');
				tbody.appendChild(createTableRow([$.i18n(list[key].title), $.i18n(text)], false, false));
				
				if(list[key].items && list[key].items.properties)
				{
					var ilist = list[key].items.properties;
					for (ikey in ilist)
					{
						
						var itext = ilist[ikey].title.replace('title', 'expl');
						tbody.appendChild(createTableRow([$.i18n(ilist[ikey].title), $.i18n(itext)], false, false));
					}
				}	
			}
		}
	table.appendChild(thead);
	table.appendChild(tbody);

	return createPanel(phead, table);
}

function createPanel(head, body, footer, type, bodyid){
	var cont = document.createElement('div');
	var p = document.createElement('div');
	var phead = document.createElement('div');
	var pbody = document.createElement('div');
	var pfooter = document.createElement('div');
	
	cont.className = "col-lg-6";
	
	if(typeof type == 'undefined')
		type = 'panel-default';
	
	p.className = 'panel '+type;
	phead.className = 'panel-heading';
	pbody.className = 'panel-body';
	pfooter.className = 'panel-footer';
	
	phead.innerHTML = head;
	
	if(typeof bodyid != 'undefined')
	{
		pfooter.style.textAlign = 'right';
		pbody.setAttribute("id", bodyid)
	}
	
	if(typeof body != 'undefined' && body != "")
		pbody.appendChild(body);
	
	if(typeof footer != 'undefined')
		pfooter.appendChild(footer);
	
	p.appendChild(phead);
	p.appendChild(pbody);
	
	if(typeof footer != 'undefined')
	{
		pfooter.style.textAlign = "right";
		p.appendChild(pfooter);
	}
	
	cont.appendChild(p);
	
	return cont;
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
