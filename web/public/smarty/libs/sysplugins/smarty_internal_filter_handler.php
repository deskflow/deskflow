<?php
/**
 * Smarty Internal Plugin Filter Handler
 *
 * Smarty filter handler class
 *
 * @package Smarty
 * @subpackage PluginsInternal
 * @author Uwe Tews
 */

/**
 * Class for filter processing
 *
 * @package Smarty
 * @subpackage PluginsInternal
 */
class Smarty_Internal_Filter_Handler {

    /**
     * Run filters over content
     *
     * The filters will be lazy loaded if required
     * class name format: Smarty_FilterType_FilterName
     * plugin filename format: filtertype.filtername.php
     * Smarty2 filter plugins could be used
     *
     * @param string                   $type     the type of filter ('pre','post','output') which shall run
     * @param string                   $content  the content which shall be processed by the filters
     * @param Smarty_Internal_Template $template template object
     * @return string the filtered content
     */
    public static function runFilter($type, $content, Smarty_Internal_Template $template)
    {
        $output = $content;
        // loop over autoload filters of specified type
        if (!empty($template->smarty->autoload_filters[$type])) {
            foreach ((array)$template->smarty->autoload_filters[$type] as $name) {
                $plugin_name = "Smarty_{$type}filter_{$name}";
                if ($template->smarty->loadPlugin($plugin_name)) {
                    if (function_exists($plugin_name)) {
                        // use loaded Smarty2 style plugin
                        $output = $plugin_name($output, $template);
                    } elseif (class_exists($plugin_name, false)) {
                        // loaded class of filter plugin
                        $output = call_user_func(array($plugin_name, 'execute'), $output, $template);
                    }
                } else {
                    // nothing found, throw exception
                    throw new SmartyException("Unable to load filter {$plugin_name}");
                }
            }
        }
        // loop over registerd filters of specified type
        if (!empty($template->smarty->registered_filters[$type])) {
            foreach ($template->smarty->registered_filters[$type] as $key => $name) {
                if (is_array($template->smarty->registered_filters[$type][$key])) {
                    $output = call_user_func($template->smarty->registered_filters[$type][$key], $output, $template);
                } else {
                    $output = $template->smarty->registered_filters[$type][$key]($output, $template);
                }
            }
        }
        // return filtered output
        return $output;
    }

}

?>