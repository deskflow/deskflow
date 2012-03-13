<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsFunction
 */

/**
 * Smarty {html_checkboxes} function plugin
 *
 * File:       function.html_checkboxes.php<br>
 * Type:       function<br>
 * Name:       html_checkboxes<br>
 * Date:       24.Feb.2003<br>
 * Purpose:    Prints out a list of checkbox input types<br>
 * Examples:
 * <pre>
 * {html_checkboxes values=$ids output=$names}
 * {html_checkboxes values=$ids name='box' separator='<br>' output=$names}
 * {html_checkboxes values=$ids checked=$checked separator='<br>' output=$names}
 * </pre>
 * Params:
 * <pre>
 * - name       (optional) - string default "checkbox"
 * - values     (required) - array
 * - options    (optional) - associative array
 * - checked    (optional) - array default not set
 * - separator  (optional) - ie <br> or &nbsp;
 * - output     (optional) - the output next to each checkbox
 * - assign     (optional) - assign the output as an array to this variable
 * - escape     (optional) - escape the content (not value), defaults to true
 * </pre>
 *
 * @link http://www.smarty.net/manual/en/language.function.html.checkboxes.php {html_checkboxes}
 *      (Smarty online manual)
 * @author     Christopher Kvarme <christopher.kvarme@flashjab.com>
 * @author credits to Monte Ohrt <monte at ohrt dot com>
 * @version    1.0
 * @param array $params parameters
 * @param object $template template object
 * @return string
 * @uses smarty_function_escape_special_chars()
 */
function smarty_function_html_checkboxes($params, $template)
{
    require_once(SMARTY_PLUGINS_DIR . 'shared.escape_special_chars.php');

    $name = 'checkbox';
    $values = null;
    $options = null;
    $selected = array();
    $separator = '';
    $escape = true;
    $labels = true;
    $label_ids = false;
    $output = null;

    $extra = '';

    foreach($params as $_key => $_val) {
        switch($_key) {
            case 'name':
            case 'separator':
                $$_key = (string) $_val;
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

            case 'checked':
            case 'selected':
                if (is_array($_val)) {
                    $selected = array();
                    foreach ($_val as $_sel) {
                        if (is_object($_sel)) {
                            if (method_exists($_sel, "__toString")) {
                                $_sel = smarty_function_escape_special_chars((string) $_sel->__toString());
                            } else {
                                trigger_error("html_checkboxes: selected attribute contains an object of class '". get_class($_sel) ."' without __toString() method", E_USER_NOTICE);
                                continue;
                            }
                        } else {
                            $_sel = smarty_function_escape_special_chars((string) $_sel);
                        }
                        $selected[$_sel] = true;
                    }
                } elseif (is_object($_val)) {
                    if (method_exists($_val, "__toString")) {
                        $selected = smarty_function_escape_special_chars((string) $_val->__toString());
                    } else {
                        trigger_error("html_checkboxes: selected attribute is an object of class '". get_class($_val) ."' without __toString() method", E_USER_NOTICE);
                    }
                } else {
                    $selected = smarty_function_escape_special_chars((string) $_val);
                }
                break;

            case 'checkboxes':
                trigger_error('html_checkboxes: the use of the "checkboxes" attribute is deprecated, use "options" instead', E_USER_WARNING);
                $options = (array) $_val;
                break;

            case 'assign':
                break;

            default:
                if(!is_array($_val)) {
                    $extra .= ' '.$_key.'="'.smarty_function_escape_special_chars($_val).'"';
                } else {
                    trigger_error("html_checkboxes: extra attribute '$_key' cannot be an array", E_USER_NOTICE);
                }
                break;
        }
    }

    if (!isset($options) && !isset($values))
        return ''; /* raise error here? */

    $_html_result = array();

    if (isset($options)) {
        foreach ($options as $_key=>$_val) {
            $_html_result[] = smarty_function_html_checkboxes_output($name, $_key, $_val, $selected, $extra, $separator, $labels, $label_ids, $escape);
        }
    } else {
        foreach ($values as $_i=>$_key) {
            $_val = isset($output[$_i]) ? $output[$_i] : '';
            $_html_result[] = smarty_function_html_checkboxes_output($name, $_key, $_val, $selected, $extra, $separator, $labels, $label_ids, $escape);
        }
    }

    if(!empty($params['assign'])) {
        $template->assign($params['assign'], $_html_result);
    } else {
        return implode("\n", $_html_result);
    }

}

function smarty_function_html_checkboxes_output($name, $value, $output, $selected, $extra, $separator, $labels, $label_ids, $escape=true) {
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
    
    $_output .= '<input type="checkbox" name="' . $name . '[]" value="' . $value . '"';
    
    if ($labels && $label_ids) {
        $_output .= ' id="' . $_id . '"';
    }
    
    if (is_array($selected)) {
        if (isset($selected[$value])) {
            $_output .= ' checked="checked"';
        }
    } elseif ($value === $selected) {
        $_output .= ' checked="checked"';
    }
    
    $_output .= $extra . ' />' . $output;
    if ($labels) {
        $_output .= '</label>';
    }
    
    $_output .=  $separator;
    return $_output;
}

?>