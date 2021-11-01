var storedLang;
var availLang = ['cs', 'de', 'en', 'es', 'fr', 'it', 'nl', 'nb', 'pl', 'pt', 'ro', 'sv', 'vi', 'ru', 'tr', 'zh-CN'];
var availLangText = ['Čeština', 'Deutsch', 'English', 'Español', 'Français', 'Italiano', 'Nederlands', 'Norsk Bokmål', 'Polski', 'Português', 'Română', 'Svenska', 'Tiếng Việt', 'русский', 'Türkçe', '汉语'];

//$.i18n.debug = true;

//i18n
function initTrans(lc) {
  $.i18n().load("i18n", lc).done(
    function () {
      $.i18n().locale = lc;
      performTranslation();
    });
}

storedLang = getStorage("langcode");
if (storedLang == null || storedLang === "undefined") {

  var langLocale = $.i18n().locale.substring(0, 2);
  //Test, if language is supported by hyperion
  var langIdx = availLang.indexOf(langLocale);
  if (langIdx === -1) {
    // If language is not supported by hyperion, try fallback language
    langLocale = $.i18n().options.fallbackLocale.substring(0, 2);
    langIdx = availLang.indexOf(langLocale);
    if (langIdx === -1) {
      langLocale = 'en';
    }
  }
  storedLang = langLocale;
  setStorage("langcode", storedLang);
}
initTrans(storedLang);
