<?php

/*
 * synergy-web -- website for synergy
 * Copyright (C) 2012 Bolton Software Ltd.
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
require "Payment.class.php";

class Website {

  public function __construct() {
    $this->settings = parse_ini_file("settings.ini", true);
    $this->session = new SessionManager($this->settings);
    $this->premium = new Premium($this->settings, $this->session);
  }
  
  public function run() {

    $path = isset($_GET["path"]) ? $_GET["path"] : null;
    $pathParts = preg_split('/\//', $path, null, PREG_SPLIT_NO_EMPTY);
    
    // defaults
    $page = "home";
    $lang = null;
    
    if (count($pathParts) != 0) {
      if (preg_match("/^\w\w(\-\w\w)?$/", $pathParts[0], $matches)) {
        $lang = $pathParts[0];
        
        if (count($pathParts) > 1) {
          $page = $pathParts[1];
        }
      }
      else {
        $page = $pathParts[0];
      }
    }
  
    $currentVersion = "1.4.11";
    $currentDate = mktime(0, 0, 0, 04, 12, 2013);

    // naming c: mac universal dropped
    $ver14c = array("1.4.10", "1.4.9");

    // naming b: only mac 10.4 is universal
    $ver14b = array("1.4.8", "1.4.7", "1.4.6", "1.4.5");

    // naming a: mac releases are universal
    $ver14a = array("1.4.4", "1.4.3", "1.4.2");

    $files = new Files();
    if ($files->downloadRequested($page)) {
      $files->download();
      exit;
    }

    if (!$this->isBot()) {
      $this->session->start();
    }

    $locale = new Locale($this);
    $locale->run($lang);

    $smarty = new \Smarty; // must come first; smarty makes T_ work somehow.
    $lang = $locale->lang;

    switch ($page) {
    case "creditcard":
      $title = T_("Credit Card");
      $this->creditCard($smarty);
      break;
    }
    
    if (!isset($title)) {
      $title = "Synergy" . (($page != "home") ? (" - " . T_(ucfirst($page))) : "");
    }
    
    $baseUrl = $this->getRoot();
    $baseWithLang = $baseUrl . (stristr($lang, "en") ? "" : "/" . $lang);
    
    $smarty->assign("lang", $lang);
    $smarty->assign("baseUrl", $baseUrl);
    $smarty->assign("baseWithLang", $baseWithLang);
    $smarty->assign("secureSite", $this->settings["general"]["secureSite"]);
    $smarty->assign("gsLang", $locale->getGoogleSearchLang());
    $smarty->assign("page", $page);
    $smarty->assign("title", $title);
    $smarty->assign("langIsEnglish", stristr($lang, "en"));
    $smarty->assign("langDir", $locale->getLangDir());
    $smarty->assign("splashImage", $locale->getSplashImage());
    $smarty->assign("premium", $this->premium);
    $smarty->assign("year", date("Y"));

    if ($page == "download") {
      
      if (isset($_GET["donate"]) && $this->premium->isUserPremium()) {
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
        $smarty->assign("ver14c", $ver14c);
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
        else if (isset($_GET["currency"])) {
          $premium = new Premium($this->settings, $this->session);
          exit($premium->convertCurrency());
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

    $view = "views/" . $page . ".html";
    if (!file_exists($view)) {
      header("HTTP/1.0 404 Not Found"); 
      die("<h1>404: Not Found</h1>");
    }
    
    $content = $smarty->fetch($view);
    $smarty->assign("content", $content);
    $smarty->display("views/layout.html");
  }
  
  public function creditCard($smarty) {
    if (!isset($_POST["amount"])) {
      throw new \Exception("missing amount field.");
    }
    
    $smarty->assign("startYear", date("Y"));
    $smarty->assign("endYear", date("Y") + 20);
    $smarty->assign("amount", $_POST["amount"]);
    $smarty->assign("userId", $_POST["userId"]);
    
    $monthList = array();
    for ($i = 0; $i < 11; $i++) {
      $number = $i + 1;
      $text = date("F", mktime(0, 0, 0, $number, 10));
      $monthList[] = array(
        "number" => $number,
        "text" => str_pad($number, 2, "0", STR_PAD_LEFT) . " - " . $text
      );
    }
    $smarty->assign("monthList", $monthList);
    
    $failed = false;
    $success = false;
    
    if (isset($_POST["number"])) {
      $payment = new Payment($this->settings);
      try {
        $response = $payment->process();      
        $success = $response["ACK"] == "Success";
        $failed = !$success;
        
        if ($success) {
          $funds = (float)$response["AMT"];
          $userId = $_POST["userId"];
          
          $premium = new Premium($this->settings, $this->session);
          $premium->assignVotesFromFunds($userId, $funds);
        }
        else if ($failed) {
          $smarty->assign("failMessage", $response["L_LONGMESSAGE0"]);
        }
      }
      catch (PaymentException $e) {
        $failed = true;
        $smarty->assign("failMessage", $e->getMessage());
      }
    }
    
    $card = array();
    $card["number"] = isset($_POST["number"]) ? $_POST["number"] : null;
    $card["name"] = isset($_POST["name"]) ? $_POST["name"] : null;
    $card["month"] = isset($_POST["month"]) ? $_POST["month"] : null;
    $card["year"] = isset($_POST["year"]) ? $_POST["year"] : null;
    $card["cvv2"] = isset($_POST["cvv2"]) ? $_POST["cvv2"] : null;
    $smarty->assign("card", $card);
    
    $smarty->assign("showForm", !$success);
    $smarty->assign("showFailed", $failed);
    $smarty->assign("showSuccess", $success);
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
  
  public function getRoot() {
    $scriptName = $_SERVER['SCRIPT_NAME'];
    $lastSlash = strrpos($scriptName, "/");
    return substr($scriptName, 0, $lastSlash);
  }
}
 
?>