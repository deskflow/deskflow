<?php

/*
 * synergy-web -- website for synergy
 * Copyright (C) 2013 Bolton Software Ltd.
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

namespace Synergy\Controllers;

require_once "Controller.php";

use Exception;
 
class DownloadController extends Controller {

  public function __construct($smarty, $website) {
    parent::__construct($smarty);
    $this->website = $website;
    $this->premium = $website->premium;
  }
  
  public function run($path) {
    
    if (isset($_GET["donate"]) && $this->premium->isUserPremium()) {
      header("Location: ../premium/");
      exit;
    }
    
    $currentVersion = "1.4.12";
    $currentDate = mktime(0, 0, 0, 05, 04, 2013);

    // naming c: mac universal dropped
    $ver14c = array("1.4.11", "1.4.10", "1.4.9");

    // naming b: only mac 10.4 is universal
    $ver14b = array("1.4.8", "1.4.7", "1.4.6", "1.4.5");

    // naming a: mac releases are universal
    $ver14a = array("1.4.4", "1.4.3", "1.4.2");
    
    $smarty = $this->website->smarty;
    
    if (isset($_GET["file"])) {
      $view = "download/file";
      $file = $_GET["file"];
      $format = "http://synergy.googlecode.com/files/%s";
      $smarty->assign("file", $file);
      $smarty->assign("isOld", strpos($file, $currentVersion) === false);
      $smarty->assign("title", $this->getFileTitle($file));
      $smarty->assign("link", sprintf($format, $file));
      $smarty->assign("social", $this->getSocialNetwork());
    }
    elseif (isset($_GET["alt"])) {        
      $view = "download/alt";
      $smarty->assign("title", T_("Alternate Downloads"));
      $smarty->assign("ver14c", $ver14c);
      $smarty->assign("ver14b", $ver14b);
      $smarty->assign("ver14a", $ver14a);
    }
    elseif (isset($_GET["list"]) || $this->premium->isUserPremium()) {
      $view = "download/list";
      $smarty->assign("curDate", date("M j, Y", $currentDate));
      $smarty->assign("cur14", $currentVersion);
      $smarty->assign("cur14State", T_("Beta"));
    }
    else {
      $formUrl = "../premium/register/";
      if ($this->premium->isLoggedIn()) {
        $formUrl = "../premium/payment/";
      }
      
      $view = "download/premium";
      $smarty->assign("formUrl", $formUrl);
      $smarty->assign("showDonateMessage", isset($_GET["donate"]));
    }
    
    $this->showView($view);
  }
  
  private function getSocialNetwork() {
    $country = substr($this->website->locale->lang, 0, 2);
    if ($country == "zh") {
      return "renren";
    }
    else {
      return "facebook";
    }
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
 