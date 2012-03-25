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

if ($_GET["page"] != "home")
  $smarty->assign("page", $_GET["page"] . "/");
else
  $smarty->assign("page", "");

$page = str_replace("/", "", $_GET["page"]);
$content = $smarty->fetch($page . ".tpl");
$smarty->assign("content", $content);

if ($page != "home")
  $smarty->assign("title", " - " . T_(ucfirst($page)));
else
  $smarty->assign("title", "");

$smarty->display("layout.tpl");

?>
