<?php
/**
 * Smarty Internal Plugin Compile Debug
 *
 * Compiles the {debug} tag.
 * It opens a window the the Smarty Debugging Console.
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Debug Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Debug extends Smarty_Internal_CompileBase {

    /**
     * Compiles code for the {debug} tag
     *
     * @param array  $args     array with attributes from parser
     * @param object $compiler compiler object
     * @return string compiled code
     */
    public function compile($args, $compiler)
    {
        // check and get attributes
        $_attr = $this->getAttributes($compiler, $args);

        // compile always as nocache
        $compiler->tag_nocache = true;

        // display debug template
        $_output = "<?php \$_smarty_tpl->smarty->loadPlugin('Smarty_Internal_Debug'); Smarty_Internal_Debug::display_debug(\$_smarty_tpl); ?>";
        return $_output;
    }

}

?>