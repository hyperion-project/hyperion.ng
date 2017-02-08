
$(function() {

    $('#side-menu').metisMenu();

});

var oldWidth;
//Loads the correct sidebar on window load,
//collapses the sidebar on window resize.
// Sets the min-height of #page-wrapper to window size
$(function() {
    $(window).bind("load resize", function() {
        var topOffset = 50;
        var width = (this.window.innerWidth > 0) ? this.window.innerWidth : this.screen.width;
        if(oldWidth != width)
		{
			if (width < 768)
			{
				$('#main-nav').css({"position":"fixed","right":"-235px","top":"45px","width":"230px","border":"1px solid rgba(0, 0, 0, .2)","box-shadow":"0 3px 9px rgba(0, 0, 0, .5)"});
				topOffset = 100; // 2-row-menu
				$('.mnava').on('click', function(){
					$("html, body").animate({ scrollTop: 0 }, "fast");
					$( "#main-nav" ).animate({right: "-235px",}, 300 );
					$(".navbar-toggle").addClass("closed");
				});
			}
			else
				$( "#main-nav" ).removeAttr("style").css({"position":"absolute"});


			var height = ((this.window.innerHeight > 0) ? this.window.innerHeight : this.screen.height) - 1;
			height = height - topOffset;
			if (height < 1) height = 1;
			if (height > topOffset) {
				$("#page-wrapper").css("min-height", (height-11) + "px");
			}
		}
    });

    var url = window.location;
    // var element = $('ul.nav a').filter(function() {
    //     return this.href == url;
    // }).addClass('active').parent().parent().addClass('in').parent();
    var element = $('ul.nav a').filter(function() {
     return this.href == url;
    }).addClass('active').parent();

    while(true){
        if (element.is('li')){
            element = element.parent().addClass('in').parent();
        } else {
            break;
        }
    }
});

$('.navbar-toggle').off().on('click', function(){
	if($('#main-nav').css("right") != "-2px")
	{
		 $('#main-nav').animate({right: "-2px",}, 300 );
		 $(".navbar-toggle").removeClass("closed");
	}
	else
	{
		$('#main-nav').animate({right: "-235px",}, 300 );
		$(".navbar-toggle").addClass("closed");
	}
});