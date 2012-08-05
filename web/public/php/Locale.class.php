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

// useful itef codes list:
//   http://www.lingoes.net/en/translator/langcode.htm

namespace Synergy;

require "gettext.inc";

class Locale {
  
  var $lang;
  var $rtl;
  var $website;
  
  function __construct($website) {
    
    $this->website = $website;
    
    // default language is english
    $this->lang = "en";
    
    // right to left text languages
    $this->rtl = array("ar", "he");
  }

  function fixItefTag($tag) {
    
    $split = explode("-", $tag);
    if (count($split) == 2) {
      // if language code and country code are the same, then we have
      // a redudntant itef tag (e.g. de-de or fr-fr).
      // of if some browsers send weird language codes, like numbers
      // for the 2nd part, just send the first part in that case.
      if ((strtolower($split[0]) == strtolower($split[1])) ||
          (strlen($split[1]) > 2) || is_numeric($split[1])) {
        return strtolower($split[0]);
      }
    }
    
    // make sure the tag is always lower so that it looks better in the url.
    return strtolower($tag);
  }

  function parseHeaderLocale($header) {
    $first = reset(explode(";", $header));
    $first = reset(explode(",", $first));
    $itef = str_replace("_", "-", $first);
    $lower = strtolower($itef);
    if ($lower != "") {
      return $lower;
    }
    return "en";
  }

  function toGnu($lang) {
    // norway does not confirm to GNU! :|
    if ($lang == "nn" || $lang == "nb")
      return "no";

    // if the language is region specific, use an underscore,
    // and make sure the country code is capitalized.
    if (strstr($lang, "-")) {  
      $split = preg_split("/-/", $lang);
      return $split[0] . "_" . strtoupper($split[1]);
    }
    return $lang;
  }
  
  function redirect($lang) {
    if (isset($_GET["page"]) && ($_GET["page"] != "")) {
      header(sprintf("Location: /%s/%s/", $lang, $_GET["page"]));
    }
    else {
      header(sprintf("Location: /%s/", $lang));
    }
  }
  
  function run() {
    
    if (isSet($_GET["hl"]) && ($_GET["hl"] != "")) {

      // language forced by visitor.
      $this->lang = $_GET["hl"];
      
      // if english, force and use / (if we don't
      // force in session, language is auto detected).
      if (!$this->website->isBot() && strstr($this->lang, "en")) {
        $_SESSION["lang"] = $this->lang;
        header("Location: /");
      } else {
        // no need to force in session, as it is
        // forced in url.
        $this->redirect($this->lang);
      }
      exit;
      
    } else if (isSet($_GET["ul"]) && ($_GET["ul"] != "")) {
      
      // make sure users can't use /en -- should be using / instead.
      if (stristr($_SERVER["REQUEST_URI"], "/en")) {
        header("Location: /");
        exit;
      }
      
      // language forced by url.
      $this->lang = $_GET["ul"];
      if (!$this->website->isBot()) {
        unset($_SESSION["lang"]);
      }
      
      // redirect legacy redundant tags.
      $mapped = $this->fixItefTag($this->lang);
      if ($mapped != $this->lang) {
        $this->redirect($mapped);
        exit;
      }
      
    } else if (!$this->website->isBot() && isSet($_SESSION["lang"])) {

      // language forced. this should only happen under /
      // where no url lang is forced.
      $this->lang = $_SESSION["lang"];
      
    } else if (isSet($_SERVER["HTTP_ACCEPT_LANGUAGE"])) {
      
      // no language specified in url, try to auto-detect.
      $this->lang = $this->parseHeaderLocale($_SERVER["HTTP_ACCEPT_LANGUAGE"]);

      // only redirect if non-english, otherwise use /.
      if (!strstr($this->lang, "en")) {
        $this->redirect($this->fixItefTag($this->lang));
        exit;
      }
    }

    $gnuLang = $this->toGnu($this->lang);
    putenv("LANGUAGE=" . $gnuLang);
    putenv("LANG=" . $gnuLang);
    putenv("LC_ALL=" . $gnuLang);
    putenv("LC_MESSAGES=" . $gnuLang);
    T_setlocale(LC_ALL, $gnuLang);
    T_bindtextdomain("website", "./locale");
    T_textdomain("website");
  }
  
  function getGoogleSearchLang() {
    if ($this->lang == "zh") {
      return "zh-cn";
    }
    return $this->lang;
  }
  
  function getLangDir() {
    if (in_array($this->lang, $this->rtl)) {
      return "rtl";
    }
    return "ltr";
  }
  
  function getCountry() {
    return reset(explode("-", $this->lang));
  }
  
  function getSplashImage() {
    $format = "img/splash/splash-%s.jpg";
    $filenameFull = sprintf($format, $this->lang);
    if (file_exists($filenameFull)) {
      return "/" . $filenameFull;
    }
    
    $filenameShort = sprintf($format, $this->getCountry());
    if (file_exists($filenameShort)) {
      return "/" . $filenameShort;
    }
    
    return "/" . sprintf($format, "en");
  }
}

?>
