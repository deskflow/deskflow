<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsModifierCompiler
 */

/**
 * @ignore
 */
require_once( SMARTY_PLUGINS_DIR .'shared.literal_compiler_param.php' );

/**
 * Smarty escape modifier plugin
 *
 * Type:     modifier<br>
 * Name:     escape<br>
 * Purpose:  escape string for output
 *
 * @link http://www.smarty.net/docsv2/en/language.modifier.escape count_characters (Smarty online manual)
 * @author Rodney Rehm
 * @param array $params parameters
 * @return string with compiled code
 */
function smarty_modifiercompiler_escape($params, $compiler)
{
    try {
        $esc_type = smarty_literal_compiler_param($params, 1, 'html');
        $char_set = smarty_literal_compiler_param($params, 2, Smarty::$_CHARSET);
        $double_encode = smarty_literal_compiler_param($params, 3, true);

        if (!$char_set) {
            $char_set = Smarty::$_CHARSET;
        }

        switch ($esc_type) {
            case 'html':
                return 'htmlspecialchars('
                    . $params[0] .', ENT_QUOTES, '
                    . var_export($char_set, true) . ', '
                    . var_export($double_encode, true) . ')';

            case 'htmlall':
                if (Smarty::$_MBSTRING) {
                    return 'mb_convert_encoding(htmlspecialchars('
                        . $params[0] .', ENT_QUOTES, '
                        . var_export($char_set, true) . ', '
                        . var_export($double_encode, true)
                        . '), "HTML-ENTITIES", '
                        . var_export($char_set, true) . ')';
                }

                // no MBString fallback
                return 'htmlentities('
                    . $params[0] .', ENT_QUOTES, '
                    . var_export($char_set, true) . ', '
                    . var_export($double_encode, true) . ')';

            case 'url':
                return 'rawurlencode(' . $params[0] . ')';

            case 'urlpathinfo':
                return 'str_replace("%2F", "/", rawurlencode(' . $params[0] . '))';

            case 'quotes':
                // escape unescaped single quotes
                return 'preg_replace("%(?<!\\\\\\\\)\'%", "\\\'",' . $params[0] . ')';

            case 'javascript':
                // escape quotes and backslashes, newlines, etc.
                return 'strtr(' . $params[0] . ', array("\\\\" => "\\\\\\\\", "\'" => "\\\\\'", "\"" => "\\\\\"", "\\r" => "\\\\r", "\\n" => "\\\n", "</" => "<\/" ))';

        }
    } catch(SmartyException $e) {
        // pass through to regular plugin fallback
    }

    // could not optimize |escape call, so fallback to regular plugin
    if ($compiler->tag_nocache | $compiler->nocache) {
        $compiler->template->required_plugins['nocache']['escape']['modifier']['file'] = SMARTY_PLUGINS_DIR .'modifier.escape.php';
        $compiler->template->required_plugins['nocache']['escape']['modifier']['function'] = 'smarty_modifier_escape';
    } else {
        $compiler->template->required_plugins['compiled']['escape']['modifier']['file'] = SMARTY_PLUGINS_DIR .'modifier.escape.php';
        $compiler->template->required_plugins['compiled']['escape']['modifier']['function'] = 'smarty_modifier_escape';
    }
    return 'smarty_modifier_escape(' . join( ', ', $params ) . ')';
}

?>