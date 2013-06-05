<?php

/*
 * synergy-web -- website for synergy
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

namespace Synergy;

class SessionManager {

  var $lifetime;

  function __construct($settings) {
    $this->settings = $settings;
    $this->lifetime = $settings["session"]["lifetime"];
    
    session_set_save_handler( 
      array(&$this, "open"), 
      array(&$this, "close"),
      array(&$this, "read"),
      array(&$this, "write"),
      array(&$this, "destroy"),
      array(&$this, "gc")
    );
  }
  
  public function start() {
    session_start();
  }
  
  public function getMysql() {
    $s = $this->settings["database"];
    return new \mysqli($s["host"], $s["user"], $s["pass"], $s["name"]);
  }

  function open($save_path, $session_name) {
    return true;
  }

  function close() {
    return true;
  }

  function read($id) {    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select data from session where ".
      "id = '%s' and expires > %d",
      $mysql->escape_string($id),
      time()
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    
    $data = "";
    if ($result->num_rows != 0) {
      $row = $result->fetch_assoc();
      $data = $row["data"];
    }
    return $data;
  }

  function write($id, $data) {
    $mysql = $this->getMysql();
    if ($data != "") {
      $result = $mysql->query(sprintf(
        "replace session (id, data, expires) values ('%s', '%s', %d)",
        $mysql->escape_string($id),
        $mysql->escape_string($data),
        (time() + $this->lifetime)
      ));
      if ($result == null) {
        throw new \Exception($mysql->error);
      }
    }
    else {
      // delete sessions with no data.
      $this->destroy($id);
    }
    return true;
  }

  function destroy($id) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "delete from session where id = '%s'",
      $mysql->escape_string($id)
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    return true;
  }

  function gc() {
    $mysql = $this->getMysql();
    $result = $mysql->query(
      "delete from session where expires < unix_timestamp()"
    );
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    return true;
  }
}

?>
