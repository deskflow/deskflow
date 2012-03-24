<?php

require "gettext.inc";

$url = "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"];
session_start();

function getLang($lang) {
  return reset(explode("_", $lang));
}

$lang = "en";
if (isSet($_GET["ul"])) {

  // language forced by url.
  $lang = $_GET["ul"];
  unset($_SESSION["lang"]);
  
} else if (isSet($_GET["hl"])) {

  // language forced by visitor.
  $lang = $_GET["hl"];
  
  // if english, force and use / (if we don't
  // force in session, language is auto detected).
  if (strstr($lang, "en") != "") {
    $_SESSION["lang"] = $lang;
    header("Location: /");
  } else {
    // no need to force in session, as it is
    // forced in url.
    header("Location: /" . $lang . "/");
  }
  exit;
  
} else if (isSet($_SESSION["lang"])) {

  // language forced. this should only happen under /
  // where no url lang is forced.
  $lang = $_SESSION["lang"];
  
} else if (isSet($_SERVER["HTTP_ACCEPT_LANGUAGE"])) {

  // no language specified in url, try to auto-detect.
  $lang = getLang(Locale::acceptFromHttp($_SERVER["HTTP_ACCEPT_LANGUAGE"]));

  // only redirect if non-english, otherwise use /.
  if (strstr($lang, "en") == "") {
    header("Location: /" . $lang . "/");
    exit;
  }
}

putenv("LANGUAGE=" . $lang);
putenv("LANG=" . $lang);
putenv("LC_ALL=" . $lang);
putenv("LC_MESSAGES=" . $lang);
T_setlocale(LC_ALL, $lang);
T_bindtextdomain("website", "./locale");
T_textdomain("website");

?>
