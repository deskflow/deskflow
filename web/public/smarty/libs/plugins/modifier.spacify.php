<?php
/**
 * Smarty plugin
 * @package Smarty
 * @subpackage PluginsModifier
 */

/**
 * Smarty spacify modifier plugin
 * 
 * Type:     modifier<br>
 * Name:     spacify<br>
 * Purpose:  add spaces between characters in a string
 * 
 * @link http://smarty.php.net/manual/en/language.modifier.spacify.php spacify (Smarty online manual)
 * @author Monte Ohrt <monte at ohrt dot com> 
 * @param string $string       input string
 * @param string $spacify_char string to insert between characters.
 * @return string
 */
function smarty_modifier_spacify($string, $spacify_char = ' ')
{
    // well… what about charsets besides latin and UTF-8?
    return implode($spacify_char, preg_split('//' . Smarty::$_UTF8_MODIFIER, $string, -1, PREG_SPLIT_NO_EMPTY));
} 

?>