<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage PluginsFunction
 */

/**
 * @ignore
 */
require_once(SMARTY_PLUGINS_DIR . 'shared.escape_special_chars.php');
/**
 * @ignore
 */
require_once(SMARTY_PLUGINS_DIR . 'shared.make_timestamp.php');

/**
 * Smarty {html_select_time} function plugin
 *
 * Type:     function<br>
 * Name:     html_select_time<br>
 * Purpose:  Prints the dropdowns for time selection
 *
 * @link http://www.smarty.net/manual/en/language.function.html.select.time.php {html_select_time}
 *          (Smarty online manual)
 * @author Roberto Berto <roberto@berto.net>
 * @author Monte Ohrt <monte AT ohrt DOT com>
 * @param array                    $params   parameters
 * @param Smarty_Internal_Template $template template object
 * @return string
 * @uses smarty_make_timestamp()
 */
function smarty_function_html_select_time($params, $template)
{
    $prefix = "Time_";
    $field_array = null;
    $field_separator = "\n";
    $option_separator = "\n";
    $time = null;

    $display_hours = true;
    $display_minutes = true;
    $display_seconds = true;
    $display_meridian = true;

    $hour_format = '%02d';
    $hour_value_format = '%02d';
    $minute_format = '%02d';
    $minute_value_format = '%02d';
    $second_format = '%02d';
    $second_value_format = '%02d';

    $hour_size = null;
    $minute_size = null;
    $second_size = null;
    $meridian_size = null;

    $all_empty = null;
    $hour_empty = null;
    $minute_empty = null;
    $second_empty = null;
    $meridian_empty = null;

    $all_id = null;
    $hour_id = null;
    $minute_id = null;
    $second_id = null;
    $meridian_id = null;

    $use_24_hours = true;
    $minute_interval = 1;
    $second_interval = 1;

    $extra_attrs = '';
    $all_extra = null;
    $hour_extra = null;
    $minute_extra = null;
    $second_extra = null;
    $meridian_extra = null;

    foreach ($params as $_key => $_value) {
        switch ($_key) {
            case 'time':
                if (!is_array($_value) && $_value !== null) {
                    $time = smarty_make_timestamp($_value);
                }
                break;

            case 'prefix':
            case 'field_array':

            case 'field_separator':
            case 'option_separator':

            case 'all_extra':
            case 'hour_extra':
            case 'minute_extra':
            case 'second_extra':
            case 'meridian_extra':

            case 'all_empty':
            case 'hour_empty':
            case 'minute_empty':
            case 'second_empty':
            case 'meridian_empty':

            case 'all_id':
            case 'hour_id':
            case 'minute_id':
            case 'second_id':
            case 'meridian_id':

            case 'hour_format':
            case 'hour_value_format':
            case 'minute_format':
            case 'minute_value_format':
            case 'second_format':
            case 'second_value_format':
                $$_key = (string)$_value;
                break;

            case 'display_hours':
            case 'display_minutes':
            case 'display_seconds':
            case 'display_meridian':
            case 'use_24_hours':
                $$_key = (bool)$_value;
                break;

            case 'minute_interval':
            case 'second_interval':

            case 'hour_size':
            case 'minute_size':
            case 'second_size':
            case 'meridian_size':
                $$_key = (int)$_value;
                break;

            default:
                if (!is_array($_value)) {
                    $extra_attrs .= ' ' . $_key . '="' . smarty_function_escape_special_chars($_value) . '"';
                } else {
                    trigger_error("html_select_date: extra attribute '$_key' cannot be an array", E_USER_NOTICE);
                }
                break;
        }
    }

    if (isset($params['time']) && is_array($params['time'])) {
        if (isset($params['time'][$prefix . 'Hour'])) {
            // $_REQUEST[$field_array] given
            foreach (array('H' => 'Hour',  'i' => 'Minute', 's' => 'Second') as $_elementKey => $_elementName) {
                $_variableName = '_' . strtolower($_elementName);
                $$_variableName = isset($params['time'][$prefix . $_elementName])
                    ? $params['time'][$prefix . $_elementName]
                    : date($_elementKey);
            }
            $_meridian = isset($params['time'][$prefix . 'Meridian'])
                ? (' ' . $params['time'][$prefix . 'Meridian'])
                : '';
            $time = strtotime( $_hour . ':' . $_minute . ':' . $_second . $_meridian );
            list($_hour, $_minute, $_second) = $time = explode('-', date('H-i-s', $time));
        } elseif (isset($params['time'][$field_array][$prefix . 'Hour'])) {
            // $_REQUEST given
            foreach (array('H' => 'Hour',  'i' => 'Minute', 's' => 'Second') as $_elementKey => $_elementName) {
                $_variableName = '_' . strtolower($_elementName);
                $$_variableName = isset($params['time'][$field_array][$prefix . $_elementName])
                    ? $params['time'][$field_array][$prefix . $_elementName]
                    : date($_elementKey);
            }
            $_meridian = isset($params['time'][$field_array][$prefix . 'Meridian'])
                ? (' ' . $params['time'][$field_array][$prefix . 'Meridian'])
                : '';
            $time = strtotime( $_hour . ':' . $_minute . ':' . $_second . $_meridian );
            list($_hour, $_minute, $_second) = $time = explode('-', date('H-i-s', $time));
        } else {
            // no date found, use NOW
            list($_year, $_month, $_day) = $time = explode('-', date('Y-m-d'));
        }
    } elseif ($time === null) {
        if (array_key_exists('time', $params)) {
            $_hour = $_minute = $_second = $time = null;
        } else {
            list($_hour, $_minute, $_second) = $time = explode('-', date('H-i-s'));
        }
    } else {
        list($_hour, $_minute, $_second) = $time = explode('-', date('H-i-s', $time));
    }

    // generate hour <select>
    if ($display_hours) {
        $_html_hours = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Hour]') : ($prefix . 'Hour');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        }
        if ($hour_extra) {
            $_extra .= ' ' . $hour_extra;
        }

        $_html_hours = '<select name="' . $_name . '"';
        if ($hour_id !== null || $all_id !== null) {
            $_html_hours .= ' id="' . smarty_function_escape_special_chars(
                $hour_id !== null ? ( $hour_id ? $hour_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name )
            ) . '"';
        }
        if ($hour_size) {
            $_html_hours .= ' size="' . $hour_size . '"';
        }
        $_html_hours .= $_extra . $extra_attrs . '>' . $option_separator;

        if (isset($hour_empty) || isset($all_empty)) {
            $_html_hours .= '<option value="">' . ( isset($hour_empty) ? $hour_empty : $all_empty ) . '</option>' . $option_separator;
        }

        $start = $use_24_hours ? 0 : 1;
        $end = $use_24_hours ? 23 : 12;
        for ($i=$start; $i <= $end; $i++) {
            $_val = sprintf('%02d', $i);
            $_text = $hour_format == '%02d' ? $_val : sprintf($hour_format, $i);
            $_value = $hour_value_format == '%02d' ? $_val : sprintf($hour_value_format, $i);

            if (!$use_24_hours) {
                $_hour12 = $_hour == 0
                    ? 12
                    : ($_hour <= 12 ? $_hour : $_hour -12);
            }

            $selected = $_hour !== null ? ($use_24_hours ? $_hour == $_val : $_hour12 == $_val) : null;
            $_html_hours .= '<option value="' . $_value . '"'
                . ($selected ? ' selected="selected"' : '')
                . '>' . $_text . '</option>' . $option_separator;
        }

        $_html_hours .= '</select>';
    }

    // generate minute <select>
    if ($display_minutes) {
        $_html_minutes = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Minute]') : ($prefix . 'Minute');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        }
        if ($minute_extra) {
            $_extra .= ' ' . $minute_extra;
        }

        $_html_minutes = '<select name="' . $_name . '"';
        if ($minute_id !== null || $all_id !== null) {
            $_html_minutes .= ' id="' . smarty_function_escape_special_chars(
                $minute_id !== null ? ( $minute_id ? $minute_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name )
            ) . '"';
        }
        if ($minute_size) {
            $_html_minutes .= ' size="' . $minute_size . '"';
        }
        $_html_minutes .= $_extra . $extra_attrs . '>' . $option_separator;

        if (isset($minute_empty) || isset($all_empty)) {
            $_html_minutes .= '<option value="">' . ( isset($minute_empty) ? $minute_empty : $all_empty ) . '</option>' . $option_separator;
        }

        $selected = $_minute !== null ? ($_minute - $_minute % $minute_interval) : null;
        for ($i=0; $i <= 59; $i += $minute_interval) {
            $_val = sprintf('%02d', $i);
            $_text = $minute_format == '%02d' ? $_val : sprintf($minute_format, $i);
            $_value = $minute_value_format == '%02d' ? $_val : sprintf($minute_value_format, $i);
            $_html_minutes .= '<option value="' . $_value . '"'
                . ($selected === $i ? ' selected="selected"' : '')
                . '>' . $_text . '</option>' . $option_separator;
        }

        $_html_minutes .= '</select>';
    }

    // generate second <select>
    if ($display_seconds) {
        $_html_seconds = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Second]') : ($prefix . 'Second');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        }
        if ($second_extra) {
            $_extra .= ' ' . $second_extra;
        }

        $_html_seconds = '<select name="' . $_name . '"';
        if ($second_id !== null || $all_id !== null) {
            $_html_seconds .= ' id="' . smarty_function_escape_special_chars(
                $second_id !== null ? ( $second_id ? $second_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name )
            ) . '"';
        }
        if ($second_size) {
            $_html_seconds .= ' size="' . $second_size . '"';
        }
        $_html_seconds .= $_extra . $extra_attrs . '>' . $option_separator;

        if (isset($second_empty) || isset($all_empty)) {
            $_html_seconds .= '<option value="">' . ( isset($second_empty) ? $second_empty : $all_empty ) . '</option>' . $option_separator;
        }

        $selected = $_second !== null ? ($_second - $_second % $second_interval) : null;
        for ($i=0; $i <= 59; $i += $second_interval) {
            $_val = sprintf('%02d', $i);
            $_text = $second_format == '%02d' ? $_val : sprintf($second_format, $i);
            $_value = $second_value_format == '%02d' ? $_val : sprintf($second_value_format, $i);
            $_html_seconds .= '<option value="' . $_value . '"'
                . ($selected === $i ? ' selected="selected"' : '')
                . '>' . $_text . '</option>' . $option_separator;
        }

        $_html_seconds .= '</select>';
    }

    // generate meridian <select>
    if ($display_meridian && !$use_24_hours) {
        $_html_meridian = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Meridian]') : ($prefix . 'Meridian');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        }
        if ($meridian_extra) {
            $_extra .= ' ' . $meridian_extra;
        }

        $_html_meridian = '<select name="' . $_name . '"';
        if ($meridian_id !== null || $all_id !== null) {
            $_html_meridian .= ' id="' . smarty_function_escape_special_chars(
                $meridian_id !== null ? ( $meridian_id ? $meridian_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name )
            ) . '"';
        }
        if ($meridian_size) {
            $_html_meridian .= ' size="' . $meridian_size . '"';
        }
        $_html_meridian .= $_extra . $extra_attrs . '>' . $option_separator;

        if (isset($meridian_empty) || isset($all_empty)) {
            $_html_meridian .= '<option value="">' . ( isset($meridian_empty) ? $meridian_empty : $all_empty ) . '</option>' . $option_separator;
        }

        $_html_meridian .= '<option value="am"'. ($_hour < 12 ? ' selected="selected"' : '') .'>AM</option>' . $option_separator
            . '<option value="pm"'. ($_hour < 12 ? '' : ' selected="selected"') .'>PM</option>' . $option_separator
            . '</select>';
    }

    $_html = '';
    foreach (array('_html_hours', '_html_minutes', '_html_seconds', '_html_meridian') as $k) {
        if (isset($$k)) {
            if ($_html) {
                $_html .= $field_separator;
            }
            $_html .= $$k;
        }
    }

    return $_html;
}

?>