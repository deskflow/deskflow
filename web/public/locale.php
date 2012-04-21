<?php

/*
 * synergy-web -- website for synergy
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require "gettext.inc";

$url = "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"];
session_start();

function getLang($lang) {
  
  // HACK: probably need to improve this to handle all
  // locales at some point :/
  if (strtolower($lang) == "zh-tw")
    return $lang;
  
  $lang = reset(explode("_", $lang));
  switch ($lang) {
    case "nb":
    case "nn":
      // hopefully this wont annoy norwegians! :)
      $lang = "no";
      break;
  }
  return $lang;
}

function parseHeaderLocale($header) {
  $first = reset(explode(";", $header));
  return str_replace("_", "-", reset(explode(",", $first)));
}

function toGnu($lang) {
  return str_replace("-", "_", $lang);
}

$lang = "en";
if (isSet($_GET["ul"])) {

  // language forced by url.
  $lang = $_GET["ul"];
  unset($_SESSION["lang"]);
  
} else if (isSet($_GET["hl"])) {

  // language forced by visitor.
  $lang = getLang($_GET["hl"]);
  
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
  $lang = getLang(parseHeaderLocale($_SERVER["HTTP_ACCEPT_LANGUAGE"]));

  // only redirect if non-english, otherwise use /.
  if (strstr($lang, "en") == "") {
    header("Location: /" . $lang . "/");
    exit;
  }
}

$gnuLang = toGnu($lang);
putenv("LANGUAGE=" . $gnuLang);
putenv("LANG=" . $gnuLang);
putenv("LC_ALL=" . $gnuLang);
putenv("LC_MESSAGES=" . $gnuLang);
T_setlocale(LC_ALL, $gnuLang);
T_bindtextdomain("website", "./locale");
T_textdomain("website");

?>
