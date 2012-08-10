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

namespace Synergy;

require "Locale.class.php";
require "Files.class.php";
require "ContactForm.class.php";
require "Premium.class.php";

class Website {

  public function __construct() {
    $this->settings = parse_ini_file("settings.ini", true);
    $this->session = new SessionManager($this->settings);
    $this->premium = new Premium($this->settings, $this->session);
  }

  public function run() {

    $currentVersion = "1.4.10";
    $currentDate = mktime(0, 0, 0, 07, 30, 2012);

    // new naming: only mac 10.4 is universal
    $ver14b = array("1.4.9", "1.4.8", "1.4.7", "1.4.6", "1.4.5");

    // old naming: mac releases are universal
    $ver14a = array("1.4.4", "1.4.3", "1.4.2");

    $page = "home";
    if (isset($_GET["page"]) && $_GET["page"] != "") {
      $page = str_replace("/", "", $_GET["page"]);
    }

    $files = new Files();
    if ($files->downloadRequested($page)) {
      $files->download();
      exit;
    }

    if (!$this->isBot()) {
      $this->session->start();
    }

    $locale = new Locale($this);
    $locale->run();

    $smarty = new \Smarty; // must come first; smarty makes T_ work somehow.
    $lang = $locale->lang;
    $title = "Synergy" . (($page != "home") ? (" - " . T_(ucfirst($page))) : "");
    
    $smarty->assign("lang", $lang);
    $smarty->assign("baseUrl", stristr($lang, "en") ? "" : "/" . $lang);
    $smarty->assign("gsLang", $locale->getGoogleSearchLang());
    $smarty->assign("page", $page);
    $smarty->assign("title", $title);
    $smarty->assign("langIsEnglish", stristr($lang, "en"));
    $smarty->assign("langDir", $locale->getLangDir());
    $smarty->assign("splashImage", $locale->getSplashImage());
    $smarty->assign("isPremium", $this->premium->isUserPremium());

    if ($page == "download") {
      
      if (isset($_GET["donate"]) && $isPremium) {
        header("Location: ../premium/");
        exit;
      }
      
      if (isset($_GET["file"])) {
        $page = "download_file";
        $file = $_GET["file"];
        $format = "http://synergy.googlecode.com/files/%s";
        $smarty->assign("file", $file);
        $smarty->assign("isOld", strpos($file, $currentVersion) === false);
        $smarty->assign("title", $this->getFileTitle($file));
        $smarty->assign("link", sprintf($format, $file));
      }
      elseif (isset($_GET["alt"])) {        
        $page = "download_alt";
        $smarty->assign("title", T_("Alternate Downloads"));
        $smarty->assign("ver14b", $ver14b);
        $smarty->assign("ver14a", $ver14a);
      }
      elseif (isset($_GET["list"]) || $this->premium->isUserPremium()) {
        $page = "download_list";
        $smarty->assign("curDate", date("M j, Y", $currentDate));
        $smarty->assign("cur14", $currentVersion);
        $smarty->assign("cur14State", T_("Beta"));
      }
      else {
        if (isset($_GET["register"])) {
          $premium = new Premium($this->settings, $this->session);
          exit($premium->register());
        }
        $page = "download_premium";
        $smarty->assign("showDonateMessage", isset($_GET["donate"]));
      }
    }
    
    if ($page == "premium") {
      $this->premium->run($smarty);
    }

    $custom = "";
    if (is_file("custom/" . $page . ".html")) {
      $custom = $smarty->fetch("custom/" . $page . ".html");
    }
    $smarty->assign("custom", $custom);

    $content = $smarty->fetch($page . ".html");
    $smarty->assign("content", $content);
    $smarty->display("layout.html");
  }

  public function isBot() {
    return in_array("HTTP_USER_AGENT", $_SERVER) &&
      preg_match("/(bot|spider)/", $_SERVER["HTTP_USER_AGENT"]) != 0;
  }

  private function getFileTitle($file) {
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
          $title .= " for Fedora and Red Hat";
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
}
 
?>