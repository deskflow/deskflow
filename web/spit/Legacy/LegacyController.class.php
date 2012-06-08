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

require_once "LegacyDataStore.class.php";

class LegacyController extends Spit\Controllers\Controller {
  
  public function run() {
    $dataStore = new LegacyDataStore;
    
    if (isset($_GET["redmine"])) {
      $row = $dataStore->getIdByRedmineId($_GET["redmine"]);
    }
    elseif (isset($_GET["google"])) {
      $row = $dataStore->getIdByGoogleId($_GET["google"]);
    }
    else {
      exit("Either redmine or google must be specified."); 
    }
    
    header("Location: " . $this->app->linkProvider->forIssue($row->id));
  }
}

?>
