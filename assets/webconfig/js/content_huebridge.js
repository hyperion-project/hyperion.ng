$(document).ready( function() {

	if($("#ip").val() != '' && $("#user").val() != '') {
		get_hue_lights();
	}

	$("#create_user").on("click", function() {
    var connectionRetries = 15;
		var data = {"devicetype":"hyperion#"+Date.now()};
		var UserInterval = setInterval(function(){
		$.ajax({
			type: "POST",
			url: 'http://'+$("#ip").val()+'/api',
			processData: false,
			timeout: 1000,
			contentType: 'application/json',
			data: JSON.stringify(data),
			success: function(r) {
					connectionRetries--;
					$("#connectionTime").html(connectionRetries);
					if(connectionRetries == 0) {
						abortConnection(UserInterval);
          }
					else
					{
						$("#abortConnection").hide();
						$('#pairmodal').modal('show');
						$("#ip_alert").hide();
						if (typeof r[0].error != 'undefined') {
							console.log("link not pressed");
						}
					  if (typeof r[0].success != 'undefined') {
							$('#pairmodal').modal('hide');
							$('#user').val(r[0].success.username);
							get_hue_lights();
							clearInterval(UserInterval);
					  }
					}
			},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
				$("#ip_alert").show();
				clearInterval(UserInterval);
			 }
		});
  },1000);
});

function abortConnection(UserInterval){
	clearInterval(UserInterval);
	$("#abortConnection").show();
	$('#pairmodal').modal('hide');
}

	function get_hue_lights(){
		$.ajax({
			type: "GET",
			url: 'http://'+$("#ip").val()+'/api/'+$("#user").val()+'/lights',
			processData: false,
			contentType: 'application/json',
			success: function(r) {
				for(var lightid in r){
					//console.log(r[lightid].name);
					$('#hue_lights').append('ID: '+lightid+' Name: '+r[lightid].name+'<br />');
				}
			}
		});
	}
});

