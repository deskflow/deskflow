<?php

require "gettext.inc";

// get locale from get, session, or browser.
session_start();
$locale = "en_US";
if (isSet($_GET["hl"])) {
  $locale = $_GET["hl"];
  $_SESSION["hl"] = $locale;
} else if (isSet($_SESSION["hl"])) {
  $locale = $_SESSION["hl"];
} else if (isSet($_SERVER["HTTP_ACCEPT_LANGUAGE"])) {
  $locale = Locale::acceptFromHttp($_SERVER["HTTP_ACCEPT_LANGUAGE"]);
}

// add country specific codes if none.
switch ($locale) {
  case "fr": $locale = "fr_FR"; break;
  case "ja": $locale = "ja_JP"; break;
  case "de": $locale = "de_DE"; break;
  case "ru": $locale = "ru_RU"; break;
  case "zh": $locale = "zh_CN"; break;
  case "ko": $locale = "ko_KR"; break;
}

putenv("LANGUAGE=".$locale);
putenv("LANG=".$locale);
putenv("LC_ALL=".$locale);
putenv("LC_MESSAGES=".$locale);
T_setlocale(LC_ALL, $locale);
T_bindtextdomain("messages", "./locale");
T_textdomain("messages");

?>
