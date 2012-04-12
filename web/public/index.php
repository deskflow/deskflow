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

require "locale.php";
require "smarty/libs/Smarty.class.php";

function getGoogleSearchLang($lang) {
  if ($lang == "zh")
    $lang = "zh-cn";
  return $lang;
}

$smarty = new Smarty;
$smarty->assign("lang", $lang);
$smarty->assign("baseUrl", $lang != "en" ? "/" . $lang : "");
$smarty->assign("gsLang", getGoogleSearchLang($lang));

$page = $_GET["page"];
$smarty->assign("page", $page);

if ($page != "home")
  $smarty->assign("title", " - " . T_(ucfirst($page)));
else
  $smarty->assign("title", "");
  
if ($page == "download") {
  
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

$page = str_replace("/", "", $_GET["page"]);
$content = $smarty->fetch($page . ".tpl");
$smarty->assign("content", $content);

$smarty->display("layout.tpl");

?>
