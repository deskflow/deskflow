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
 * Smarty {html_select_date} plugin
 * 
 * Type:     function<br>
 * Name:     html_select_date<br>
 * Purpose:  Prints the dropdowns for date selection.
 * 
 * ChangeLog:
 * <pre>
 *            - 1.0 initial release
 *            - 1.1 added support for +/- N syntax for begin
 *              and end year values. (Monte)
 *            - 1.2 added support for yyyy-mm-dd syntax for
 *              time value. (Jan Rosier)
 *            - 1.3 added support for choosing format for
 *              month values (Gary Loescher)
 *            - 1.3.1 added support for choosing format for
 *              day values (Marcus Bointon)
 *            - 1.3.2 support negative timestamps, force year
 *              dropdown to include given date unless explicitly set (Monte)
 *            - 1.3.4 fix behaviour of 0000-00-00 00:00:00 dates to match that
 *              of 0000-00-00 dates (cybot, boots)
 *            - 2.0 complete rewrite for performance,  
 *              added attributes month_names, *_id
 * </pre>
 * 
 * @link http://www.smarty.net/manual/en/language.function.html.select.date.php {html_select_date}
 *      (Smarty online manual)
 * @version 2.0
 * @author Andrei Zmievski 
 * @author Monte Ohrt <monte at ohrt dot com> 
 * @author Rodney Rehm
 * @param array                    $params   parameters
 * @param Smarty_Internal_Template $template template object
 * @return string 
 */
function smarty_function_html_select_date($params, $template)
{
    // generate timestamps used for month names only
    static $_month_timestamps = null;
    static $_current_year = null;
    if ($_month_timestamps === null) {
        $_current_year = date('Y');
        $_month_timestamps = array();
        for ($i = 1; $i <= 12; $i++) {
            $_month_timestamps[$i] = mktime(0, 0, 0, $i, 1, 2000);
        }
    }

    /* Default values. */
    $prefix = "Date_";
    $start_year = null;
    $end_year = null;
    $display_days = true;
    $display_months = true;
    $display_years = true;
    $month_format = "%B";
    /* Write months as numbers by default  GL */
    $month_value_format = "%m";
    $day_format = "%02d";
    /* Write day values using this format MB */
    $day_value_format = "%d";
    $year_as_text = false;
    /* Display years in reverse order? Ie. 2000,1999,.... */
    $reverse_years = false;
    /* Should the select boxes be part of an array when returned from PHP?
       e.g. setting it to "birthday", would create "birthday[Day]",
       "birthday[Month]" & "birthday[Year]". Can be combined with prefix */
    $field_array = null;
    /* <select size>'s of the different <select> tags.
       If not set, uses default dropdown. */
    $day_size = null;
    $month_size = null;
    $year_size = null;
    /* Unparsed attributes common to *ALL* the <select>/<input> tags.
       An example might be in the template: all_extra ='class ="foo"'. */
    $all_extra = null;
    /* Separate attributes for the tags. */
    $day_extra = null;
    $month_extra = null;
    $year_extra = null;
    /* Order in which to display the fields.
       "D" -> day, "M" -> month, "Y" -> year. */
    $field_order = 'MDY';
    /* String printed between the different fields. */
    $field_separator = "\n";
    $option_separator = "\n";
    $time = null;
    // $all_empty = null;
    // $day_empty = null;
    // $month_empty = null;
    // $year_empty = null;
    $extra_attrs = '';
    $all_id = null;
    $day_id = null;
    $month_id = null;
    $year_id = null;

    foreach ($params as $_key => $_value) {
        switch ($_key) {
            case 'time':
                if (!is_array($_value) && $_value !== null) {
                    $time = smarty_make_timestamp($_value);
                }
                break;
                
            case 'month_names':
                if (is_array($_value) && count($_value) == 12) {
                    $$_key = $_value;
                } else {
                    trigger_error("html_select_date: month_names must be an array of 12 strings", E_USER_NOTICE);
                }
                break;
                
            case 'prefix':
            case 'field_array':
            case 'start_year':
            case 'end_year':
            case 'day_format':
            case 'day_value_format':
            case 'month_format':
            case 'month_value_format':
            case 'day_size':
            case 'month_size':
            case 'year_size':
            case 'all_extra':
            case 'day_extra':
            case 'month_extra':
            case 'year_extra':
            case 'field_order':
            case 'field_separator':
            case 'option_separator':
            case 'all_empty':
            case 'month_empty':
            case 'day_empty':
            case 'year_empty':
            case 'all_id':
            case 'month_id':
            case 'day_id':
            case 'year_id':
                $$_key = (string)$_value;
                break;

            case 'display_days':
            case 'display_months':
            case 'display_years':
            case 'year_as_text':
            case 'reverse_years':
                $$_key = (bool)$_value;
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
    
    // Note: date() is faster than strftime()
    // Note: explode(date()) is faster than date() date() date()
    if (isset($params['time']) && is_array($params['time'])) {
        if (isset($params['time'][$prefix . 'Year'])) {
            // $_REQUEST[$field_array] given
            foreach (array('Y' => 'Year',  'm' => 'Month', 'd' => 'Day') as $_elementKey => $_elementName) {
                $_variableName = '_' . strtolower($_elementName);
                $$_variableName = isset($params['time'][$prefix . $_elementName])
                    ? $params['time'][$prefix . $_elementName]
                    : date($_elementKey);
            }
            $time = mktime(0, 0, 0, $_month, $_day, $_year);
        } elseif (isset($params['time'][$field_array][$prefix . 'Year'])) {
            // $_REQUEST given
            foreach (array('Y' => 'Year',  'm' => 'Month', 'd' => 'Day') as $_elementKey => $_elementName) {
                $_variableName = '_' . strtolower($_elementName);
                $$_variableName = isset($params['time'][$field_array][$prefix . $_elementName])
                    ? $params['time'][$field_array][$prefix . $_elementName]
                    : date($_elementKey);
            }
            $time = mktime(0, 0, 0, $_month, $_day, $_year);
        } else {
            // no date found, use NOW
            list($_year, $_month, $_day) = $time = explode('-', date('Y-m-d'));
        }
    } elseif ($time === null) {
        if (array_key_exists('time', $params)) {
            $_year = $_month = $_day = $time = null;
        } else {
            list($_year, $_month, $_day) = $time = explode('-', date('Y-m-d'));
        }
    } else {
        list($_year, $_month, $_day) = $time = explode('-', date('Y-m-d', $time));
    }

    // make syntax "+N" or "-N" work with $start_year and $end_year
    // Note preg_match('!^(\+|\-)\s*(\d+)$!', $end_year, $match) is slower than trim+substr
    foreach (array('start', 'end') as $key) {
        $key .= '_year';
        $t = $$key;
        if ($t === null) {
            $$key = (int)$_current_year;
        } else if ($t[0] == '+') {
            $$key = (int)($_current_year + trim(substr($t, 1)));
        } else if ($t[0] == '-') {
            $$key = (int)($_current_year - trim(substr($t, 1)));
        } else {
            $$key = (int)$$key;
        }
    }

    // flip for ascending or descending
    if (($start_year > $end_year && !$reverse_years) || ($start_year < $end_year && $reverse_years)) {
        $t = $end_year;
        $end_year = $start_year;
        $start_year = $t;
    }

    // generate year <select> or <input>
    if ($display_years) {
        $_html_years = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Year]') : ($prefix . 'Year');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        } 
        if ($year_extra) {
            $_extra .= ' ' . $year_extra;
        }
        
        if ($year_as_text) {
            $_html_years = '<input type="text" name="' . $_name . '" value="' . $_year . '" size="4" maxlength="4"' . $_extra . $extra_attrs . ' />';
        } else {
            $_html_years = '<select name="' . $_name . '"';
            if ($year_id !== null || $all_id !== null) {
                $_html_years .= ' id="' . smarty_function_escape_special_chars( 
                    $year_id !== null ? ( $year_id ? $year_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name ) 
                ) . '"';
            }
            if ($year_size) {
                $_html_years .= ' size="' . $year_size . '"';
            } 
            $_html_years .= $_extra . $extra_attrs . '>' . $option_separator;
            
            if (isset($year_empty) || isset($all_empty)) {
                $_html_years .= '<option value="">' . ( isset($year_empty) ? $year_empty : $all_empty ) . '</option>' . $option_separator;
            }
            
            $op = $start_year > $end_year ? -1 : 1;
            for ($i=$start_year; $op > 0 ? $i <= $end_year : $i >= $end_year; $i += $op) {
                $_html_years .= '<option value="' . $i . '"'
                    . ($_year == $i ? ' selected="selected"' : '')
                    . '>' . $i . '</option>' . $option_separator;
            }
            
            $_html_years .= '</select>';
        }
    }
    
    // generate month <select> or <input>
    if ($display_months) {
        $_html_month = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Month]') : ($prefix . 'Month');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        } 
        if ($month_extra) {
            $_extra .= ' ' . $month_extra;
        }
        
        $_html_months = '<select name="' . $_name . '"';
        if ($month_id !== null || $all_id !== null) {
            $_html_months .= ' id="' . smarty_function_escape_special_chars( 
                $month_id !== null ? ( $month_id ? $month_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name ) 
            ) . '"';
        }
        if ($month_size) {
            $_html_months .= ' size="' . $month_size . '"';
        } 
        $_html_months .= $_extra . $extra_attrs . '>' . $option_separator;
        
        if (isset($month_empty) || isset($all_empty)) {
            $_html_months .= '<option value="">' . ( isset($month_empty) ? $month_empty : $all_empty ) . '</option>' . $option_separator;
        }
        
        for ($i = 1; $i <= 12; $i++) {
            $_val = sprintf('%02d', $i);
            $_text = isset($month_names) ? smarty_function_escape_special_chars($month_names[$i]) : ($month_format == "%m" ? $_val : strftime($month_format, $_month_timestamps[$i]));
            $_value = $month_value_format == "%m" ? $_val : strftime($month_value_format, $_month_timestamps[$i]);
            $_html_months .= '<option value="' . $_value . '"'
                . ($_val == $_month ? ' selected="selected"' : '')
                . '>' . $_text . '</option>' . $option_separator;
        }
        
        $_html_months .= '</select>';
    }
    
    // generate day <select> or <input>
    if ($display_days) {
        $_html_day = '';
        $_extra = '';
        $_name = $field_array ? ($field_array . '[' . $prefix . 'Day]') : ($prefix . 'Day');
        if ($all_extra) {
            $_extra .= ' ' . $all_extra;
        } 
        if ($day_extra) {
            $_extra .= ' ' . $day_extra;
        }
        
        $_html_days = '<select name="' . $_name . '"';
        if ($day_id !== null || $all_id !== null) {
            $_html_days .= ' id="' . smarty_function_escape_special_chars( 
                $day_id !== null ? ( $day_id ? $day_id : $_name ) : ( $all_id ? ($all_id . $_name) : $_name ) 
            ) . '"';
        }
        if ($day_size) {
            $_html_days .= ' size="' . $day_size . '"';
        } 
        $_html_days .= $_extra . $extra_attrs . '>' . $option_separator;
        
        if (isset($day_empty) || isset($all_empty)) {
            $_html_days .= '<option value="">' . ( isset($day_empty) ? $day_empty : $all_empty ) . '</option>' . $option_separator;
        }
        
        for ($i = 1; $i <= 31; $i++) {
            $_val = sprintf('%02d', $i);
            $_text = $day_format == '%02d' ? $_val : sprintf($day_format, $i);
            $_value = $day_value_format ==  '%02d' ? $_val : sprintf($day_value_format, $i);
            $_html_days .= '<option value="' . $_value . '"'
                . ($_val == $_day ? ' selected="selected"' : '')
                . '>' . $_text . '</option>' . $option_separator;
        }
        
        $_html_days .= '</select>';
    }

    // order the fields for output
    $_html = '';
    for ($i=0; $i <= 2; $i++) {
        switch ($field_order[$i]) {
            case 'Y':
            case 'y':
                if (isset($_html_years)) {
                    if ($_html) {
                        $_html .= $field_separator;
                    }
                    $_html .= $_html_years;
                }
            break;
            
            case 'm':
            case 'M':
                if (isset($_html_months)) {
                    if ($_html) {
                        $_html .= $field_separator;
                    }
                    $_html .= $_html_months;
                }
            break;
            
            case 'd':
            case 'D':
                if (isset($_html_days)) {
                    if ($_html) {
                        $_html .= $field_separator;
                    }
                    $_html .= $_html_days;
                }
            break;
        }
    }
    return $_html;
}

?>