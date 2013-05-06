<?php

/*
 * synergy-web -- website for synergy
 * Copyright (C) 2013 Bolton Software Ltd.
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

namespace Synergy\Models;

class PaymentInfo {

  public function __construct($data) {
    $this->data = $data;
  }
  
  public function __toString() {
    $r = array();
    $d = $this->data;
    
    switch ($d->method) {
      case "paypal": $r[] = "PayPal"; break;
      case "google": $r[] = "Google Wallet"; break;
      case "card": $r[] = "Credit card"; break;
    }
    
    $created = new \DateTime($d->created);
    $r[] = $created->format("jS M Y");
    
    $r[] = "$" . $d->amount;
    
    $successText = '<span class="success">Successfull</span>';
    
    switch ($d->method) {
      case "paypal":
        $r[] = ($d->result == "VERIFIED" ? $successText : "Unknown");
        break;
      
      case "google":
        $r[] = ($d->result == "REVIEWING" ? $successText : "Unknown");
        break;
      
      case "card":
        $r[] = ($d->result == "Success" ? $successText : "Failed");
        break;
    }
    
    return implode($r, ", ");
  }
}
 
?>