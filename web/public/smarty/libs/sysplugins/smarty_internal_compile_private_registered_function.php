<?php
/**
 * Smarty Internal Plugin Compile Registered Function
 *
 * Compiles code for the execution of a registered function
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Registered Function Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Private_Registered_Function extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('_any');

    /**
     * Compiles code for the execution of a registered function
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @param string $tag       name of function
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter, $tag)
    {
        // This tag does create output
        $compiler->has_output = true;
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);
        if ($_attr['nocache']) {
            $compiler->tag_nocache = true;
        }
        unset($_attr['nocache']);
               if (isset($compiler->smarty->registered_plugins[Smarty::PLUGIN_FUNCTION][$tag])) {
                   $tag_info = $compiler->smarty->registered_plugins[Smarty::PLUGIN_FUNCTION][$tag];
               } else {
                   $tag_info = $compiler->default_handler_plugins[Smarty::PLUGIN_FUNCTION][$tag];
               }
        // not cachable?
        $compiler->tag_nocache =  $compiler->tag_nocache || !$tag_info[1];
        // convert attributes into parameter array string
        $_paramsArray = array();
        foreach ($_attr as $_key => $_value) {
            if (is_int($_key)) {
                $_paramsArray[] = "$_key=>$_value";
            } elseif ($compiler->template->caching && in_array($_key,$tag_info[2])) {
                $_value = str_replace("'","^#^",$_value);
                $_paramsArray[] = "'$_key'=>^#^.var_export($_value,true).^#^";
            } else {
                $_paramsArray[] = "'$_key'=>$_value";
            }
        }
        $_params = 'array(' . implode(",", $_paramsArray) . ')';
        $function = $tag_info[0];
        // compile code
        if (!is_array($function)) {
            $output = "<?php echo {$function}({$_params},\$_smarty_tpl);?>\n";
        } else if (is_object($function[0])) {
            $output = "<?php echo \$_smarty_tpl->smarty->registered_plugins[Smarty::PLUGIN_FUNCTION]['{$tag}'][0][0]->{$function[1]}({$_params},\$_smarty_tpl);?>\n";
        } else {
            $output = "<?php echo {$function[0]}::{$function[1]}({$_params},\$_smarty_tpl);?>\n";
        }
        return $output;
    }

}

?>