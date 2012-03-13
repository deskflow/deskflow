<?php
/**
 * Smarty Internal Plugin
 *
 * @package Smarty
 * @subpackage Cacher
 */

/**
 * Smarty Cache Handler Base for Key/Value Storage Implementations
 *
 * This class implements the functionality required to use simple key/value stores
 * for hierarchical cache groups. key/value stores like memcache or APC do not support
 * wildcards in keys, therefore a cache group cannot be cleared like "a|*" - which
 * is no problem to filesystem and RDBMS implementations.
 *
 * This implementation is based on the concept of invalidation. While one specific cache
 * can be identified and cleared, any range of caches cannot be identified. For this reason
 * each level of the cache group hierarchy can have its own value in the store. These values
 * are nothing but microtimes, telling us when a particular cache group was cleared for the
 * last time. These keys are evaluated for every cache read to determine if the cache has
 * been invalidated since it was created and should hence be treated as inexistent.
 *
 * Although deep hierarchies are possible, they are not recommended. Try to keep your
 * cache groups as shallow as possible. Anything up 3-5 parents should be ok. So
 * »a|b|c« is a good depth where »a|b|c|d|e|f|g|h|i|j|k« isn't. Try to join correlating
 * cache groups: if your cache groups look somewhat like »a|b|$page|$items|$whatever«
 * consider using »a|b|c|$page-$items-$whatever« instead.
 *
 * @package Smarty
 * @subpackage Cacher
 * @author Rodney Rehm
 */
abstract class Smarty_CacheResource_KeyValueStore extends Smarty_CacheResource {

    /**
     * cache for contents
     * @var array
     */
    protected $contents = array();
    /**
     * cache for timestamps
     * @var array
     */
    protected $timestamps = array();

    /**
     * populate Cached Object with meta data from Resource
     *
     * @param Smarty_Template_Cached   $cached    cached object
     * @param Smarty_Internal_Template $_template template object
     * @return void
     */
    public function populate(Smarty_Template_Cached $cached, Smarty_Internal_Template $_template)
    {
        $cached->filepath = $_template->source->uid
                . '#' . $this->sanitize($cached->source->name)
                . '#' . $this->sanitize($cached->cache_id)
                . '#' . $this->sanitize($cached->compile_id);

        $this->populateTimestamp($cached);
    }

    /**
     * populate Cached Object with timestamp and exists from Resource
     *
     * @param Smarty_Template_Cached $cached cached object
     * @return void
     */
    public function populateTimestamp(Smarty_Template_Cached $cached)
    {
        if (!$this->fetch($cached->filepath, $cached->source->name, $cached->cache_id, $cached->compile_id, $content, $timestamp, $cached->source->uid)) {
            return;
        }
        $cached->content = $content;
        $cached->timestamp = (int) $timestamp;
        $cached->exists = $cached->timestamp;
    }

    /**
     * Read the cached template and process the header
     *
     * @param Smarty_Internal_Template $_template template object
     * @param Smarty_Template_Cached $cached cached object
     * @return booelan true or false if the cached content does not exist
     */
    public function process(Smarty_Internal_Template $_template, Smarty_Template_Cached $cached=null)
    {
        if (!$cached) {
            $cached = $_template->cached;
        }
        $content = $cached->content ? $cached->content : null;
        $timestamp = $cached->timestamp ? $cached->timestamp : null;
        if ($content === null || !$timestamp) {
            if (!$this->fetch($_template->cached->filepath, $_template->source->name, $_template->cache_id, $_template->compile_id, $content, $timestamp, $_template->source->uid)) {
                return false;
            }
        }
        if (isset($content)) {
            $_smarty_tpl = $_template;
            eval("?>" . $content);
            return true;
        }
        return false;
    }

    /**
     * Write the rendered template output to cache
     *
     * @param Smarty_Internal_Template $_template template object
     * @param string $content content to cache
     * @return boolean success
     */
    public function writeCachedContent(Smarty_Internal_Template $_template, $content)
    {
        $this->addMetaTimestamp($content);
        return $this->write(array($_template->cached->filepath => $content), $_template->properties['cache_lifetime']);
    }

    /**
     * Empty cache
     *
     * {@internal the $exp_time argument is ignored altogether }}
     *
     * @param Smarty  $smarty   Smarty object
     * @param integer $exp_time expiration time [being ignored]
     * @return integer number of cache files deleted [always -1]
     * @uses purge() to clear the whole store
     * @uses invalidate() to mark everything outdated if purge() is inapplicable
     */
    public function clearAll(Smarty $smarty, $exp_time=null)
    {
        if (!$this->purge()) {
            $this->invalidate(null);
        }
        return -1;
    }

    /**
     * Empty cache for a specific template
     *
     * {@internal the $exp_time argument is ignored altogether}}
     *
     * @param Smarty  $smarty        Smarty object
     * @param string  $resource_name template name
     * @param string  $cache_id      cache id
     * @param string  $compile_id    compile id
     * @param integer $exp_time      expiration time [being ignored]
     * @return integer number of cache files deleted [always -1]
     * @uses buildCachedFilepath() to generate the CacheID
     * @uses invalidate() to mark CacheIDs parent chain as outdated
     * @uses delete() to remove CacheID from cache
     */
    public function clear(Smarty $smarty, $resource_name, $cache_id, $compile_id, $exp_time)
    {
        $uid = $this->getTemplateUid($smarty, $resource_name, $cache_id, $compile_id);
        $cid = $uid . '#' . $this->sanitize($resource_name) . '#' . $this->sanitize($cache_id) . '#' . $this->sanitize($compile_id);
        $this->delete(array($cid));
        $this->invalidate($cid, $resource_name, $cache_id, $compile_id, $uid);
        return -1;
    }
    /**
     * Get template's unique ID
     *
     * @param Smarty $smarty        Smarty object
     * @param string $resource_name template name
     * @param string $cache_id      cache id
     * @param string $compile_id    compile id
     * @return string filepath of cache file
     */
    protected function getTemplateUid(Smarty $smarty, $resource_name, $cache_id, $compile_id)
    {
        $uid = '';
        if (isset($resource_name)) {
            $tpl = new $smarty->template_class($resource_name, $smarty);
            if ($tpl->source->exists) {
                $uid = $tpl->source->uid;
            }
            
            // remove from template cache
            if ($smarty->allow_ambiguous_resources) {
                $_templateId = $tpl->source->unique_resource . $tpl->cache_id . $tpl->compile_id;
            } else {
                $_templateId = $smarty->joined_template_dir . '#' . $resource_name . $tpl->cache_id . $tpl->compile_id;
            }
            if (isset($_templateId[150])) {
                $_templateId = sha1($_templateId);
            }
            unset($smarty->template_objects[$_templateId]);
        }
        return $uid;
    }

    /**
     * Sanitize CacheID components
     *
     * @param string $string CacheID component to sanitize
     * @return string sanitized CacheID component
     */
    protected function sanitize($string)
    {
        // some poeple smoke bad weed
        $string = trim($string, '|');
        if (!$string) {
            return null;
        }
        return preg_replace('#[^\w\|]+#S', '_', $string);
    }

    /**
     * Fetch and prepare a cache object.
     *
     * @param string  $cid           CacheID to fetch
     * @param string  $resource_name template name
     * @param string  $cache_id      cache id
     * @param string  $compile_id    compile id
     * @param string  $content       cached content
     * @param integer &$timestamp    cached timestamp (epoch)
     * @param string  $resource_uid  resource's uid
     * @return boolean success
     */
    protected function fetch($cid, $resource_name = null, $cache_id = null, $compile_id = null, &$content = null, &$timestamp = null, $resource_uid = null)
    {
        $t = $this->read(array($cid));
        $content = !empty($t[$cid]) ? $t[$cid] : null;
        $timestamp = null;

        if ($content && ($timestamp = $this->getMetaTimestamp($content))) {
            $invalidated = $this->getLatestInvalidationTimestamp($cid, $resource_name, $cache_id, $compile_id, $resource_uid);
            if ($invalidated > $timestamp) {
                $timestamp = null;
                $content = null;
            }
        }

        return !!$content;
    }

    /**
     * Add current microtime to the beginning of $cache_content
     *
     * {@internal the header uses 8 Bytes, the first 4 Bytes are the seconds, the second 4 Bytes are the microseconds}}
     *
     * @param string &$content the content to be cached
     */
    protected function addMetaTimestamp(&$content)
    {
        $mt = explode(" ", microtime());
        $ts = pack("NN", $mt[1], (int) ($mt[0] * 100000000));
        $content = $ts . $content;
    }

    /**
     * Extract the timestamp the $content was cached
     *
     * @param string &$content the cached content
     * @return float the microtime the content was cached
     */
    protected function getMetaTimestamp(&$content)
    {
        $s = unpack("N", substr($content, 0, 4));
        $m = unpack("N", substr($content, 4, 4));
        $content = substr($content, 8);
        return $s[1] + ($m[1] / 100000000);
    }

    /**
     * Invalidate CacheID
     *
     * @param string $cid           CacheID
     * @param string $resource_name template name
     * @param string $cache_id      cache id
     * @param string $compile_id    compile id
     * @param string $resource_uid  source's uid
     * @return void
     */
    protected function invalidate($cid = null, $resource_name = null, $cache_id = null, $compile_id = null, $resource_uid = null)
    {
        $now = microtime(true);
        $key = null;
        // invalidate everything
        if (!$resource_name && !$cache_id && !$compile_id) {
            $key = 'IVK#ALL';
        }
        // invalidate all caches by template
        else if ($resource_name && !$cache_id && !$compile_id) {
            $key = 'IVK#TEMPLATE#' . $resource_uid . '#' . $this->sanitize($resource_name);
        }
        // invalidate all caches by cache group
        else if (!$resource_name && $cache_id && !$compile_id) {
            $key = 'IVK#CACHE#' . $this->sanitize($cache_id);
        }
        // invalidate all caches by compile id
        else if (!$resource_name && !$cache_id && $compile_id) {
            $key = 'IVK#COMPILE#' . $this->sanitize($compile_id);
        }
        // invalidate by combination
        else {
            $key = 'IVK#CID#' . $cid;
        }
        $this->write(array($key => $now));
    }

    /**
     * Determine the latest timestamp known to the invalidation chain
     *
     * @param string $cid           CacheID to determine latest invalidation timestamp of
     * @param string $resource_name template name
     * @param string $cache_id      cache id
     * @param string $compile_id    compile id
     * @param string $resource_uid  source's filepath
     * @return float the microtime the CacheID was invalidated
     */
    protected function getLatestInvalidationTimestamp($cid, $resource_name = null, $cache_id = null, $compile_id = null, $resource_uid = null)
    {
        // abort if there is no CacheID
        if (false && !$cid) {
            return 0;
        }
        // abort if there are no InvalidationKeys to check
        if (!($_cid = $this->listInvalidationKeys($cid, $resource_name, $cache_id, $compile_id, $resource_uid))) {
            return 0;
        }
        
        // there are no InValidationKeys
        if (!($values = $this->read($_cid))) {
            return 0;
        }
        // make sure we're dealing with floats
        $values = array_map('floatval', $values);
        return max($values);
    }

    /**
     * Translate a CacheID into the list of applicable InvalidationKeys.
     *
     * Splits "some|chain|into|an|array" into array( '#clearAll#', 'some', 'some|chain', 'some|chain|into', ... )
     *
     * @param string $cid           CacheID to translate
     * @param string $resource_name template name
     * @param string $cache_id      cache id
     * @param string $compile_id    compile id
     * @param string $resource_uid  source's filepath
     * @return array list of InvalidationKeys
     * @uses $invalidationKeyPrefix to prepend to each InvalidationKey
     */
    protected function listInvalidationKeys($cid, $resource_name = null, $cache_id = null, $compile_id = null, $resource_uid = null)
    {
        $t = array('IVK#ALL');
        $_name = $_compile = '#';
        if ($resource_name) {
            $_name .= $resource_uid . '#' . $this->sanitize($resource_name);
            $t[] = 'IVK#TEMPLATE' . $_name;
        }
        if ($compile_id) {
            $_compile .= $this->sanitize($compile_id);
            $t[] = 'IVK#COMPILE' . $_compile;
        }
        $_name .= '#';
        // some poeple smoke bad weed
        $cid = trim($cache_id, '|');
        if (!$cid) {
            return $t;
        }
        $i = 0;
        while (true) {
            // determine next delimiter position
            $i = strpos($cid, '|', $i);
            // add complete CacheID if there are no more delimiters
            if ($i === false) {
                $t[] = 'IVK#CACHE#' . $cid;
                $t[] = 'IVK#CID' . $_name . $cid . $_compile;
                $t[] = 'IVK#CID' . $_name . $_compile;
                break;
            }
            $part = substr($cid, 0, $i);
            // add slice to list
            $t[] = 'IVK#CACHE#' . $part;
            $t[] = 'IVK#CID' . $_name . $part . $_compile;
            // skip past delimiter position
            $i++;
        }
        return $t;
    }

    /**
     * Check is cache is locked for this template
     *
     * @param Smarty $smarty Smarty object
     * @param Smarty_Template_Cached $cached cached object
     * @return booelan true or false if cache is locked
     */
    public function hasLock(Smarty $smarty, Smarty_Template_Cached $cached)
    {
        $key = 'LOCK#' . $cached->filepath;
        $data = $this->read(array($key));
        return $data && time() - $data[$key] < $smarty->locking_timeout;
    }

    /**
     * Lock cache for this template
     *
     * @param Smarty $smarty Smarty object
     * @param Smarty_Template_Cached $cached cached object
     */
    public function acquireLock(Smarty $smarty, Smarty_Template_Cached $cached)
    {
        $cached->is_locked = true;
        $key = 'LOCK#' . $cached->filepath;
        $this->write(array($key => time()), $smarty->locking_timeout);
    }

    /**
     * Unlock cache for this template
     *
     * @param Smarty $smarty Smarty object
     * @param Smarty_Template_Cached $cached cached object
     */
    public function releaseLock(Smarty $smarty, Smarty_Template_Cached $cached)
    {
        $cached->is_locked = false;
        $key = 'LOCK#' . $cached->filepath;
        $this->delete(array($key));
    }

    /**
     * Read values for a set of keys from cache
     *
     * @param array $keys list of keys to fetch
     * @return array list of values with the given keys used as indexes
     */
    protected abstract function read(array $keys);

    /**
     * Save values for a set of keys to cache
     *
     * @param array $keys   list of values to save
     * @param int   $expire expiration time
     * @return boolean true on success, false on failure
     */
    protected abstract function write(array $keys, $expire=null);

    /**
     * Remove values from cache
     *
     * @param array $keys list of keys to delete
     * @return boolean true on success, false on failure
     */
    protected abstract function delete(array $keys);

    /**
     * Remove *all* values from cache
     *
     * @return boolean true on success, false on failure
     */
    protected function purge()
    {
        return false;
    }

}

?>