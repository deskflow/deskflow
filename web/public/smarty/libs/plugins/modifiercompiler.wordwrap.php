<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsModifierCompiler
 */

/**
 * Smarty wordwrap modifier plugin
 * 
 * Type:     modifier<br>
 * Name:     wordwrap<br>
 * Purpose:  wrap a string of text at a given length
 * 
 * @link http://smarty.php.net/manual/en/language.modifier.wordwrap.php wordwrap (Smarty online manual)
 * @author Uwe Tews 
 * @param array $params parameters
 * @return string with compiled code
 */
function smarty_modifiercompiler_wordwrap($params, $compiler)
{
    if (!isset($params[1])) {
        $params[1] = 80;
    } 
    if (!isset($params[2])) {
        $params[2] = '"\n"';
    } 
    if (!isset($params[3])) {
        $params[3] = 'false';
    } 
    $function = 'wordwrap';
    if (Smarty::$_MBSTRING) {
        if ($compiler->tag_nocache | $compiler->nocache) {
            $compiler->template->required_plugins['nocache']['wordwrap']['modifier']['file'] = SMARTY_PLUGINS_DIR .'shared.mb_wordwrap.php';
            $compiler->template->required_plugins['nocache']['wordwrap']['modifier']['function'] = 'smarty_mb_wordwrap';
        } else {
            $compiler->template->required_plugins['compiled']['wordwrap']['modifier']['file'] = SMARTY_PLUGINS_DIR .'shared.mb_wordwrap.php';
            $compiler->template->required_plugins['compiled']['wordwrap']['modifier']['function'] = 'smarty_mb_wordwrap';
        }
        $function = 'smarty_mb_wordwrap';
    }
    return $function . '(' . $params[0] . ',' . $params[1] . ',' . $params[2] . ',' . $params[3] . ')';
} 

?>