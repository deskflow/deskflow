<?php
/**
 * Smarty Internal Plugin Compile Registered Block
 *
 * Compiles code for the execution of a registered block function
 *
 * @package Smarty
 * @subpackage Compiler
 * @author Uwe Tews
 */

/**
 * Smarty Internal Plugin Compile Registered Block Class
 *
 * @package Smarty
 * @subpackage Compiler
 */
class Smarty_Internal_Compile_Private_Registered_Block extends Smarty_Internal_CompileBase {

    /**
     * Attribute definition: Overwrites base class.
     *
     * @var array
     * @see Smarty_Internal_CompileBase
     */
    public $optional_attributes = array('_any');

    /**
     * Compiles code for the execution of a block function
     *
     * @param array  $args      array with attributes from parser
     * @param object $compiler  compiler object
     * @param array  $parameter array with compilation parameter
     * @param string $tag       name of block function
     * @return string compiled code
     */
    public function compile($args, $compiler, $parameter, $tag)
    {
        if (!isset($tag[5]) || substr($tag,-5) != 'close') {
            // opening tag of block plugin
            // check and get attributes
            $_attr = $this->getAttributes($compiler, $args);
            if ($_attr['nocache']) {
                $compiler->tag_nocache = true;
            }
               unset($_attr['nocache']);
               if (isset($compiler->smarty->registered_plugins[Smarty::PLUGIN_BLOCK][$tag])) {
                   $tag_info = $compiler->smarty->registered_plugins[Smarty::PLUGIN_BLOCK][$tag];
               } else {
                   $tag_info = $compiler->default_handler_plugins[Smarty::PLUGIN_BLOCK][$tag];
               }
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

            $this->openTag($compiler, $tag, array($_params, $compiler->nocache));
            // maybe nocache because of nocache variables or nocache plugin
            $compiler->nocache = !$tag_info[1] | $compiler->nocache | $compiler->tag_nocache;
            $function = $tag_info[0];
            // compile code
            if (!is_array($function)) {
                $output = "<?php \$_smarty_tpl->smarty->_tag_stack[] = array('{$tag}', {$_params}); \$_block_repeat=true; echo {$function}({$_params}, null, \$_smarty_tpl, \$_block_repeat);while (\$_block_repeat) { ob_start();?>";
            } else if (is_object($function[0])) {
                $output = "<?php \$_smarty_tpl->smarty->_tag_stack[] = array('{$tag}', {$_params}); \$_block_repeat=true; echo \$_smarty_tpl->smarty->registered_plugins['block']['{$tag}'][0][0]->{$function[1]}({$_params}, null, \$_smarty_tpl, \$_block_repeat);while (\$_block_repeat) { ob_start();?>";
            } else {
                $output = "<?php \$_smarty_tpl->smarty->_tag_stack[] = array('{$tag}', {$_params}); \$_block_repeat=true; echo {$function[0]}::{$function[1]}({$_params}, null, \$_smarty_tpl, \$_block_repeat);while (\$_block_repeat) { ob_start();?>";
            }
        } else {
            // must endblock be nocache?
            if ($compiler->nocache) {
                $compiler->tag_nocache = true;
            }
            $base_tag = substr($tag, 0, -5);
            // closing tag of block plugin, restore nocache
            list($_params, $compiler->nocache) = $this->closeTag($compiler, $base_tag);
            // This tag does create output
            $compiler->has_output = true;
               if (isset($compiler->smarty->registered_plugins[Smarty::PLUGIN_BLOCK][$base_tag])) {
                   $function = $compiler->smarty->registered_plugins[Smarty::PLUGIN_BLOCK][$base_tag][0];
               } else {
                   $function = $compiler->default_handler_plugins[Smarty::PLUGIN_BLOCK][$base_tag][0];
               }
            // compile code
            if (!isset($parameter['modifier_list'])) {
                $mod_pre = $mod_post ='';
            } else {
                $mod_pre = ' ob_start(); ';
                $mod_post = 'echo '.$compiler->compileTag('private_modifier',array(),array('modifierlist'=>$parameter['modifier_list'],'value'=>'ob_get_clean()')).';';
            }
            if (!is_array($function)) {
                $output = "<?php \$_block_content = ob_get_clean(); \$_block_repeat=false;".$mod_pre." echo {$function}({$_params}, \$_block_content, \$_smarty_tpl, \$_block_repeat);".$mod_post." } array_pop(\$_smarty_tpl->smarty->_tag_stack);?>";
            } else if (is_object($function[0])) {
                $output = "<?php \$_block_content = ob_get_clean(); \$_block_repeat=false;".$mod_pre." echo \$_smarty_tpl->smarty->registered_plugins['block']['{$base_tag}'][0][0]->{$function[1]}({$_params}, \$_block_content, \$_smarty_tpl, \$_block_repeat); ".$mod_post."} array_pop(\$_smarty_tpl->smarty->_tag_stack);?>";
            } else {
                $output = "<?php \$_block_content = ob_get_clean(); \$_block_repeat=false;".$mod_pre." echo {$function[0]}::{$function[1]}({$_params}, \$_block_content, \$_smarty_tpl, \$_block_repeat); ".$mod_post."} array_pop(\$_smarty_tpl->smarty->_tag_stack);?>";
            }
        }
        return $output . "\n";
    }

}

?>