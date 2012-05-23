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
 
// TODO: convert this file to a class.

require "locale.php";
require "files.php";
require "smarty/libs/Smarty.class.php";

$files = new Synergy\Files();
if ($files->downloadRequested()) {
  $files->download();
  exit;
}

session_start();

$locale = new Synergy\Locale();
$locale->run();

$smarty = new Smarty; // must come first; smarty makes T_ work somehow.
$lang = $locale->lang;
$page = str_replace("/", "", $_GET["page"]);
$title = "Synergy" . (($page != "home") ? (" - " . T_(ucfirst($page))) : "");

$smarty->assign("lang", $lang);
$smarty->assign("baseUrl", stristr($lang, "en") ? "" : "/" . $lang);
$smarty->assign("gsLang", $locale->getGoogleSearchLang());
$smarty->assign("page", $page);
$smarty->assign("title", $title);
$smarty->assign("langIsEnglish", stristr($lang, "en"));
$smarty->assign("langDir", $locale->getLangDir());
$smarty->assign("splashImage", $locale->getSplashImage());

function getFileTitle($file) {
  $parts = explode("-", $file);
  
  // first part is always the version.
  $title = "Synergy " . $parts[1];
  
  // figure out the file extension.
  $end = count($parts) - 1;
  $endParts = explode(".", $parts[$end]);
  $parts[$end] = $endParts[0];
  if (count($endParts) > 1) {
    unset($endParts[0]);
    $ext = strtolower(implode(".", $endParts));
  }
  
  if (count($parts) > 2) {
    
    $p2 = strtolower($parts[2]);
    
    if ($p2 == "source") {
      $title .= " Source Code";
    }
    else if (substr($p2, 0, 6) == "macosx") {
      $title .= " for Mac OS X";
      if (strlen($p2) > 6) {
        $maj = substr($p2, 6, 2);
        $min = substr($p2, 8, 2);
        if ($maj == 10) {
          switch ($min) {
            case 4: $name = "Tiger"; break;
            case 5: $name = "Leopard"; break;
            case 6: $name = "Snow Leopard"; break;
            case 7: $name = "Lion"; break;
            case 8: $name = "Mountain Lion"; break;
          }
        }
        $title .= sprintf(" %d.%d", $maj, $min);
        if (isset($name)) {
          $title .= " " . $name;
        }
      }
    }
    else if ($p2 == "windows") {
      $title .= " for Windows";
      
      if (count($parts) > 2) {
        switch (strtolower($parts[3])) {
          case "x86": $proc = "32-bit"; break;
          case "x64": $proc = "64-bit"; break;
        }
        if (isset($proc)) {
          $title .= sprintf(" (%s)", $proc);
        }
      }
    }
    else if ($p2 == "linux" && isset($ext)) {
      if ($ext == "deb") {
        $title .= " for Ubuntu and Debian";
      }
      else if ($ext == "rpm") {
        $title .= " for Ubuntu and Debian";
      }
      
      if (count($parts) > 2) {
        switch (strtolower($parts[3])) {
          case "i686": $proc = "32-bit"; break;
          case "x86_64": $proc = "64-bit"; break;
        }
        if (isset($proc)) {
          $title .= sprintf(" (%s)", $proc);
        }
      }
    }
  }
  return $title;
}

if ($page == "download") {
  
  if (isset($_GET["file"])) {
    $page = "download_file";
    $file = $_GET["file"];
    $format = "http://synergy.googlecode.com/files/%s";
    $smarty->assign("file", $file);
    $smarty->assign("title", getFileTitle($file));
    $smarty->assign("link", sprintf($format, $file));
  }
  else {
    $smarty->assign("curDate", date("M j, Y", mktime(0, 0, 0, 04, 12, 2012)));
    $smarty->assign("cur14", "1.4.8");
    $smarty->assign("cur14State", T_("Beta"));
    $smarty->assign("cur13", "1.3.8");
    $smarty->assign("cur13State", T_("Stable"));
    
    // new naming: only mac 10.4 is universal
    $smarty->assign("ver14b", array("1.4.7", "1.4.6", "1.4.5"));
    
    // old naming: mac releases are universal
    $smarty->assign("ver14a", array("1.4.4", "1.4.3", "1.4.2"));
  }
}

$content = $smarty->fetch($page . ".tpl");
$smarty->assign("content", $content);
$smarty->display("layout.tpl");

?>
