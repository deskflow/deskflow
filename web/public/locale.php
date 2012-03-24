<?php

require "gettext.inc";

$url = "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"];
session_start();

$locale = "en";
if (isSet($_GET["ul"])) {

  // language forced by url.
  $locale = $_GET["ul"];
  unset($_SESSION["lang"]);
  
} else if (isSet($_GET["hl"])) {

  // language forced by visitor.
  $locale = $_GET["hl"];
  
  // if english, force and use / (if we don't
  // force in session, language is auto detected).
  if (strstr($locale, "en") != "") {
    $_SESSION["lang"] = $locale;
    header("Location: /");
  } else {
    // no need to force in session, as it is
    // forced in url.
    header("Location: /" . $locale . "/");
  }
  exit;
  
} else if (isSet($_SESSION["lang"])) {

  // language forced. this should only happen under /
  // where no url lang is forced.
  $locale = $_SESSION["lang"];
  
} else if (isSet($_SERVER["HTTP_ACCEPT_LANGUAGE"])) {

  // no language specified, try to auto-detect.
  $locale = Locale::acceptFromHttp($_SERVER["HTTP_ACCEPT_LANGUAGE"]);
  header("Location: /" . $locale . "/");
  exit;
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
