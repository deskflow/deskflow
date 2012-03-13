<?php
/**
* Smarty Internal Plugin Resource Extends
*
* @package Smarty
* @subpackage TemplateResources
* @author Uwe Tews
* @author Rodney Rehm
*/

/**
* Smarty Internal Plugin Resource Extends
*
* Implements the file system as resource for Smarty which {extend}s a chain of template files templates
*
* @package Smarty
* @subpackage TemplateResources
*/
class Smarty_Internal_Resource_Extends extends Smarty_Resource {

    /**
    * mbstring.overload flag
    *
    * @var int
    */
    public $mbstring_overload = 0;

    /**
    * populate Source Object with meta data from Resource
    *
    * @param Smarty_Template_Source   $source    source object
    * @param Smarty_Internal_Template $_template template object
    */
    public function populate(Smarty_Template_Source $source, Smarty_Internal_Template $_template=null)
    {
        $uid = '';
        $sources = array();
        $components = explode('|', $source->name);
        $exists = true;
        foreach ($components as $component) {
            $s = Smarty_Resource::source(null, $source->smarty, $component);
            if ($s->type == 'php') {
                throw new SmartyException("Resource type {$s->type} cannot be used with the extends resource type");
            }
            $sources[$s->uid] = $s;
            $uid .= $s->filepath;
            if ($_template && $_template->smarty->compile_check) {
                $exists = $exists && $s->exists;
            }
        }
        $source->components = $sources;
        $source->filepath = $s->filepath;
        $source->uid = sha1($uid);
        if ($_template && $_template->smarty->compile_check) {
            $source->timestamp = $s->timestamp;
            $source->exists = $exists;
        }
        // need the template at getContent()
        $source->template = $_template;
    }

    /**
    * populate Source Object with timestamp and exists from Resource
    *
    * @param Smarty_Template_Source $source source object
    */
    public function populateTimestamp(Smarty_Template_Source $source)
    {
        $source->exists = true;
        foreach ($source->components as $s) {
            $source->exists = $source->exists && $s->exists;
        }
        $source->timestamp = $s->timestamp;
    }

    /**
    * Load template's source from files into current template object
    *
    * @param Smarty_Template_Source $source source object
    * @return string template source
    * @throws SmartyException if source cannot be loaded
    */
    public function getContent(Smarty_Template_Source $source)
    {
        if (!$source->exists) {
            throw new SmartyException("Unable to read template {$source->type} '{$source->name}'");
        }

        $this->mbstring_overload = ini_get('mbstring.func_overload') & 2;
        $_rdl = preg_quote($source->smarty->right_delimiter);
        $_ldl = preg_quote($source->smarty->left_delimiter);
        $_components = array_reverse($source->components);
        $_first = reset($_components);
        $_last = end($_components);

        foreach ($_components as $_component) {
            // register dependency
            if ($_component != $_first) {
                $source->template->properties['file_dependency'][$_component->uid] = array($_component->filepath, $_component->timestamp, $_component->type);
            }

            // read content
            $source->filepath = $_component->filepath;
            $_content = $_component->content;

            // extend sources
            if ($_component != $_last) {
                if (preg_match_all("!({$_ldl}block\s(.+?){$_rdl})!", $_content, $_open) !=
                preg_match_all("!({$_ldl}/block{$_rdl})!", $_content, $_close)) {
                    throw new SmartyException("unmatched {block} {/block} pairs in template {$_component->type} '{$_component->name}'");
                }
                preg_match_all("!{$_ldl}block\s(.+?){$_rdl}|{$_ldl}/block{$_rdl}|{$_ldl}\*([\S\s]*?)\*{$_rdl}!", $_content, $_result, PREG_OFFSET_CAPTURE);
                $_result_count = count($_result[0]);
                $_start = 0;
                while ($_start+1 < $_result_count) {
                    $_end = 0;
                    $_level = 1;
                    if (($this->mbstring_overload ? mb_substr($_result[0][$_start][0],0,mb_strlen($source->smarty->left_delimiter,'latin1')+1, 'latin1') : substr($_result[0][$_start][0],0,strlen($source->smarty->left_delimiter)+1)) == $source->smarty->left_delimiter.'*') {
                        $_start++;
                        continue;
                    }
                    while ($_level != 0) {
                        $_end++;
                        if (($this->mbstring_overload ? mb_substr($_result[0][$_start + $_end][0],0,mb_strlen($source->smarty->left_delimiter,'latin1')+1, 'latin1') : substr($_result[0][$_start + $_end][0],0,strlen($source->smarty->left_delimiter)+1)) == $source->smarty->left_delimiter.'*') {
                            continue;
                        }
                        if (!strpos($_result[0][$_start + $_end][0], '/')) {
                            $_level++;
                        } else {
                            $_level--;
                        }
                    }
                    $_block_content = str_replace($source->smarty->left_delimiter . '$smarty.block.parent' . $source->smarty->right_delimiter, '%%%%SMARTY_PARENT%%%%',
                    ($this->mbstring_overload ? mb_substr($_content, $_result[0][$_start][1] + mb_strlen($_result[0][$_start][0], 'latin1'), $_result[0][$_start + $_end][1] - $_result[0][$_start][1] - + mb_strlen($_result[0][$_start][0], 'latin1'), 'latin1') : substr($_content, $_result[0][$_start][1] + strlen($_result[0][$_start][0]), $_result[0][$_start + $_end][1] - $_result[0][$_start][1] - + strlen($_result[0][$_start][0]))));
                    Smarty_Internal_Compile_Block::saveBlockData($_block_content, $_result[0][$_start][0], $source->template, $_component->filepath);
                    $_start = $_start + $_end + 1;
                }
            } else {
                return $_content;
            }
        }
    }

    /**
    * Determine basename for compiled filename
    *
    * @param Smarty_Template_Source $source source object
    * @return string resource's basename
    */
    public function getBasename(Smarty_Template_Source $source)
    {
        return str_replace(':', '.', basename($source->filepath));
    }

}

?>