#!/usr/local/bin/php -qn
<?php
/**
 * tsmarty2c.php - rips gettext strings from smarty template
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
 * This command line script rips gettext strings from smarty file, and prints them to stdout in C format, 
 * that can later be used with the standard gettext tools.
 *
 * Usage:
 * ./tsmarty2c.php <filename or directory> <file2> <..> > smarty.c
 *
 * If a parameter is a directory, the template files within will be parsed.
 *
 * @package	smarty-gettext
 * @version	$Id: tsmarty2c.php,v 1.2 2004/04/30 11:40:22 sagi Exp $
 * @link	http://smarty-gettext.sf.net/
 * @author	Sagi Bashari <sagi@boom.org.il>
 * @copyright 2004 Sagi Bashari
 */

// smarty open tag
$ldq = preg_quote('{');

// smarty close tag
$rdq = preg_quote('}');

// smarty command
$cmd = preg_quote('t');

// extensions of smarty files, used when going through a directory
$extensions = array('tpl');

// "fix" string - strip slashes, escape and convert new lines to \n
function fs($str)
{
	$str = stripslashes($str);
	$str = str_replace('"', '\"', $str);
	$str = str_replace("\r\n", ' ', $str);
	$str = str_replace("\n", ' ', $str);
	return $str;
}

// rips gettext strings from $file and prints them in C format
function do_file($file)
{
	$content = @file_get_contents($file);

	if (empty($content)) {
		return;
	}

	global $ldq, $rdq, $cmd;

	preg_match_all("/{$ldq}\s*({$cmd})\s*([^{$rdq}]*){$rdq}([^{$ldq}]*){$ldq}\/\\1{$rdq}/", $content, $matches);
	
	for ($i=0; $i < count($matches[0]); $i++) {
		if (preg_match('/plural\s*=\s*["\']?\s*(.[^\"\']*)\s*["\']?/', $matches[2][$i], $match)) {
			print 'ngettext("'.fs($matches[3][$i]).'","'.fs($match[1]).'",x);'."\n";
		} else {
			print 'gettext("'.fs($matches[3][$i]).'");'."\n";
		}
	}
}

// go through a directory
function do_dir($dir)
{
	$d = dir($dir);

	while (false !== ($entry = $d->read())) {
		if ($entry == '.' || $entry == '..') {
			continue;
		}

		$entry = $dir.'/'.$entry;

		if (is_dir($entry)) { // if a directory, go through it
			do_dir($entry);
		} else { // if file, parse only if extension is matched
			$pi = pathinfo($entry);
			
			if (isset($pi['extension']) && in_array($pi['extension'], $GLOBALS['extensions'])) {
				do_file($entry);
			}
		}
	}

	$d->close();
}

for ($ac=1; $ac < $_SERVER['argc']; $ac++) {
	if (is_dir($_SERVER['argv'][$ac])) { // go through directory
		do_dir($_SERVER['argv'][$ac]);
	} else { // do file
		do_file($_SERVER['argv'][$ac]);
	}
}

?>
