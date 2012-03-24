<?php
/**
 * smarty-T_.php - T_ support for smarty
 *
 * ------------------------------------------------------------------------- *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 2.1 of the License, or (at your option) any later version.        *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Lesser General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public          *
 * License along with this library; if not, write to the Free Software       *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 * ------------------------------------------------------------------------- *
 *
 * To register as a smarty block function named 't', use:
 *   $smarty->register_block('t', 'smarty_translate');
 *
 * @package	smarty-T_
 * @version	$Id: smarty-T_.php,v 1.1 2004/04/30 11:39:32 sagi Exp $
 * @link	http://smarty-T_.sf.net/
 * @author	Sagi Bashari <sagi@boom.org.il>
 * @copyright 2004 Sagi Bashari
 */
 
/**
 * Replace arguments in a string with their values. Arguments are represented by % followed by their number.
 *
 * @param	string	Source string
 * @param	mixed	Arguments, can be passed in an array or through single variables.
 * @returns	string	Modified string
 */
function strarg($str)
{
	$tr = array();
	$p = 0;

	for ($i=1; $i < func_num_args(); $i++) {
		$arg = func_get_arg($i);
		
		if (is_array($arg)) {
			foreach ($arg as $aarg) {
				$tr['%'.++$p] = $aarg;
			}
		} else {
			$tr['%'.++$p] = $arg;
		}
	}
	
	return strtr($str, $tr);
}

/**
 * Smarty block function, provides T_ support for smarty.
 *
 * The block content is the text that should be translated.
 *
 * Any parameter that is sent to the function will be represented as %n in the translation text, 
 * where n is 1 for the first parameter. The following parameters are reserved:
 *   - escape - sets escape mode:
 *       - 'html' for HTML escaping, this is the default.
 *       - 'js' for javascript escaping.
 *       - 'no'/'off'/0 - turns off escaping
 *   - plural - The plural version of the text (2nd parameter of nT_())
 *   - count - The item count for plural mode (3rd parameter of nT_())
 */
function smarty_block_t($params, $text, &$smarty)
{
  if ($text == "")
    return "";
  
	$text = str_replace("\r\n", ' ', $text);
	$text = str_replace("\n", ' ', $text);
	
	// set escape mode
	if (isset($params['escape'])) {
		$escape = $params['escape'];
		unset($params['escape']);
	}
	
	// set plural version
	if (isset($params['plural'])) {
		$plural = $params['plural'];
		unset($params['plural']);
		
		// set count
		if (isset($params['count'])) {
			$count = $params['count'];
			unset($params['count']);
		}
	}
  
  $text = T_($text);

	// run strarg if there are parameters
	if (count($params)) {
		$text = strarg($text, $params);
	}

	return $text;
}

?>
