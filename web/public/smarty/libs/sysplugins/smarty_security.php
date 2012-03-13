<?php
/**
 * Smarty plugin
 *
 * @package Smarty
 * @subpackage Security
 * @author Uwe Tews
 */
 
/*
 * FIXME: Smarty_Security API
 *      - getter and setter instead of public properties would allow cultivating an internal cache properly
 *      - current implementation of isTrustedResourceDir() assumes that Smarty::$template_dir and Smarty::$config_dir are immutable
 *        the cache is killed every time either of the variables change. That means that two distinct Smarty objects with differing
 *        $template_dir or $config_dir should NOT share the same Smarty_Security instance, 
 *        as this would lead to (severe) performance penalty! how should this be handled? 
 */

/**
 * This class does contain the security settings
 */
class Smarty_Security {

    /**
     * This determines how Smarty handles "<?php ... ?>" tags in templates.
     * possible values:
     * <ul>
     *   <li>Smarty::PHP_PASSTHRU -> echo PHP tags as they are</li>
     *   <li>Smarty::PHP_QUOTE    -> escape tags as entities</li>
     *   <li>Smarty::PHP_REMOVE   -> remove php tags</li>
     *   <li>Smarty::PHP_ALLOW    -> execute php tags</li>
     * </ul>
     *
     * @var integer
     */
    public $php_handling = Smarty::PHP_PASSTHRU;
    /**
     * This is the list of template directories that are considered secure.
     * $template_dir is in this list implicitly.
     *
     * @var array
     */
    public $secure_dir = array();
    /**
     * This is an array of directories where trusted php scripts reside.
     * {@link $security} is disabled during their inclusion/execution.
     *
     * @var array
     */
    public $trusted_dir = array();
    /**
     * List of regular expressions (PCRE) that include trusted URIs
     *
     * @var array
     */
    public $trusted_uri = array();
    /**
     * This is an array of trusted static classes.
     *
     * If empty access to all static classes is allowed.
     * If set to 'none' none is allowed.
     * @var array
     */
    public $static_classes = array();
    /**
     * This is an array of trusted PHP functions.
     *
     * If empty all functions are allowed.
     * To disable all PHP functions set $php_functions = null.
     * @var array
     */
    public $php_functions = array(
        'isset', 'empty',
        'count', 'sizeof',
        'in_array', 'is_array',
        'time',
        'nl2br',
    );
    /**
     * This is an array of trusted PHP modifers.
     *
     * If empty all modifiers are allowed.
     * To disable all modifier set $modifiers = null.
     * @var array
     */
    public $php_modifiers = array(
        'escape',
        'count'
    );
    /**
     * This is an array of allowed tags.
     *
     * If empty no restriction by allowed_tags.
     * @var array
     */
    public $allowed_tags = array();
    /**
     * This is an array of disabled tags.
     *
     * If empty no restriction by disabled_tags.
     * @var array
     */
    public $disabled_tags = array();
    /**
     * This is an array of allowed modifier plugins.
     *
     * If empty no restriction by allowed_modifiers.
     * @var array
     */
    public $allowed_modifiers = array();
    /**
     * This is an array of disabled modifier plugins.
     *
     * If empty no restriction by disabled_modifiers.
     * @var array
     */
    public $disabled_modifiers = array();
    /**
     * This is an array of trusted streams.
     *
     * If empty all streams are allowed.
     * To disable all streams set $streams = null.
     * @var array
     */
    public $streams = array('file');
    /**
     * + flag if constants can be accessed from template
     * @var boolean
     */
    public $allow_constants = true;
    /**
     * + flag if super globals can be accessed from template
     * @var boolean
     */
    public $allow_super_globals = true;

    /**
     * Cache for $resource_dir lookups
     * @var array
     */
    protected $_resource_dir = null;
    /**
     * Cache for $template_dir lookups
     * @var array
     */
    protected $_template_dir = null;
    /**
     * Cache for $config_dir lookups
     * @var array
     */
    protected $_config_dir = null;
    /**
     * Cache for $secure_dir lookups
     * @var array
     */
    protected $_secure_dir = null;
    /**
     * Cache for $php_resource_dir lookups
     * @var array
     */
    protected $_php_resource_dir = null;
    /**
     * Cache for $trusted_dir lookups
     * @var array
     */
    protected $_trusted_dir = null;
    
    
    /**
     * @param Smarty $smarty
     */
    public function __construct($smarty)
    {
        $this->smarty = $smarty;
    }
    
    /**
     * Check if PHP function is trusted.
     *
     * @param string $function_name
     * @param object $compiler compiler object
     * @return boolean true if function is trusted
     * @throws SmartyCompilerException if php function is not trusted
     */
    public function isTrustedPhpFunction($function_name, $compiler)
    {
        if (isset($this->php_functions) && (empty($this->php_functions) || in_array($function_name, $this->php_functions))) {
            return true;
        }

        $compiler->trigger_template_error("PHP function '{$function_name}' not allowed by security setting");
        return false; // should not, but who knows what happens to the compiler in the future?
    }

    /**
     * Check if static class is trusted.
     *
     * @param string $class_name
     * @param object $compiler compiler object
     * @return boolean true if class is trusted
     * @throws SmartyCompilerException if static class is not trusted
     */
    public function isTrustedStaticClass($class_name, $compiler)
    {
        if (isset($this->static_classes) && (empty($this->static_classes) || in_array($class_name, $this->static_classes))) {
            return true;
        }

        $compiler->trigger_template_error("access to static class '{$class_name}' not allowed by security setting");
        return false; // should not, but who knows what happens to the compiler in the future?
    }

    /**
     * Check if PHP modifier is trusted.
     *
     * @param string $modifier_name
     * @param object $compiler compiler object
     * @return boolean true if modifier is trusted
     * @throws SmartyCompilerException if modifier is not trusted
     */
    public function isTrustedPhpModifier($modifier_name, $compiler)
    {
        if (isset($this->php_modifiers) && (empty($this->php_modifiers) || in_array($modifier_name, $this->php_modifiers))) {
            return true;
        }

        $compiler->trigger_template_error("modifier '{$modifier_name}' not allowed by security setting");
        return false; // should not, but who knows what happens to the compiler in the future?
    }

    /**
     * Check if tag is trusted.
     *
     * @param string $tag_name
     * @param object $compiler compiler object
     * @return boolean true if tag is trusted
     * @throws SmartyCompilerException if modifier is not trusted
     */
    public function isTrustedTag($tag_name, $compiler)
    {
        // check for internal always required tags
        if (in_array($tag_name, array('assign', 'call', 'private_filter', 'private_block_plugin', 'private_function_plugin', 'private_object_block_function',
                    'private_object_function', 'private_registered_function', 'private_registered_block', 'private_special_variable', 'private_print_expression', 'private_modifier'))) {
            return true;
        }
        // check security settings
        if (empty($this->allowed_tags)) {
            if (empty($this->disabled_tags) || !in_array($tag_name, $this->disabled_tags)) {
                return true;
            } else {
                $compiler->trigger_template_error("tag '{$tag_name}' disabled by security setting", $compiler->lex->taglineno);
            }
        } else if (in_array($tag_name, $this->allowed_tags) && !in_array($tag_name, $this->disabled_tags)) {
            return true;
        } else {
            $compiler->trigger_template_error("tag '{$tag_name}' not allowed by security setting", $compiler->lex->taglineno);
        }
        return false; // should not, but who knows what happens to the compiler in the future?
    }

    /**
     * Check if modifier plugin is trusted.
     *
     * @param string $modifier_name
     * @param object $compiler compiler object
     * @return boolean true if tag is trusted
     * @throws SmartyCompilerException if modifier is not trusted
     */
    public function isTrustedModifier($modifier_name, $compiler)
    {
        // check for internal always allowed modifier
        if (in_array($modifier_name, array('default'))) {
            return true;
        }
        // check security settings
        if (empty($this->allowed_modifiers)) {
            if (empty($this->disabled_modifiers) || !in_array($modifier_name, $this->disabled_modifiers)) {
                return true;
            } else {
                $compiler->trigger_template_error("modifier '{$modifier_name}' disabled by security setting", $compiler->lex->taglineno);
            }
        } else if (in_array($modifier_name, $this->allowed_modifiers) && !in_array($modifier_name, $this->disabled_modifiers)) {
            return true;
        } else {
            $compiler->trigger_template_error("modifier '{$modifier_name}' not allowed by security setting", $compiler->lex->taglineno);
        }
        return false; // should not, but who knows what happens to the compiler in the future?
    }

    /**
     * Check if stream is trusted.
     *
     * @param string $stream_name
     * @return boolean true if stream is trusted
     * @throws SmartyException if stream is not trusted
     */
    public function isTrustedStream($stream_name)
    {
        if (isset($this->streams) && (empty($this->streams) || in_array($stream_name, $this->streams))) {
            return true;
        }

        throw new SmartyException("stream '{$stream_name}' not allowed by security setting");
    }

    /**
     * Check if directory of file resource is trusted.
     *
     * @param string $filepath
     * @return boolean true if directory is trusted
     * @throws SmartyException if directory is not trusted
     */
    public function isTrustedResourceDir($filepath)
    {
        $_template = false;
        $_config = false;
        $_secure = false;

        $_template_dir = $this->smarty->getTemplateDir();
        $_config_dir = $this->smarty->getConfigDir();

        // check if index is outdated
        if ((!$this->_template_dir || $this->_template_dir !== $_template_dir)
                || (!$this->_config_dir || $this->_config_dir !== $_config_dir)
                || (!empty($this->secure_dir) && (!$this->_secure_dir || $this->_secure_dir !== $this->secure_dir))
        ) {
            $this->_resource_dir = array();
            $_template = true;
            $_config = true;
            $_secure = !empty($this->secure_dir);
        }

        // rebuild template dir index
        if ($_template) {
            $this->_template_dir = $_template_dir;
            foreach ($_template_dir as $directory) {
                $directory = realpath($directory);
                $this->_resource_dir[$directory] = true;
            }
        }

        // rebuild config dir index
        if ($_config) {
            $this->_config_dir = $_config_dir;
            foreach ($_config_dir as $directory) {
                $directory = realpath($directory);
                $this->_resource_dir[$directory] = true;
            }
        }

        // rebuild secure dir index
        if ($_secure) {
            $this->_secure_dir = $this->secure_dir;
            foreach ((array) $this->secure_dir as $directory) {
                $directory = realpath($directory);
                $this->_resource_dir[$directory] = true;
            }
        }

        $_filepath = realpath($filepath);
        $directory = dirname($_filepath);
        $_directory = array();
        while (true) {
            // remember the directory to add it to _resource_dir in case we're successful
            $_directory[$directory] = true;
            // test if the directory is trusted
            if (isset($this->_resource_dir[$directory])) {
                // merge sub directories of current $directory into _resource_dir to speed up subsequent lookups
                $this->_resource_dir = array_merge($this->_resource_dir, $_directory);
                return true;
            }
            // abort if we've reached root
            if (($pos = strrpos($directory, DS)) === false || !isset($directory[1])) {
                break;
            }
            // bubble up one level
            $directory = substr($directory, 0, $pos);
        }

        // give up
        throw new SmartyException("directory '{$_filepath}' not allowed by security setting");
    }
    
    /**
     * Check if URI (e.g. {fetch} or {html_image}) is trusted
     *
     * To simplify things, isTrustedUri() resolves all input to "{$PROTOCOL}://{$HOSTNAME}".
     * So "http://username:password@hello.world.example.org:8080/some-path?some=query-string"
     * is reduced to "http://hello.world.example.org" prior to applying the patters from {@link $trusted_uri}.
     * @param string $uri 
     * @return boolean true if URI is trusted
     * @throws SmartyException if URI is not trusted
     * @uses $trusted_uri for list of patterns to match against $uri
     */
    public function isTrustedUri($uri)
    {
        $_uri = parse_url($uri);
        if (!empty($_uri['scheme']) && !empty($_uri['host'])) {
            $_uri = $_uri['scheme'] . '://' . $_uri['host'];
            foreach ($this->trusted_uri as $pattern) {
                if (preg_match($pattern, $_uri)) {
                    return true;
                }
            }
        }
        
        throw new SmartyException("URI '{$uri}' not allowed by security setting");
    }
    
    /**
     * Check if directory of file resource is trusted.
     *
     * @param string $filepath
     * @return boolean true if directory is trusted
     * @throws SmartyException if PHP directory is not trusted
     */
    public function isTrustedPHPDir($filepath)
    {
        if (empty($this->trusted_dir)) {
            throw new SmartyException("directory '{$filepath}' not allowed by security setting (no trusted_dir specified)");
        }

        // check if index is outdated
        if (!$this->_trusted_dir || $this->_trusted_dir !== $this->trusted_dir) {
            $this->_php_resource_dir = array();

            $this->_trusted_dir = $this->trusted_dir;
            foreach ((array) $this->trusted_dir as $directory) {
                $directory = realpath($directory);
                $this->_php_resource_dir[$directory] = true;
            }
        }

        $_filepath = realpath($filepath);
        $directory = dirname($_filepath);
        $_directory = array();
        while (true) {
            // remember the directory to add it to _resource_dir in case we're successful
            $_directory[] = $directory;
            // test if the directory is trusted
            if (isset($this->_php_resource_dir[$directory])) {
                // merge sub directories of current $directory into _resource_dir to speed up subsequent lookups
                $this->_php_resource_dir = array_merge($this->_php_resource_dir, $_directory);
                return true;
            }
            // abort if we've reached root
            if (($pos = strrpos($directory, DS)) === false || !isset($directory[2])) {
                break;
            }
            // bubble up one level
            $directory = substr($directory, 0, $pos);
        }

        throw new SmartyException("directory '{$_filepath}' not allowed by security setting");
    }

}

?>