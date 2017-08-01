var prevTag;

function removeOverlay()
{
	$("#loading_overlay").removeClass("overlay");
}

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

function getStorage(item, session)
{
	if(storageComp())
	{
		if(session === true)
			return sessionStorage.getItem(item);
		else
			return localStorage.getItem(item);
	}
	return null;
}

function setStorage(item, value, session)
{
	if(storageComp())
	{
		if(session === true)
			sessionStorage.setItem(item, value);
		else
			localStorage.setItem(item, value);
	}
}

function debugMessage(msg)
{
	if (debugMessagesActive)
	{
		console.log(msg);
	}
}

function validateDuration(d)
{
	if(typeof d === "undefined" || d < 0)
		return d = 0;
	else
		return d *= 1000;
}

function getHashtag()
{
	if(getStorage('lasthashtag', true) != null)
		return getStorage('lasthashtag', true);
	else
	{
		var tag = document.URL;
		tag = tag.substr(tag.indexOf("#") + 1);
		if(tag == "" || typeof tag === "undefined" || tag.startsWith("http"))
			tag = "dashboard"
		return tag;
	}
}

function loadContent(event)
{
	var tag;
	
	if(typeof event != "undefined")
	{	
		tag = event.currentTarget.hash;
		tag = tag.substr(tag.indexOf("#") + 1);
		setStorage('lasthashtag', tag, true);
	}
	else
		tag = getHashtag();

	if(prevTag != tag)
	{
		prevTag = tag;
		$("#page-content").off();
		$("#page-content").load("/content/"+tag+".html", function(response,status,xhr){
			if(status == "error")
				$("#page-content").html('<h3>'+$.i18n('info_404')+'</h3>');
				removeOverlay();
		});
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
		$('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saveandreload')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "iswitch"){
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-exchange"></i>'+$.i18n('general_btn_iswitch')+'</button>');
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
	else if (type == "checklist")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_checklist_title')+'</h4>');
		$('#id_body').append(message);
		$('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	
	$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+header+'</h4>');
	$('#id_body').append(message);
	
	if(type == "select" || type == "iswitch")
		$('#id_body').append('<select id="id_select" class="form-control" style="margin-top:10px;width:auto;"></select>');
	
	$("#modal_dialog").modal({
		backdrop : "static",
		keyboard: false,
		show: true
	});
}

function createHintH(type, text, container)
{
	if(type = "intro")
		tclass = "introd";
		
	$('#'+container).prepend('<div class="'+tclass+'"><h4 style="font-size:16px">'+text+'</h4><hr/></div>');
}

function createHint(type, text, container, buttonid, buttontxt)
{
	var fe, tclass;
	
	if(type == "intro")
	{
		fe = '';
		tclass = "intro-hint";
	}
	else if(type == "info")
	{
		fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-info"></i></div><div style="text-align:center;font-size:13px">Information</div>';
		tclass = "info-hint";
	}
	else if(type == "wizard")
	{	
		fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-magic"></i></div><div style="text-align:center;font-size:13px">Information</div>';
		tclass = "wizard-hint";
	}
	else if(type == "warning")
	{	
		fe = '<div style="font-size:25px;text-align:center"><i class="fa fa-info"></i></div><div style="text-align:center;font-size:13px">Information</div>';
		tclass = "warning-hint";
	}
	
	if(buttonid)
		buttonid = '<p><button id="'+buttonid+'" class="btn btn-wizard" style="margin-top:15px;">'+text+'</button></p>';
	else
		buttonid = "";
	
	if(type == "intro")
		$('#'+container).prepend('<div class="bs-callout bs-callout-primary" style="margin-top:0px"><h4>'+$.i18n("conf_helptable_expl")+'</h4>'+text+'</div>');
	else if(type == "wizard")
		$('#'+container).prepend('<div class="bs-callout bs-callout-wizard" style="margin-top:0px"><h4>'+$.i18n("wiz_wizavail")+'</h4>'+$.i18n('wiz_guideyou',text)+buttonid+'</div>');
	else
	{
		createTable('','htb',container, true, tclass);
		$('#'+container+' .htb').append(createTableRow([fe ,text],false,true));
	}
}

function valValue(id,value,min,max)
{
	if(typeof max === 'undefined' || max == "")
		max = 999999;
	
	if(Number(value) > Number(max))	
	{
		$('#'+id).val(max);
		showInfoDialog("warning","",$.i18n('edt_msg_error_maximum_incl',max));
		return max;
	}
	else if(Number(value) < Number(min))
	{
		$('#'+id).val(min);
		showInfoDialog("warning","",$.i18n('edt_msg_error_minimum_incl',min));
		return min;
	}
	return value;		
}

function readImg(input,cb)
{
    if (input.files && input.files[0]) {
        var reader = new FileReader();

        reader.onload = function (e) {
			var i = new Image();
			i.src = e.target.result;
			cb(i.src,i.width,i.height);
        }
        reader.readAsDataURL(input.files[0]);
    }
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
		$('#'+container+' h4').remove();
		$('#'+container+' .well').first().removeClass('well well-sm');
	}

	if (setconfig)
	{
		for(var key in editor.root.editors)
		{
			editor.getEditor("root."+key).setValue( serverConfig[key] );
		}
	}

	return editor;
}

function buildWL(link,linkt,cl)
{	
	var baseLink = "https://docs.hyperion-project.org/";
	var lang;
	
	if(typeof linkt == "undefined")
		linkt = "Placeholder";
	
	if(storedLang == "de" || navigator.locale == "de")
		lang = "de";
	else
		lang = "en";
	
	if(cl === true)
	{
		linkt = $.i18n(linkt);
		return '<div class="bs-callout bs-callout-primary"><h4>'+linkt+'</h4>'+$.i18n('general_wiki_moreto',linkt)+': <a href="'+baseLink+lang+'/'+link+'" target="_blank">'+linkt+'<a></div>'
	}
	else
		return ': <a href="'+baseLink+lang+'/'+link+'" target="_blank">'+linkt+'<a>';
}

function rgbToHex(rgb)
{
	if(rgb.length == 3)
	{
		return "#" +
		("0" + parseInt(rgb[0],10).toString(16)).slice(-2) +
		("0" + parseInt(rgb[1],10).toString(16)).slice(-2) +
		("0" + parseInt(rgb[2],10).toString(16)).slice(-2);    
	}
	else
		debugMessage('rgbToHex: Given rgb is no array or has wrong length');
}

function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}


function createCP(id, color, cb)
{
	if(Array.isArray(color))
		color = rgbToHex(color);
	else if(color == "undefined")
		color = "#AA3399";
	
	if(color.startsWith("#"))
	{
		$('#'+id).colorpicker({
			format: 'rgb',
			customClass: 'colorpicker-2x',
			color: color,
			sliders: {
				saturation: {
					maxLeft: 200,
					maxTop: 200
				},
				hue: {
					maxTop: 200
				},
			}
		});
		$('#'+id).colorpicker().on('changeColor', function(e) {
			rgb = e.color.toRGB();
			hex = e.color.toHex();
			cb(rgb,hex,e);
		});
	}
	else
		debugMessage('createCP: Given color is not legit');
}

// Creates a table with thead and tbody ids
// @param string hid  : a class for thead
// @param string bid  : a class for tbody
// @param string cont : a container id to html() the table
// @param string bless: if true the table is borderless
function createTable(hid, bid, cont, bless, tclass)
{
	var table = document.createElement('table');
	var thead = document.createElement('thead');
	var tbody = document.createElement('tbody');
	
	table.className = "table";
	if(bless === true)
		table.className += " borderless";
	if(typeof tclass !== "undefined")
		table.className += " "+tclass;
	table.style.marginBottom = "0px";
	if(hid != "")
		thead.className = hid;
	tbody.className = bid;
	if(hid != "")
		table.appendChild(thead);
	table.appendChild(tbody);
	
	$('#'+cont).append(table);
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
	pfooter.className = "btn btn-primary";
	pfooter.setAttribute("id", footerid);
	pfooter.innerHTML = '<i class="fa fa-fw fa-save"></i>'+$.i18n('general_button_savesettings');
	
	return createPanel(phead, "", pfooter, "panel-default", bodyid);
}

function sortProperties(list)
{
	for(key in list)
	{
		list[key].key = key;
	}
	list = $.map(list, function(value, index) {
				return [value];
		});
	return list.sort(function(a,b) {
				return a.propertyOrder - b.propertyOrder;
		});
}

function createHelpTable(list, phead){
	var table = document.createElement('table');
	var thead = document.createElement('thead');
	var tbody = document.createElement('tbody');
	list = sortProperties(list);
	
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
				var ilist = sortProperties(list[key].items.properties);
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
