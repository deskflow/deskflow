<?php
/*
   Copyright (c) 2003, 2009 Danilo Segan <danilo@kvota.net>.
   Copyright (c) 2005 Nico Kaiser <nico@siriux.net>

   This file is part of PHP-gettext.

   PHP-gettext is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   PHP-gettext is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with PHP-gettext; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/**
 * Provides a simple gettext replacement that works independently from
 * the system's gettext abilities.
 * It can read MO files and use them for translating strings.
 * The files are passed to gettext_reader as a Stream (see streams.php)
 *
 * This version has the ability to cache all strings and translations to
 * speed up the string lookup.
 * While the cache is enabled by default, it can be switched off with the
 * second parameter in the constructor (e.g. whenusing very large MO files
 * that you don't want to keep in memory)
 */
class gettext_reader {
  //public:
   var $error = 0; // public variable that holds error code (0 if no error)

   //private:
  var $BYTEORDER = 0;        // 0: low endian, 1: big endian
  var $STREAM = NULL;
  var $short_circuit = false;
  var $enable_cache = false;
  var $originals = NULL;      // offset of original table
  var $translations = NULL;    // offset of translation table
  var $pluralheader = NULL;    // cache header field for plural forms
  var $total = 0;          // total string count
  var $table_originals = NULL;  // table for original strings (offsets)
  var $table_translations = NULL;  // table for translated strings (offsets)
  var $cache_translations = NULL;  // original -> translation mapping


  /* Methods */


  /**
   * Reads a 32bit Integer from the Stream
   *
   * @access private
   * @return Integer from the Stream
   */
  function readint() {
      if ($this->BYTEORDER == 0) {
        // low endian
        $input=unpack('V', $this->STREAM->read(4));
        return array_shift($input);
      } else {
        // big endian
        $input=unpack('N', $this->STREAM->read(4));
        return array_shift($input);
      }
    }

  function read($bytes) {
    return $this->STREAM->read($bytes);
  }

  /**
   * Reads an array of Integers from the Stream
   *
   * @param int count How many elements should be read
   * @return Array of Integers
   */
  function readintarray($count) {
    if ($this->BYTEORDER == 0) {
        // low endian
        return unpack('V'.$count, $this->STREAM->read(4 * $count));
      } else {
        // big endian
        return unpack('N'.$count, $this->STREAM->read(4 * $count));
      }
  }

  /**
   * Constructor
   *
   * @param object Reader the StreamReader object
   * @param boolean enable_cache Enable or disable caching of strings (default on)
   */
  function gettext_reader($Reader, $enable_cache = true) {
    // If there isn't a StreamReader, turn on short circuit mode.
    if (! $Reader || isset($Reader->error) ) {
      $this->short_circuit = true;
      return;
    }

    // Caching can be turned off
    $this->enable_cache = $enable_cache;

    $MAGIC1 = "\x95\x04\x12\xde";
    $MAGIC2 = "\xde\x12\x04\x95";

    $this->STREAM = $Reader;
    $magic = $this->read(4);
    if ($magic == $MAGIC1) {
      $this->BYTEORDER = 1;
    } elseif ($magic == $MAGIC2) {
      $this->BYTEORDER = 0;
    } else {
      $this->error = 1; // not MO file
      return false;
    }

    // FIXME: Do we care about revision? We should.
    $revision = $this->readint();

    $this->total = $this->readint();
    $this->originals = $this->readint();
    $this->translations = $this->readint();
  }

  /**
   * Loads the translation tables from the MO file into the cache
   * If caching is enabled, also loads all strings into a cache
   * to speed up translation lookups
   *
   * @access private
   */
  function load_tables() {
    if (is_array($this->cache_translations) &&
      is_array($this->table_originals) &&
      is_array($this->table_translations))
      return;

    /* get original and translations tables */
    if (!is_array($this->table_originals)) {
      $this->STREAM->seekto($this->originals);
      $this->table_originals = $this->readintarray($this->total * 2);
    }
    if (!is_array($this->table_translations)) {
      $this->STREAM->seekto($this->translations);
      $this->table_translations = $this->readintarray($this->total * 2);
    }

    if ($this->enable_cache) {
      $this->cache_translations = array ();
      /* read all strings in the cache */
      for ($i = 0; $i < $this->total; $i++) {
        $this->STREAM->seekto($this->table_originals[$i * 2 + 2]);
        $original = $this->STREAM->read($this->table_originals[$i * 2 + 1]);
        $this->STREAM->seekto($this->table_translations[$i * 2 + 2]);
        $translation = $this->STREAM->read($this->table_translations[$i * 2 + 1]);
        $this->cache_translations[$original] = $translation;
      }
    }
  }

  /**
   * Returns a string from the "originals" table
   *
   * @access private
   * @param int num Offset number of original string
   * @return string Requested string if found, otherwise ''
   */
  function get_original_string($num) {
    $length = $this->table_originals[$num * 2 + 1];
    $offset = $this->table_originals[$num * 2 + 2];
    if (! $length)
      return '';
    $this->STREAM->seekto($offset);
    $data = $this->STREAM->read($length);
    return (string)$data;
  }

  /**
   * Returns a string from the "translations" table
   *
   * @access private
   * @param int num Offset number of original string
   * @return string Requested string if found, otherwise ''
   */
  function get_translation_string($num) {
    $length = $this->table_translations[$num * 2 + 1];
    $offset = $this->table_translations[$num * 2 + 2];
    if (! $length)
      return '';
    $this->STREAM->seekto($offset);
    $data = $this->STREAM->read($length);
    return (string)$data;
  }

  /**
   * Binary search for string
   *
   * @access private
   * @param string string
   * @param int start (internally used in recursive function)
   * @param int end (internally used in recursive function)
   * @return int string number (offset in originals table)
   */
  function find_string($string, $start = -1, $end = -1) {
    if (($start == -1) or ($end == -1)) {
      // find_string is called with only one parameter, set start end end
      $start = 0;
      $end = $this->total;
    }
    if (abs($start - $end) <= 1) {
      // We're done, now we either found the string, or it doesn't exist
      $txt = $this->get_original_string($start);
      if ($string == $txt)
        return $start;
      else
        return -1;
    } else if ($start > $end) {
      // start > end -> turn around and start over
      return $this->find_string($string, $end, $start);
    } else {
      // Divide table in two parts
      $half = (int)(($start + $end) / 2);
      $cmp = strcmp($string, $this->get_original_string($half));
      if ($cmp == 0)
        // string is exactly in the middle => return it
        return $half;
      else if ($cmp < 0)
        // The string is in the upper half
        return $this->find_string($string, $start, $half);
      else
        // The string is in the lower half
        return $this->find_string($string, $half, $end);
    }
  }

  /**
   * Translates a string
   *
   * @access public
   * @param string string to be translated
   * @return string translated string (or original, if not found)
   */
  function translate($string) {
    if ($this->short_circuit)
      return $string;
    $this->load_tables();

    if ($this->enable_cache) {
      // Caching enabled, get translated string from cache
      if (array_key_exists($string, $this->cache_translations))
        return $this->cache_translations[$string];
      else
        return $string;
    } else {
      // Caching not enabled, try to find string
      $num = $this->find_string($string);
      if ($num == -1)
        return $string;
      else
        return $this->get_translation_string($num);
    }
  }

  /**
   * Sanitize plural form expression for use in PHP eval call.
   *
   * @access private
   * @return string sanitized plural form expression
   */
  function sanitize_plural_expression($expr) {
    // Get rid of disallowed characters.
    $expr = preg_replace('@[^a-zA-Z0-9_:;\(\)\?\|\&=!<>+*/\%-]@', '', $expr);

    // Add parenthesis for tertiary '?' operator.
    $expr .= ';';
    $res = '';
    $p = 0;
    for ($i = 0; $i < strlen($expr); $i++) {
      $ch = $expr[$i];
      switch ($ch) {
      case '?':
        $res .= ' ? (';
        $p++;
        break;
      case ':':
        $res .= ') : (';
        break;
      case ';':
        $res .= str_repeat( ')', $p) . ';';
        $p = 0;
        break;
      default:
        $res .= $ch;
      }
    }
    return $res;
  }

  /**
   * Parse full PO header and extract only plural forms line.
   *
   * @access private
   * @return string verbatim plural form header field
   */
  function extract_plural_forms_header_from_po_header($header) {
    if (preg_match("/(^|\n)plural-forms: ([^\n]*)\n/i", $header, $regs))
      $expr = $regs[2];
    else
      $expr = "nplurals=2; plural=n == 1 ? 0 : 1;";
    return $expr;
  }

  /**
   * Get possible plural forms from MO header
   *
   * @access private
   * @return string plural form header
   */
  function get_plural_forms() {
    // lets assume message number 0 is header
    // this is true, right?
    $this->load_tables();

    // cache header field for plural forms
    if (! is_string($this->pluralheader)) {
      if ($this->enable_cache) {
        $header = $this->cache_translations[""];
      } else {
        $header = $this->get_translation_string(0);
      }
      $expr = $this->extract_plural_forms_header_from_po_header($header);
      $this->pluralheader = $this->sanitize_plural_expression($expr);
    }
    return $this->pluralheader;
  }

  /**
   * Detects which plural form to take
   *
   * @access private
   * @param n count
   * @return int array index of the right plural form
   */
  function select_string($n) {
    $string = $this->get_plural_forms();
    $string = str_replace('nplurals',"\$total",$string);
    $string = str_replace("n",$n,$string);
    $string = str_replace('plural',"\$plural",$string);

    $total = 0;
    $plural = 0;

    eval("$string");
    if ($plural >= $total) $plural = $total - 1;
    return $plural;
  }

  /**
   * Plural version of gettext
   *
   * @access public
   * @param string single
   * @param string plural
   * @param string number
   * @return translated plural form
   */
  function ngettext($single, $plural, $number) {
    if ($this->short_circuit) {
      if ($number != 1)
        return $plural;
      else
        return $single;
    }

    // find out the appropriate form
    $select = $this->select_string($number);

    // this should contains all strings separated by NULLs
    $key = $single . chr(0) . $plural;


    if ($this->enable_cache) {
      if (! array_key_exists($key, $this->cache_translations)) {
        return ($number != 1) ? $plural : $single;
      } else {
        $result = $this->cache_translations[$key];
        $list = explode(chr(0), $result);
        return $list[$select];
      }
    } else {
      $num = $this->find_string($key);
      if ($num == -1) {
        return ($number != 1) ? $plural : $single;
      } else {
        $result = $this->get_translation_string($num);
        $list = explode(chr(0), $result);
        return $list[$select];
      }
    }
  }

  function pgettext($context, $msgid) {
    $key = $context . chr(4) . $msgid;
    $ret = $this->translate($key);
    if (strpos($ret, "\004") !== FALSE) {
      return $msgid;
    } else {
      return $ret;
    }
  }

  function npgettext($context, $singular, $plural, $number) {
    $key = $context . chr(4) . $singular;
    $ret = $this->ngettext($key, $plural, $number);
    if (strpos($ret, "\004") !== FALSE) {
      return $singular;
    } else {
      return $ret;
    }

  }
}

?>
