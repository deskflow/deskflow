<?php
function smarty_block_t($params, $text, &$smarty)
{
  if ($text == "")
    return "";
  
	$text = str_replace("\r\n", ' ', $text);
	$text = str_replace("\n", ' ', $text);
	return gettext($text);
}
?>
