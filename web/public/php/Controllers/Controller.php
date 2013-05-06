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
 
class Controller {

  public function __construct($smarty) {
    $this->smarty = $smarty;
  }
  
  protected function showView($viewName) {
  
    $custom = "";
    if (is_file("custom/" . $viewName . ".html")) {
      $custom = $this->smarty->fetch("custom/" . $viewName . ".html");
    }
    $this->smarty->assign("custom", $custom);

    $viewPath = "views/" . $viewName . ".html";
    if (!file_exists($viewPath)) {
      header("HTTP/1.0 404 Not Found"); 
      die("<h1>404: Not Found</h1>");
    }
    
    $content = $this->smarty->fetch($viewPath);
    $this->smarty->assign("content", $content);
    $this->smarty->display("views/layout.html");
  }
}
 
 ?>
 