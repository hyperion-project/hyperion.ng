	JSONEditor.defaults.editors.colorPicker = JSONEditor.defaults.editors.string.extend({
		
		getValue: function() {
			color = $(this.input).data('colorpicker').color.toRGB();
			return [color.r, color.g, color.b];
        },

        setValue: function(val) {
			function rgb2hex(rgb){
				return "#" +
				("0" + parseInt(rgb[0],10).toString(16)).slice(-2) +
				("0" + parseInt(rgb[1],10).toString(16)).slice(-2) +
				("0" + parseInt(rgb[2],10).toString(16)).slice(-2);
			}

			$(this.input).colorpicker('setValue', rgb2hex(val));
        },
		
        build: function() {
			this._super();
			var myinput = this
				$(this.input).colorpicker({
                format: 'rgb',
                customClass: 'colorpicker-2x',
                sliders: {
                    saturation: {
                        maxLeft: 200,
                        maxTop: 200
                    },
                    hue: {
                        maxTop: 200
                    },
                },
               
            })
			//$(this.input).colorpicker().on('changeColor', function(e) {
			//$(this.input).trigger("change");
			//});
			
        }
    });

    JSONEditor.defaults.resolvers.unshift(function(schema) {
        if(schema.type === "array" && schema.format === "colorpicker") {
            return "colorPicker";
        }

    });

$(hyperion).one("cmd-config-getschema", function(event) {
	effects = parsedConfSchemaJSON.properties.effectSchemas.internal
	EffectsHtml = "";
	for(var idx=0; idx<effects.length; idx++)
		{
			EffectsHtml += '<option value="'+effects[idx].schemaContent.script+'">'+effects[idx].schemaContent.title+'</option>';
		}
		$("#effectslist").html(EffectsHtml);
		$("#effectslist").trigger("change");
	});
	
	function validateEditor() {
		if(effects_editor.validate().length)
		{
			showInfoDialog('error','INVALID VALUES','Please check for red marked inputs and try again.');
			return false;
		}
		else
		{
			return true;
		}
	};
	
	function validateName() {
		effectName = $('#name-input').val();
		if (effectName == "")
		{
			showInfoDialog('error','INVALID NAME FIELD','Effect name is empty! Please fill in a name and try again.');
			return false;
		}
		else
		{
			return true;
		}
	};
	
	function triggerTestEffect() {
		var args = effects_editor.getEditor('root.args');
		requestTestEffect(effectName, ":/effects/" + effectPy.slice(1), JSON.stringify(args.getValue()));
	};
	
	effectName = "";
	effects_editor = null;	
	effectPy = "";
	
	$("#effectslist").off().on("change", function(event) {
		for(var idx=0; idx<effects.length; idx++){
			if (effects[idx].schemaContent.script == this.value){
				effects_editor = createJsonEditor('editor_container', {
				args : effects[idx].schemaContent,
				},false);
			effectPy = ':';
			effectPy += effects[idx].schemaContent.script;
			}
		}
		effects_editor.on('change',function() {
			if ($("#btn_cont_test").hasClass("btn-success") && validateName() && validateEditor())
			{
				triggerTestEffect();
			}
		});
	});
	
	$('#btn_write').off().on('click',function() {
		if(validateEditor() && validateName())
		{
			requestWriteEffect(effectName,effectPy,JSON.stringify(effects_editor.getValue()));
			showInfoDialog('success','SUCCESS!','Your effect has been created successfully!');
		}
	});

	$('#btn_start_test').off().on('click',function() {
		if(validateEditor() && validateName())
		{
			triggerTestEffect();
		}
	});
	
	$('#btn_stop_test').off().on('click',function() {
		requestPriorityClear();
	});
	
	$('#btn_cont_test').off().on('click',function() {
		toggleClass('#btn_cont_test', "btn-success", "btn-danger");
	});
	
// Delete effect
	delList=parsedServerInfoJSON.info.effects
	var EffectHtml
	
	for(var idx=0; idx<delList.length; idx++)
		{
			if(!/:/.test(delList[idx].file))
			{
				EffectHtml += '<option value="'+delList[idx].name+'">'+delList[idx].name+'</option>';
			}
		}
		$("#effectsdellist").html(EffectHtml);
	
	$('#btn_delete').off().on('click',function() {
		var name = $("#effectsdellist").val();
		requestDeleteEffect(name);
	});
	
$(document).ready( function() {
	requestServerConfigSchema();
});
