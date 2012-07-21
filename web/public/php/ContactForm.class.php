<?php

/*
 * synergy-web -- website for synergy
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

class ContactForm {
  private $from = "Synergy website <website@synergy-foss.org>";
  private $to = "contact@synergy-foss.org";
  private $subject = "Contact form";

  public function __construct() {
    $this->name = isset($_POST["name"]) ? $_POST["name"] : "";
    $this->email1 = isset($_POST["email1"]) ? $_POST["email1"] : "";
    $this->email2 = isset($_POST["email2"]) ? $_POST["email2"] : "";
    $this->comments = isset($_POST["comments"]) ? $_POST["comments"] : "";
    $this->human = isset($_POST["human"]) ? $_POST["human"] : "";
  }
  
  public function isEmpty() {
    return $this->name == "";
  }
  
  public function send() {
    $message = sprintf("Name: %s\nEmail: %s\n\n%s",
      $this->name, $this->email1, $this->comments);
    $headers = sprintf("From: %s\r\nReply-To: %s", $this->from, $this->email1);
    mail($this->to, $this->subject, $message, $headers);
  }
}
 
 ?>