<?php

/*
 * SPIT: Simple PHP Issue Tracker
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require "LegacyController.class.php";

class Legacy {
  
  public function __construct($spit) {
    $spit->addController("legacy", new LegacyController($this));
    
    $redmine = new \stdClass;
    $redmine->find = "/#redmine:(\d+)/";
    $redmine->replace = sprintf("[#$1](%s/legacy/?redmine=$1)", $spit->getProjectRoot(false));
    array_push($spit->textRegex, $redmine);
    
    $google = new \stdClass;
    $google->find = "/#google:(\d+)/";
    $google->replace = sprintf("[#$1](%s/legacy/?google=$1)", $spit->getProjectRoot(false));
    array_push($spit->textRegex, $google);
  }
}

?>
