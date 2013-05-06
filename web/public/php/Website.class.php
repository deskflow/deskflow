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

require_once "Locale.class.php";
require_once "Controllers/DownloadController.php";
require_once "Controllers/PremiumController.php";
require_once "smarty/libs/Smarty.class.php";

class Website extends Controllers\Controller {

  public function __construct() {
    $this->settings = parse_ini_file("settings.ini", true);
    $this->session = new SessionManager($this->settings);
    $this->smarty = new \Smarty;
    $this->premium = new Controllers\PremiumController($this->smarty, $this);
  }
  
  public function start() {

    $path = isset($_GET["path"]) ? $_GET["path"] : null;
    $pathParts = preg_split('/\//', $path, null, PREG_SPLIT_NO_EMPTY);
    
    // defaults
    $this->page = "home";
    $lang = null;
    
    if (count($pathParts) != 0) {
      if (preg_match("/^\w\w(\-\w\w)?$/", $pathParts[0], $matches)) {
        $lang = $pathParts[0];
        
        if (count($pathParts) > 1) {
          $this->page = $pathParts[1];
        }
      }
      else {
        $this->page = $pathParts[0];
      }
    }
    
    if (!$this->isBot()) {
      $this->session->start();
    }

    $locale = new Locale($this);
    $locale->run($lang);
    $lang = $locale->lang;
    
    if (!isset($title)) {
      $title = "Synergy" . (($this->page != "home") ? (" - " . T_(ucfirst($this->page))) : "");
    }
    
    $baseUrl = $this->getRoot();
    $baseWithLang = $baseUrl . (stristr($lang, "en") ? "" : "/" . $lang);
    
    $smarty = $this->smarty;
    $smarty->assign("lang", $lang);
    $smarty->assign("baseUrl", $baseUrl);
    $smarty->assign("baseWithLang", $baseWithLang);
    $smarty->assign("secureSite", $this->settings["general"]["secureSite"]);
    $smarty->assign("gsLang", $locale->getGoogleSearchLang());
    $smarty->assign("page", $this->page);
    $smarty->assign("title", $title);
    $smarty->assign("langIsEnglish", stristr($lang, "en"));
    $smarty->assign("langDir", $locale->getLangDir());
    $smarty->assign("splashImage", $locale->getSplashImage());
    $smarty->assign("premium", $this->premium);
    $smarty->assign("year", date("Y"));
    $smarty->assign("timestamp", time());
    
    $controller = $this;
    if ($this->page == "download") {
      $controller = new Controllers\DownloadController($smarty, $this);
    }
    else if ($this->page == "premium") {
      $controller = $this->premium;
    }
    $controller->run($path);
  }
  
  private function run($path) {
    $this->showView($this->page);
  }

  public function isBot() {
    return in_array("HTTP_USER_AGENT", $_SERVER) &&
      preg_match("/(bot|spider)/", $_SERVER["HTTP_USER_AGENT"]) != 0;
  }
  
  public function getRoot() {
    $scriptName = $_SERVER['SCRIPT_NAME'];
    $lastSlash = strrpos($scriptName, "/");
    return substr($scriptName, 0, $lastSlash);
  }
}
 
?>