<?php

/**
 * Smarty Internal Plugin Resource PHP
 *
 * Implements the file system as resource for PHP templates
 *
 * @package Smarty
 * @subpackage TemplateResources
 * @author Uwe Tews
 * @author Rodney Rehm
 */
class Smarty_Internal_Resource_PHP extends Smarty_Resource_Uncompiled {
    /**
     * container for short_open_tag directive's value before executing PHP templates
     * @var string
     */
    protected $short_open_tag;

    /**
     * Create a new PHP Resource
     *
     */
    public function __construct()
    {
        $this->short_open_tag = ini_get( 'short_open_tag' );
    }

    /**
     * populate Source Object with meta data from Resource
     *
     * @param Smarty_Template_Source $source source object
     * @param Smarty_Internal_Template $_template template object
     * @return void
     */
    public function populate(Smarty_Template_Source $source, Smarty_Internal_Template $_template=null)
    {
        $source->filepath = $this->buildFilepath($source, $_template);

        if ($source->filepath !== false) {
            if (is_object($source->smarty->security_policy)) {
                $source->smarty->security_policy->isTrustedResourceDir($source->filepath);
            }

            $source->uid = sha1($source->filepath);
            if ($source->smarty->compile_check) {
                $source->timestamp = @filemtime($source->filepath);
                $source->exists = !!$source->timestamp;
            }
        }
    }

    /**
     * populate Source Object with timestamp and exists from Resource
     *
     * @param Smarty_Template_Source $source source object
     * @return void
     */
    public function populateTimestamp(Smarty_Template_Source $source)
    {
        $source->timestamp = @filemtime($source->filepath);
        $source->exists = !!$source->timestamp;
    }

    /**
     * Load template's source from file into current template object
     *
     * @param Smarty_Template_Source $source source object
     * @return string template source
     * @throws SmartyException if source cannot be loaded
     */
    public function getContent(Smarty_Template_Source $source)
    {
        if ($source->timestamp) {
            return '';
        }
        throw new SmartyException("Unable to read template {$source->type} '{$source->name}'");
    }

    /**
     * Render and output the template (without using the compiler)
     *
     * @param Smarty_Template_Source $source source object
     * @param Smarty_Internal_Template $_template template object
     * @return void
     * @throws SmartyException if template cannot be loaded or allow_php_templates is disabled
     */
    public function renderUncompiled(Smarty_Template_Source $source, Smarty_Internal_Template $_template)
    {
        $_smarty_template = $_template;

        if (!$source->smarty->allow_php_templates) {
            throw new SmartyException("PHP templates are disabled");
        }
        if (!$source->exists) {
            if ($_template->parent instanceof Smarty_Internal_Template) {
                $parent_resource = " in '{$_template->parent->template_resource}'";
            } else {
                $parent_resource = '';
            }
            throw new SmartyException("Unable to load template {$source->type} '{$source->name}'{$parent_resource}");
        }

        // prepare variables
        extract($_template->getTemplateVars());

        // include PHP template with short open tags enabled
        ini_set( 'short_open_tag', '1' );
        include($source->filepath);
        ini_set( 'short_open_tag', $this->short_open_tag );
    }
}

?>