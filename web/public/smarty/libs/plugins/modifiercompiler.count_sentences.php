<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsModifierCompiler
 */

/**
 * Smarty count_sentences modifier plugin
 *
 * Type:     modifier<br>
 * Name:     count_sentences
 * Purpose:  count the number of sentences in a text
 *
 * @link http://www.smarty.net/manual/en/language.modifier.count.paragraphs.php
 *          count_sentences (Smarty online manual)
 * @author Uwe Tews
 * @param array $params parameters
 * @return string with compiled code
 */
function smarty_modifiercompiler_count_sentences($params, $compiler)
{
    // find periods, question marks, exclamation marks with a word before but not after.
    return 'preg_match_all("#\w[\.\?\!](\W|$)#S' . Smarty::$_UTF8_MODIFIER . '", ' . $params[0] . ', $tmp)';
}

?>