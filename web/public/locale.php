<?php

require "gettext.inc";

// get locale from get, session, or browser.
session_start();
$locale = "en_US";
if (isSet($_GET["hl"])) {
  $locale = $_GET["hl"];
  $_SESSION["hl"] = $locale;
} else if (isSet($_SESSION["hl"]) && ($_SESSION["hl"] != "")) {
  $locale = $_SESSION["hl"];
} else if (isSet($_SERVER["HTTP_ACCEPT_LANGUAGE"])) {
  $locale = Locale::acceptFromHttp($_SERVER["HTTP_ACCEPT_LANGUAGE"]);
}

// get language from locale (we don't really care
// too much about country code, just language).
$localeParts = explode("_", $locale);
$language = $localeParts;

putenv("LANGUAGE=" . $locale);
putenv("LANG=" . $locale);
putenv("LC_ALL=" . $locale);
putenv("LC_MESSAGES=" . $locale);
T_setlocale(LC_ALL, $locale);
T_bindtextdomain("website", "./locale");
T_textdomain("website");

?>
