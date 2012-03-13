<?php
/**
 * Smarty Internal Plugin Compile Object Funtion
 *
 * Compiles code for registered objects as function
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Object Function Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Private_Object_Function extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('_any');

    /**
     * Compiles code for the execution of function plugin
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @param string $tag       name of function
     * @param string $method    name of method to call
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter, $tag, $method)
    {
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);
        if ($_attr['nocache'] === true) {
            $compiler->tag_nocache = true;
        }
        unset($_attr['nocache']);
        $_assign = null;
        if (isset($_attr['assign'])) {
            $_assign = $_attr['assign'];
            unset($_attr['assign']);
        }
        // convert attributes into parameter array string
        if ($compiler->smarty->registered_objects[$tag][2]) {
            $_paramsArray = array();
            foreach ($_attr as $_key => $_value) {
                if (is_int($_key)) {
                    $_paramsArray[] = "$_key=>$_value";
                } else {
                    $_paramsArray[] = "'$_key'=>$_value";
                }
            }
            $_params = 'array(' . implode(",", $_paramsArray) . ')';
            $return = "\$_smarty_tpl->smarty->registered_objects['{$tag}'][0]->{$method}({$_params},\$_smarty_tpl)";
        } else {
            $_params = implode(",", $_attr);
            $return = "\$_smarty_tpl->smarty->registered_objects['{$tag}'][0]->{$method}({$_params})";
        }
        if (empty($_assign)) {
            // This tag does create output
            $compiler->has_output = true;
            $output = "<?php echo {$return};?>\n";
        } else {
            $output = "<?php \$_smarty_tpl->assign({$_assign},{$return});?>\n";
        }
        return $output;
    }

}

?>