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

function removeStorage(item, session)
{
	if(storageComp())
	{
		if(session === true)
			sessionStorage.removeItem(item);
		else
			localStorage.removeItem(item);
	}
}

function debugMessage(msg)
{
	if (window.debugMessagesActive)
	{
		console.log(msg);
	}
}

function updateSessions()
{
	var sess = window.serverInfo.sessions;
	if (sess && sess.length)
	{
		window.wSess = [];
		for(var i = 0; i<sess.length; i++)
		{
			if(sess[i].type == "_hyperiond-http._tcp.")
			{
				window.wSess.push(sess[i]);
			}
		}

		if (window.wSess.length > 1)
			$('#btn_instanceswitch').toggle(true);
		else
			$('#btn_instanceswitch').toggle(false);
	}
}

function validateDuration(d)
{
	if(typeof d === "undefined" || d < 0)
		return 0;
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

function loadContent(event, forceRefresh)
{
	var tag;

	var lastSelectedInstance = getStorage('lastSelectedInstance', false);

	if (lastSelectedInstance && (lastSelectedInstance != window.currentHyperionInstance))
		if (typeof(window.serverInfo.instance[lastSelectedInstance].running) !== 'undefined' && window.serverInfo.instance[lastSelectedInstance].running)
			instanceSwitch(lastSelectedInstance);
		else
			removeStorage('lastSelectedInstance', false);

	if(typeof event != "undefined")
	{
		tag = event.currentTarget.hash;
		tag = tag.substr(tag.indexOf("#") + 1);
		setStorage('lasthashtag', tag, true);
	}
	else
		tag = getHashtag();

	if(forceRefresh || prevTag != tag)
	{
		prevTag = tag;
		$("#page-content").off();
		$("#page-content").load("/content/"+tag+".html", function(response,status,xhr){
			if(status == "error")
			{
				$("#page-content").html('<h3>'+$.i18n('info_404')+'</h3>');
				removeOverlay();
			}
			updateUiOnInstance(window.currentHyperionInstance);
		});
	}
}

function getInstanceNameByIndex(index)
{
	var instData = window.serverInfo.instance
	for(var key in instData)
	{
		if(instData[key].instance == index)
			return instData[key].friendly_name;
	}
	return "unknown"
}

function updateHyperionInstanceListing()
{
	var data = window.serverInfo.instance.filter(entry => entry.running);
	$('#hyp_inst_listing').html("");
	for(var key in data)
	{
		var currInstMarker = (data[key].instance == window.currentHyperionInstance) ? "component-on" : "";

		var html = '<li id="hyperioninstance_'+data[key].instance+'"> \
			<a>  \
				<div>  \
					<i class="fa fa-circle fa-fw '+currInstMarker+'"></i> \
					<span>'+data[key].friendly_name+'</span> \
				</div> \
			</a> \
		</li> '

		if(data.length-1 > key)
			html += '<li class="divider"></li>'

		$('#hyp_inst_listing').append(html);

		$('#hyperioninstance_'+data[key].instance).off().on("click",function(e){
			var inst = e.currentTarget.id.split("_")[1]
			instanceSwitch(inst)
		});
	}
}

function initLanguageSelection()
{
	// Initialise language selection list with languages supported
	for (var i = 0; i < availLang.length; i++)
	{
		$("#language-select").append('<option value="'+i+'" selected="">'+availLangText[i]+'</option>');
	}

	var langLocale = storedLang;

	// If no language has been set, resolve browser locale
	if ( langLocale === 'auto' )
	{
		langLocale = $.i18n().locale.substring(0,2);
	}

	// Resolve text for language code
	var langText = 'Please Select';

	//Test, if language is supported by hyperion
	var langIdx = availLang.indexOf(langLocale);
	if ( langIdx > -1 )
	{
		langText = availLangText[langIdx];
	}
	else
	{
		// If language is not supported by hyperion, try fallback language
		langLocale = $.i18n().options.fallbackLocale.substring(0,2);	
		langIdx = availLang.indexOf(langLocale);
		if ( langIdx > -1 )
		{
			langText = availLangText[langIdx];
		}
	}
	//console.log("langLocale: ", langLocale, "langText: ", langText);

	$('#language-select').prop('title', langText);
	$("#language-select").val(langIdx);
	$("#language-select").selectpicker("refresh");
}

function updateUiOnInstance(inst)
{
	if(inst != 0)
	{
		var currentURL = $(location).attr("href");
		if(currentURL.indexOf('#conf_network') != -1 || currentURL.indexOf('#update') != -1 || currentURL.indexOf('#conf_webconfig') != -1 || currentURL.indexOf('#conf_grabber') != -1 || currentURL.indexOf('#conf_logging') != -1)
			$("#hyperion_global_setting_notify").fadeIn("fast");
		else
			$("#hyperion_global_setting_notify").attr("style", "display:none");

		$("#dashboard_active_instance_friendly_name").html($.i18n('dashboard_active_instance') + ': ' + window.serverInfo.instance[inst].friendly_name);
		$("#dashboard_active_instance").removeAttr("style");
	}
	else
	{
		$("#hyperion_global_setting_notify").fadeOut("fast");
		$("#dashboard_active_instance").attr("style", "display:none");
	}
}

function instanceSwitch(inst)
{
	requestInstanceSwitch(inst)
	window.currentHyperionInstance = inst;
	window.currentHyperionInstanceName = getInstanceNameByIndex(inst);
	setStorage('lastSelectedInstance', inst, false)
	updateHyperionInstanceListing()
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
	if (type=="success")
	{
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-check modal-icon-check">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_success_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-success" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="warning")
	{
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_warning_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-warning" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type=="error")
	{
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-error">');
		if(header == "")
			$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_general_error_title')+'</h4>');
		$('#id_footer').html('<button type="button" class="btn btn-danger" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type == "select")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saveandreload')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "iswitch")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" id="id_btn_saveset" class="btn btn-primary" data-dismiss="modal"><i class="fa fa-fw fa-exchange"></i>'+$.i18n('general_btn_iswitch')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "uilock")
	{
		$('#id_body').html('<img src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<b>'+$.i18n('InfoDialog_nowrite_foottext')+'</b>');
	}
	else if (type == "import")
	{
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-warning modal-icon-warning">');
		$('#id_footer').html('<button type="button" id="id_btn_import" class="btn btn-warning" data-dismiss="modal"><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_saverestart')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "delInst")
	{
		$('#id_body').html('<i style="margin-bottom:20px" class="fa fa-remove modal-icon-warning">');
		$('#id_footer').html('<button type="button" id="id_btn_yes" class="btn btn-warning" data-dismiss="modal"><i class="fa fa-fw fa-trash"></i>'+$.i18n('general_btn_yes')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "renInst")
	{
		$('#id_body_rename').html('<i style="margin-bottom:20px" class="fa fa-pencil modal-icon-edit"><br>');
		$('#id_body_rename').append('<h4>'+header+'</h4>');
		$('#id_body_rename').append('<input class="form-control" id="renInst_name" type="text" value="'+message+'">');
		$('#id_footer_rename').html('<button type="button" id="id_btn_ok" class="btn btn-success" data-dismiss-modal="#modal_dialog_rename" disabled><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_ok')+'</button>');
		$('#id_footer_rename').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "changePassword")
	{
		$('#id_body_rename').html('<i style="margin-bottom:20px" class="fa fa-key modal-icon-edit"><br>');
		$('#id_body_rename').append('<h4>'+header+'</h4>');
		$('#id_body_rename').append('<input class="form-control" id="oldPw" placeholder="Old" type="text"> <br />');
		$('#id_body_rename').append('<input class="form-control" id="newPw" placeholder="New" type="text">');
		$('#id_footer_rename').html('<button type="button" id="id_btn_ok" class="btn btn-success" data-dismiss-modal="#modal_dialog_rename" disabled><i class="fa fa-fw fa-save"></i>'+$.i18n('general_btn_ok')+'</button>');
		$('#id_footer_rename').append('<button type="button" class="btn btn-danger" data-dismiss="modal"><i class="fa fa-fw fa-close"></i>'+$.i18n('general_btn_cancel')+'</button>');
	}
	else if (type == "checklist")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+$.i18n('infoDialog_checklist_title')+'</h4>');
		$('#id_body').append(header);
		$('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type == "newToken")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal">'+$.i18n('general_btn_ok')+'</button>');
	}
	else if (type == "grantToken")
	{
		$('#id_body').html('<img style="margin-bottom:20px" src="img/hyperion/hyperionlogo.png" alt="Redefine ambient light!">');
		$('#id_footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal" id="tok_grant_acc">'+$.i18n('general_btn_grantAccess')+'</button>');
		$('#id_footer').append('<button type="button" class="btn btn-danger" data-dismiss="modal" id="tok_deny_acc">'+$.i18n('general_btn_denyAccess')+'</button>');
	}

	if(type != "renInst")
	{
		$('#id_body').append('<h4 style="font-weight:bold;text-transform:uppercase;">'+header+'</h4>');
		$('#id_body').append(message);
	}

	if(type == "select" || type == "iswitch")
		$('#id_body').append('<select id="id_select" class="form-control" style="margin-top:10px;width:auto;"></select>');


	$(type == "renInst" || type == "changePassword" ? "#modal_dialog_rename" : "#modal_dialog").modal({
		backdrop : "static",
		keyboard: false,
		show: true
	});

	$(document).on('click', '[data-dismiss-modal]', function () {
		var target = $(this).attr('data-dismiss-modal');
		$(target).modal('hide');
	});
}

function createHintH(type, text, container)
{
	type = String(type);
	if(type == "intro")
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

function createEffHint(title, text)
{
	return '<div class="bs-callout bs-callout-primary" style="margin-top:0px"><h4>'+title+'</h4>'+text+'</div>';
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
		// inject fileName property
		reader.fileName = input.files[0].name

        reader.onload = function (e) {
			cb(e.target.result, e.target.fileName);
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

function createJsonEditor(container,schema,setconfig,usePanel,arrayre)
{
	$('#'+container).off();
	$('#'+container).html("");

	if (typeof arrayre === 'undefined')
		arrayre = true;

	var editor = new JSONEditor(document.getElementById(container),
	{
		theme: 'bootstrap3',
		iconlib: "fontawesome4",
		disable_collapse: 'true',
		form_name_root: 'sa',
		disable_edit_json: true,
		disable_properties: true,
		disable_array_reorder: arrayre,
		no_additional_properties: true,
		disable_array_delete_all_rows: true,
		disable_array_delete_last_row: true,
		access: storedAccess,
		schema: {
			title:'',
			properties: schema
		}
	});

	if(usePanel)
	{
		$('#'+container+' .well').first().removeClass('well well-sm');
		$('#'+container+' h4').first().remove();
		$('#'+container+' .well').first().removeClass('well well-sm');
	}

	if (setconfig)
	{
		for(var key in editor.root.editors)
		{
			editor.getEditor("root."+key).setValue(Object.assign({}, editor.getEditor("root."+key).value, window.serverConfig[key] ));
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

/*
	Show a notification
	@param type     Valid types are "info","success","warning","danger"
	@param message  The message to show
	@param title     A title (optional)
	@param addhtml   Add custom html to the notification end
 */
function showNotification(type, message, title="", addhtml="")
{
	if(title == "")
	{
		switch(type)
		{
			case "info":
			title = $.i18n('infoDialog_general_info_title');
			break;
			case "success":
			title = $.i18n('infoDialog_general_success_title');
			break;
			case "warning":
			title = $.i18n('infoDialog_general_warning_title');
			break;
			case "danger":
			title = $.i18n('infoDialog_general_error_title');
			break;
		}
	}

	$.notify({
		// options
		title: title,
		message: message
	},{
		// settings
		type: type,
		animate: {
			enter: 'animated fadeInDown',
			exit: 'animated fadeOutUp'
		},
		placement:{
			align:'center'
		},
		mouse_over : 'pause',
		template: '<div data-notify="container" class="bg-w col-md-6 bs-callout bs-callout-{0}" role="alert">' +
		'<button type="button" aria-hidden="true" class="close" data-notify="dismiss">×</button>' +
		'<span data-notify="icon"></span> ' +
		'<h4 data-notify="title">{1}</h4> ' +
		'<span data-notify="message">{2}</span>' +
		addhtml+
		'<div class="progress" data-notify="progressbar">' +
			'<div class="progress-bar progress-bar-{0}" role="progressbar" aria-valuenow="0" aria-valuemin="0" aria-valuemax="100" style="width: 0%;"></div>' +
		'</div>' +
		'<a href="{3}" target="{4}" data-notify="url"></a>' +
		'</div>'
	});
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
			var rgb = e.color.toRGB();
			var hex = e.color.toHex();
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
	var pfooter = document.createElement('button');
	pfooter.className = "btn btn-primary";
	pfooter.setAttribute("id", footerid);
	pfooter.innerHTML = '<i class="fa fa-fw fa-save"></i>'+$.i18n('general_button_savesettings');

	return createPanel(phead, "", pfooter, "panel-default", bodyid);
}

function sortProperties(list)
{
	for(var key in list)
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

	for (var key in list)
	{
		if(list[key].access != 'system')
		{
			// break one iteration (in the loop), if the schema has the entry hidden=true
			if ("options" in list[key] && "hidden" in list[key].options && (list[key].options.hidden))
				continue;
			if ("access" in list[key] && ((list[key].access == "advanced" && storedAccess == "default") || (list[key].access == "expert" && storedAccess != "expert")))
				continue;
			var text = list[key].title.replace('title', 'expl');
			tbody.appendChild(createTableRow([$.i18n(list[key].title), $.i18n(text)], false, false));

			if(list[key].items && list[key].items.properties)
			{
				var ilist = sortProperties(list[key].items.properties);
				for (var ikey in ilist)
				{
					// break one iteration (in the loop), if the schema has the entry hidden=true
					if ("options" in ilist[ikey] && "hidden" in ilist[ikey].options && (ilist[ikey].options.hidden))
						continue;
					if ("access" in ilist[ikey] && ((ilist[ikey].access == "advanced" && storedAccess == "default") || (ilist[ikey].access == "expert" && storedAccess != "expert")))
						continue;
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
		pbody.setAttribute("id", bodyid);
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

function createSel(array, group, split)
{
	if (array.length != 0)
	{
		var el = createSelGroup(group);
		for(var i=0; i<array.length; i++)
		{
			var opt;
			if(split)
			{
				opt = array[i].split(":")
				opt = createSelOpt(opt[0],opt[1])
			}
			else
				opt = createSelOpt(array[i])
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

function getReleases(callback)
{
	$.ajax({
	    url: window.gitHubReleaseApiUrl,
	    method: 'get',
	    error: function(XMLHttpRequest, textStatus, errorThrown)
			{
					callback(false);
	    },
	    success: function(releases)
			{
				window.gitHubVersionList = releases;
				var highestRelease = {
					tag_name: '0.0.0'
				};
				var highestAlphaRelease = {
					tag_name: '0.0.0'
				};
				var highestBetaRelease = {
					tag_name: '0.0.0'
				};
				var highestRcRelease = {
					tag_name: '0.0.0'
				};

				for(var i in releases) {

					//drafts will be ignored
					if(releases[i].draft)
						continue;

					if(releases[i].tag_name.includes('alpha'))
					{
						if (sem = semverLite.gt(releases[i].tag_name, highestAlphaRelease.tag_name))
							highestAlphaRelease = releases[i];
					}
					else if (releases[i].tag_name.includes('beta'))
					{
						if (sem = semverLite.gt(releases[i].tag_name, highestBetaRelease.tag_name))
							highestBetaRelease = releases[i];
					}
					else if (releases[i].tag_name.includes('rc'))
					{
						if (semverLite.gt(releases[i].tag_name, highestRcRelease.tag_name))
							highestRcRelease = releases[i];
					}
					else
					{
						if (semverLite.gt(releases[i].tag_name, highestRelease.tag_name))
							highestRelease = releases[i];
					}
				}
				window.latestStableVersion = highestRelease;
				window.latestBetaVersion = highestBetaRelease;
				window.latestAlphaVersion= highestAlphaRelease;
				window.latestRcVersion = highestRcRelease;


				if(window.serverConfig.general.watchedVersionBranch == "Beta" && semverLite.gt(highestBetaRelease.tag_name, highestRelease.tag_name))
					window.latestVersion = highestBetaRelease;
				else
					window.latestVersion = highestRelease;

				if(window.serverConfig.general.watchedVersionBranch == "Alpha" && semverLite.gt(highestAlphaRelease.tag_name, highestBetaRelease.tag_name))
					window.latestVersion = highestAlphaRelease;

				if(window.serverConfig.general.watchedVersionBranch == "Alpha" && semverLite.lt(highestAlphaRelease.tag_name, highestBetaRelease.tag_name))
					window.latestVersion = highestBetaRelease;

				//next two if statements are only necessary if we don't have a beta or stable release. We need one alpha release at least
				if(window.latestVersion.tag_name == '0.0.0' && highestBetaRelease.tag_name != '0.0.0')
					window.latestVersion = highestBetaRelease;

				if(window.latestVersion.tag_name == '0.0.0' && highestAlphaRelease.tag_name != '0.0.0')
					window.latestVersion = highestAlphaRelease;

				callback(true);

			}
	});
}

function handleDarkMode()
{
		$("<link/>", {
			rel: "stylesheet",
			type: "text/css",
			href: "../css/darkMode.css"
		}).appendTo("head");

		setStorage("darkMode", "on", false);
		$('#btn_darkmode_icon').removeClass('fa fa-moon-o');
		$('#btn_darkmode_icon').addClass('fa fa-sun-o');
}
