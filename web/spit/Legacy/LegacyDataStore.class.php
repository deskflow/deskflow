<?php

/*
 * SPIT: Simple PHP Issue Tracker
 * Copyright (C) 2012 Bolton Software Ltd.
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

class LegacyDataStore extends Spit\DataStores\DataStore {
  
  public function getIdByRedmineId($id) {
    $result = $this->query(
      "select id from `custom` where redmineId = %d",
      (int)$id
    );
    
    return $this->fromResultSingle($result);
  }
  
  public function getIdByGoogleId($id) {
    $result = $this->query(
      "select id from `custom` where googleId = \"gc-%d\"",
      (int)$id
    );
    
    return $this->fromResultSingle($result);
  }
}

?>
