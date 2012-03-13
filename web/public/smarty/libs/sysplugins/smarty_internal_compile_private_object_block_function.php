<?php
/**
 * Smarty Internal Plugin Compile Object Block Function
 *
 * Compiles code for registered objects as block function
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Object Block Function Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Private_Object_Block_Function extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('_any');

    /**
     * Compiles code for the execution of block plugin
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @param string $tag       name of block object
     * @param string $method    name of method to call
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter, $tag, $method)
    {
        if (!isset($tag[5]) || substr($tag, -5) != 'close') {
            // opening tag of block plugin
            // check and get attributes
            $_attr = $this->getAttributes($compiler, $args);
            if ($_attr['nocache'] === true) {
                $compiler->tag_nocache = true;
            }
            unset($_attr['nocache']);
            // convert attributes into parameter array string
            $_paramsArray = array();
            foreach ($_attr as $_key => $_value) {
                if (is_int($_key)) {
                    $_paramsArray[] = "$_key=>$_value";
                } else {
                    $_paramsArray[] = "'$_key'=>$_value";
                }
            }
            $_params = 'array(' . implode(",", $_paramsArray) . ')';

            $this->openTag($compiler, $tag . '->' . $method, array($_params, $compiler->nocache));
            // maybe nocache because of nocache variables or nocache plugin
            $compiler->nocache = $compiler->nocache | $compiler->tag_nocache;
            // compile code
            $output = "<?php \$_smarty_tpl->smarty->_tag_stack[] = array('{$tag}->{$method}', {$_params}); \$_block_repeat=true; echo \$_smarty_tpl->smarty->registered_objects['{$tag}'][0]->{$method}({$_params}, null, \$_smarty_tpl, \$_block_repeat);while (\$_block_repeat) { ob_start();?>";
        } else {
            $base_tag = substr($tag, 0, -5);
            // must endblock be nocache?
            if ($compiler->nocache) {
                $compiler->tag_nocache = true;
            }
            // closing tag of block plugin, restore nocache
            list($_params, $compiler->nocache) = $this->closeTag($compiler, $base_tag . '->' . $method);
            // This tag does create output
            $compiler->has_output = true;
            // compile code
            if (!isset($parameter['modifier_list'])) {
                $mod_pre = $mod_post = '';
            } else {
                $mod_pre = ' ob_start(); ';
                $mod_post = 'echo ' . $compiler->compileTag('private_modifier', array(), array('modifierlist' => $parameter['modifier_list'], 'value' => 'ob_get_clean()')) . ';';
            }
            $output = "<?php \$_block_content = ob_get_contents(); ob_end_clean(); \$_block_repeat=false;" . $mod_pre . " echo \$_smarty_tpl->smarty->registered_objects['{$base_tag}'][0]->{$method}({$_params}, \$_block_content, \$_smarty_tpl, \$_block_repeat); " . $mod_post . "  } array_pop(\$_smarty_tpl->smarty->_tag_stack);?>";
        }
        return $output . "\n";
    }

}

?>