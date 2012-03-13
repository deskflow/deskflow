<?php
/**
 * Smarty plugin
 * 
 * @package Smarty
 * @subpackage Debug
 */

/**
 * Smarty debug_print_var modifier plugin
 * 
 * Type:     modifier<br>
 * Name:     debug_print_var<br>
 * Purpose:  formats variable contents for display in the console
 *
 * @author Monte Ohrt <monte at ohrt dot com> 
 * @param array|object $var     variable to be formatted
 * @param integer      $depth   maximum recursion depth if $var is an array
 * @param integer      $length  maximum string length if $var is a string
 * @return string 
 */
function smarty_modifier_debug_print_var ($var, $depth = 0, $length = 40)
{
    $_replace = array("\n" => '<i>\n</i>',
        "\r" => '<i>\r</i>',
        "\t" => '<i>\t</i>'
        );

    switch (gettype($var)) {
        case 'array' :
            $results = '<b>Array (' . count($var) . ')</b>';
            foreach ($var as $curr_key => $curr_val) {
                $results .= '<br>' . str_repeat('&nbsp;', $depth * 2)
                 . '<b>' . strtr($curr_key, $_replace) . '</b> =&gt; '
                 . smarty_modifier_debug_print_var($curr_val, ++$depth, $length);
                $depth--;
            } 
            break;
            
        case 'object' :
            $object_vars = get_object_vars($var);
            $results = '<b>' . get_class($var) . ' Object (' . count($object_vars) . ')</b>';
            foreach ($object_vars as $curr_key => $curr_val) {
                $results .= '<br>' . str_repeat('&nbsp;', $depth * 2)
                 . '<b> -&gt;' . strtr($curr_key, $_replace) . '</b> = '
                 . smarty_modifier_debug_print_var($curr_val, ++$depth, $length);
                $depth--;
            } 
            break;
            
        case 'boolean' :
        case 'NULL' :
        case 'resource' :
            if (true === $var) {
                $results = 'true';
            } elseif (false === $var) {
                $results = 'false';
            } elseif (null === $var) {
                $results = 'null';
            } else {
                $results = htmlspecialchars((string) $var);
            } 
            $results = '<i>' . $results . '</i>';
            break;
            
        case 'integer' :
        case 'float' :
            $results = htmlspecialchars((string) $var);
            break;
            
        case 'string' :
            $results = strtr($var, $_replace);
            if (Smarty::$_MBSTRING) {
                if (mb_strlen($var, Smarty::$_CHARSET) > $length) {
                    $results = mb_substr($var, 0, $length - 3, Smarty::$_CHARSET) . '...';
                }
            } else {
                if (isset($var[$length])) {
                    $results = substr($var, 0, $length - 3) . '...';
                }
            }

            $results = htmlspecialchars('"' . $results . '"');
            break;
            
        case 'unknown type' :
        default :
            $results = strtr((string) $var, $_replace);
            if (Smarty::$_MBSTRING) {
                if (mb_strlen($results, Smarty::$_CHARSET) > $length) {
                    $results = mb_substr($results, 0, $length - 3, Smarty::$_CHARSET) . '...';
                }
            } else {
                if (strlen($results) > $length) {
                    $results = substr($results, 0, $length - 3) . '...';
                }
            }
             
            $results = htmlspecialchars($results);
    } 

    return $results;
} 

?>