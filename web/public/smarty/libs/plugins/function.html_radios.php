<?php
/**
 * Smarty plugin
 * 
 * @package Smarty
 * @subpackage PluginsFunction
 */

/**
 * Smarty {html_radios} function plugin
 * 
 * File:       function.html_radios.php<br>
 * Type:       function<br>
 * Name:       html_radios<br>
 * Date:       24.Feb.2003<br>
 * Purpose:    Prints out a list of radio input types<br>
 * Params:
 * <pre>
 * - name       (optional) - string default "radio"
 * - values     (required) - array
 * - options    (required) - associative array
 * - checked    (optional) - array default not set
 * - separator  (optional) - ie <br> or &nbsp;
 * - output     (optional) - the output next to each radio button
 * - assign     (optional) - assign the output as an array to this variable
 * - escape     (optional) - escape the content (not value), defaults to true
 * </pre>
 * Examples:
 * <pre>
 * {html_radios values=$ids output=$names}
 * {html_radios values=$ids name='box' separator='<br>' output=$names}
 * {html_radios values=$ids checked=$checked separator='<br>' output=$names}
 * </pre>
 * 
 * @link http://smarty.php.net/manual/en/language.function.html.radios.php {html_radios}
 *      (Smarty online manual)
 * @author Christopher Kvarme <christopher.kvarme@flashjab.com> 
 * @author credits to Monte Ohrt <monte at ohrt dot com> 
 * @version 1.0
 * @param array                    $params   parameters
 * @param Smarty_Internal_Template $template template object
 * @return string 
 * @uses smarty_function_escape_special_chars()
 */
function smarty_function_html_radios($params, $template)
{
    require_once(SMARTY_PLUGINS_DIR . 'shared.escape_special_chars.php');

    $name = 'radio';
    $values = null;
    $options = null;
    $selected = null;
    $separator = '';
    $escape = true;
    $labels = true;
    $label_ids = false;
    $output = null;
    $extra = '';

    foreach($params as $_key => $_val) {
        switch ($_key) {
            case 'name':
            case 'separator':
                $$_key = (string) $_val;
                break;

            case 'checked':
            case 'selected':
                if (is_array($_val)) {
                    trigger_error('html_radios: the "' . $_key . '" attribute cannot be an array', E_USER_WARNING);
                } elseif (is_object($_val)) {
                    if (method_exists($_val, "__toString")) {
                        $selected = smarty_function_escape_special_chars((string) $_val->__toString());
                    } else {
                        trigger_error("html_radios: selected attribute is an object of class '". get_class($_val) ."' without __toString() method", E_USER_NOTICE);
                    }
                } else {
                    $selected = (string) $_val;
                } 
                break;

            case 'escape':
            case 'labels':
            case 'label_ids':
                $$_key = (bool) $_val;
                break;

            case 'options':
                $$_key = (array) $_val;
                break;

            case 'values':
            case 'output':
                $$_key = array_values((array) $_val);
                break;

            case 'radios':
                trigger_error('html_radios: the use of the "radios" attribute is deprecated, use "options" instead', E_USER_WARNING);
                $options = (array) $_val;
                break;

            case 'assign':
                break;

            default:
                if (!is_array($_val)) {
                    $extra .= ' ' . $_key . '="' . smarty_function_escape_special_chars($_val) . '"';
                } else {
                    trigger_error("html_radios: extra attribute '$_key' cannot be an array", E_USER_NOTICE);
                } 
                break;
        } 
    } 

    if (!isset($options) && !isset($values)) {
        /* raise error here? */
        return '';
    }

    $_html_result = array();

    if (isset($options)) {
        foreach ($options as $_key => $_val) {
            $_html_result[] = smarty_function_html_radios_output($name, $_key, $_val, $selected, $extra, $separator, $labels, $label_ids, $escape);
        }
    } else {
        foreach ($values as $_i => $_key) {
            $_val = isset($output[$_i]) ? $output[$_i] : '';
            $_html_result[] = smarty_function_html_radios_output($name, $_key, $_val, $selected, $extra, $separator, $labels, $label_ids, $escape);
        } 
    } 

    if (!empty($params['assign'])) {
        $template->assign($params['assign'], $_html_result);
    } else {
        return implode("\n", $_html_result);
    } 
} 

function smarty_function_html_radios_output($name, $value, $output, $selected, $extra, $separator, $labels, $label_ids, $escape)
{
    $_output = '';
    
    if (is_object($value)) {
        if (method_exists($value, "__toString")) {
            $value = (string) $value->__toString();
        } else {
            trigger_error("html_options: value is an object of class '". get_class($value) ."' without __toString() method", E_USER_NOTICE);
            return '';
        }
    } else {
        $value = (string) $value;
    }
    
    if (is_object($output)) {
        if (method_exists($output, "__toString")) {
            $output = (string) $output->__toString();
        } else {
            trigger_error("html_options: output is an object of class '". get_class($output) ."' without __toString() method", E_USER_NOTICE);
            return '';
        }
    } else {
        $output = (string) $output;
    }
    
    if ($labels) {
        if ($label_ids) {
            $_id = smarty_function_escape_special_chars(preg_replace('![^\w\-\.]!' . Smarty::$_UTF8_MODIFIER, '_', $name . '_' . $value));
            $_output .= '<label for="' . $_id . '">';
        } else {
            $_output .= '<label>';
        } 
    }
    
    $name = smarty_function_escape_special_chars($name);
    $value = smarty_function_escape_special_chars($value);
    if ($escape) {
        $output = smarty_function_escape_special_chars($output);
    }
    
    $_output .= '<input type="radio" name="' . $name . '" value="' . $value . '"';

    if ($labels && $label_ids) {
        $_output .= ' id="' . $_id . '"';
    }

    if ($value === $selected) {
        $_output .= ' checked="checked"';
    }
    
    $_output .= $extra . ' />' . $output;
    if ($labels) {
        $_output .= '</label>';
    }
    
    $_output .= $separator;
    return $_output;
} 

?>