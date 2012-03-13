<?php
/**
 * Smarty plugin
 * 
 * @package Smarty
 * @subpackage PluginsFunction
 */

/**
 * Smarty {html_image} function plugin
 * 
 * Type:     function<br>
 * Name:     html_image<br>
 * Date:     Feb 24, 2003<br>
 * Purpose:  format HTML tags for the image<br>
 * Examples: {html_image file="/images/masthead.gif"}<br>
 * Output:   <img src="/images/masthead.gif" width=400 height=23><br>
 * Params:
 * <pre>
 * - file        - (required) - file (and path) of image
 * - height      - (optional) - image height (default actual height)
 * - width       - (optional) - image width (default actual width)
 * - basedir     - (optional) - base directory for absolute paths, default is environment variable DOCUMENT_ROOT
 * - path_prefix - prefix for path output (optional, default empty)
 * </pre>
 * 
 * @link http://www.smarty.net/manual/en/language.function.html.image.php {html_image}
 *      (Smarty online manual)
 * @author Monte Ohrt <monte at ohrt dot com> 
 * @author credits to Duda <duda@big.hu> 
 * @version 1.0
 * @param array                    $params   parameters
 * @param Smarty_Internal_Template $template template object
 * @return string 
 * @uses smarty_function_escape_special_chars()
 */
function smarty_function_html_image($params, $template)
{
    require_once(SMARTY_PLUGINS_DIR . 'shared.escape_special_chars.php');
 
    $alt = '';
    $file = '';
    $height = '';
    $width = '';
    $extra = '';
    $prefix = '';
    $suffix = '';
    $path_prefix = '';
    $basedir = isset($_SERVER['DOCUMENT_ROOT']) ? $_SERVER['DOCUMENT_ROOT'] : '';
    foreach($params as $_key => $_val) {
        switch ($_key) {
            case 'file':
            case 'height':
            case 'width':
            case 'dpi':
            case 'path_prefix':
            case 'basedir':
                $$_key = $_val;
                break;

            case 'alt':
                if (!is_array($_val)) {
                    $$_key = smarty_function_escape_special_chars($_val);
                } else {
                    throw new SmartyException ("html_image: extra attribute '$_key' cannot be an array", E_USER_NOTICE);
                } 
                break;

            case 'link':
            case 'href':
                $prefix = '<a href="' . $_val . '">';
                $suffix = '</a>';
                break;

            default:
                if (!is_array($_val)) {
                    $extra .= ' ' . $_key . '="' . smarty_function_escape_special_chars($_val) . '"';
                } else {
                    throw new SmartyException ("html_image: extra attribute '$_key' cannot be an array", E_USER_NOTICE);
                } 
                break;
        } 
    } 

    if (empty($file)) {
        trigger_error("html_image: missing 'file' parameter", E_USER_NOTICE);
        return;
    } 

    if ($file[0] == '/') {
        $_image_path = $basedir . $file;
    } else {
        $_image_path = $file;
    }
    
    // strip file protocol
    if (stripos($params['file'], 'file://') === 0) {
        $params['file'] = substr($params['file'], 7);
    }
    
    $protocol = strpos($params['file'], '://');
    if ($protocol !== false) {
        $protocol = strtolower(substr($params['file'], 0, $protocol));
    }
    
    if (isset($template->smarty->security_policy)) {
        if ($protocol) {
            // remote resource (or php stream, …)
            if(!$template->smarty->security_policy->isTrustedUri($params['file'])) {
                return;
            }
        } else {
            // local file
            if(!$template->smarty->security_policy->isTrustedResourceDir($params['file'])) {
                return;
            }
        }
    }

    if (!isset($params['width']) || !isset($params['height'])) {
        // FIXME: (rodneyrehm) getimagesize() loads the complete file off a remote resource, use custom [jpg,png,gif]header reader!
        if (!$_image_data = @getimagesize($_image_path)) {
            if (!file_exists($_image_path)) {
                trigger_error("html_image: unable to find '$_image_path'", E_USER_NOTICE);
                return;
            } else if (!is_readable($_image_path)) {
                trigger_error("html_image: unable to read '$_image_path'", E_USER_NOTICE);
                return;
            } else {
                trigger_error("html_image: '$_image_path' is not a valid image file", E_USER_NOTICE);
                return;
            } 
        }

        if (!isset($params['width'])) {
            $width = $_image_data[0];
        } 
        if (!isset($params['height'])) {
            $height = $_image_data[1];
        } 
    } 

    if (isset($params['dpi'])) {
        if (strstr($_SERVER['HTTP_USER_AGENT'], 'Mac')) {
            // FIXME: (rodneyrehm) wrong dpi assumption
            // don't know who thought this up… even if it was true in 1998, it's definitely wrong in 2011.
            $dpi_default = 72;
        } else {
            $dpi_default = 96;
        } 
        $_resize = $dpi_default / $params['dpi'];
        $width = round($width * $_resize);
        $height = round($height * $_resize);
    } 

    return $prefix . '<img src="' . $path_prefix . $file . '" alt="' . $alt . '" width="' . $width . '" height="' . $height . '"' . $extra . ' />' . $suffix;
} 

?>