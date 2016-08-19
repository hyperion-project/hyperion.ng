<script>

/**
* Enables translation for the form
* with the ID given in "formID"
* Generates token with the given token prefix
* and an underscore followed by the input id
* Example: input id = input_one
* token prefix = tokenprefix
* The translation token would be: "tokenprefix_input_one"
* Default language in "lang" attribute will always be "en"
* @param {String} tokenPrefix
* @param {String} formID
*/
function enableFormTranslation(tokenPrefix, formID) {
var $inputs = $("#" + formID + " :input");

$inputs.each(function() {
  console.log("InputID: " + $(this).attr('id'));
  var oldtext = $("label[for='" + $(this).attr('id') + "']").text();
  $("label[for='" + $(this).attr('id') + "']").html('<span lang="en" data-lang-token="' + tokenPrefix + "_" + $(this).attr('id') + '">' + oldtext + '</span>');
});
}

});

</script>
