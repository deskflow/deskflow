<?php

require "locale.php";
require "smarty/libs/Smarty.class.php";

function getGoogleSearchLang($locale) {
  $lang = reset(explode("_", $locale));
  if ($lang == "zh")
    $lang = "zh-cn";
  return $lang;
}

$smarty = new Smarty;
$smarty->assign("locale", $locale);
$smarty->assign("baseUrl", $locale != "en" ? "/" . $locale : "");
$smarty->assign("gsLang", getGoogleSearchLang($locale));

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
