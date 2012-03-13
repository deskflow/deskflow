<?php
/**
 * Smarty Internal Plugin Resource Stream
 *
 * Implements the streams as resource for Smarty template
 *
 * @package Smarty
 * @subpackage TemplateResources
 * @author Uwe Tews
 * @author Rodney Rehm
 */

/**
 * Smarty Internal Plugin Resource Stream
 *
 * Implements the streams as resource for Smarty template
 *
 * @link http://php.net/streams
 * @package Smarty
 * @subpackage TemplateResources
 */
class Smarty_Internal_Resource_Stream extends Smarty_Resource_Recompiled {

    /**
     * populate Source Object with meta data from Resource
     *
     * @param Smarty_Template_Source   $source    source object
     * @param Smarty_Internal_Template $_template template object
     * @return void
     */
    public function populate(Smarty_Template_Source $source, Smarty_Internal_Template $_template=null)
    {
        $source->filepath = str_replace(':', '://', $source->resource);
        $source->uid = false;
        $source->content = $this->getContent($source);
        $source->timestamp = false;
        $source->exists = !!$source->content;
    }

    /**
     * Load template's source from stream into current template object
     *
     * @param Smarty_Template_Source $source source object
     * @return string template source
     * @throws SmartyException if source cannot be loaded
     */
    public function getContent(Smarty_Template_Source $source)
    {
        $t = '';
        // the availability of the stream has already been checked in Smarty_Resource::fetch()
        $fp = fopen($source->filepath, 'r+');
        if ($fp) {
            while (!feof($fp) && ($current_line = fgets($fp)) !== false) {
                $t .= $current_line;
            }
            fclose($fp);
            return $t;
        } else {
            return false;
        }
    }
    
    /**
     * modify resource_name according to resource handlers specifications
     *
     * @param Smarty $smarty        Smarty instance
     * @param string $resource_name resource_name to make unique
     * @return string unique resource name
     */
    protected function buildUniqueResourceName(Smarty $smarty, $resource_name)
    {
        return get_class($this) . '#' . $resource_name;
    }
}

?>