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

namespace Synergy;

require "Paypal.class.php";

class Payment {

  public function __construct($settings) {
    $this->settings = $settings;
    $this->paypal = new Paypal(
      $settings["paypal"]["user"],
      $settings["paypal"]["password"],
      $settings["paypal"]["signature"],
      $settings["paypal"]["endpoint"]);
  }

  public function process() {
    
    $requestParams = array(
      "IPADDRESS" => $_SERVER["REMOTE_ADDR"],
      "PAYMENTACTION" => "Sale"
    );

    $creditCardDetails = array(
      "CREDITCARDTYPE" => $this->getCardType(),
      "ACCT" => $this->getCardNumber(),
      "EXPDATE" => $this->getExpiryDate(),
      "CVV2" => $_POST["cvv2"]
    );

    $payerDetails = array(
      "FIRSTNAME" => $this->getFirstName(),
      "LASTNAME" => $this->getLastName()
    );

    $orderParams = array(
      "AMT" => $_POST["amount"],
      "ITEMAMT" => $_POST["amount"],
      "SHIPPINGAMT" => "0",
      "CURRENCYCODE" => "USD"
    );

    $item = array(
      "L_NAME0" => "Synergy Premium",
      "L_DESC0" => "Account funds",
      "L_AMT0" => $_POST["amount"],
      "L_QTY0" => "1"
    );

    $response = $this->paypal->request("DoDirectPayment",
      $requestParams + $creditCardDetails + $payerDetails + $orderParams + $item);

    if (is_array($response) && $response["ACK"] == "Success") {
      $transactionId = $response["TRANSACTIONID"];
    }
    
    var_dump($response);
    exit;
  }
  
  private function getExpiryDate() {
    $month = str_pad($_POST["month"], 2, "0", STR_PAD_LEFT);
    $year = $_POST["year"];
    return $month . $year;
  }
  
  private function getCardNumber() {
    return str_replace(" ", "", $_POST["number"]);
  }
  
  private function getCardType() {
    // http://www.regular-expressions.info/creditcard.html
    $n = $this->getCardNumber();
    if (preg_match("/^4[0-9]{12}(?:[0-9]{3})?$/", $n)) {
      return "Visa";
    }
    elseif (preg_match("/^5[1-5][0-9]{14}$/", $n)) {
      return "MasterCard";
    }
    elseif (preg_match("/^3[47][0-9]{13}$/", $n)) {
      return "Amex";
    }
    elseif (preg_match("/^6(?:011|5[0-9]{2})[0-9]{12}$/", $n)) {
      return "Discover";
    }
    throw new \Exception("unrecognised credit card number: " . $n);
  }
  
  private function getFirstName() {
    $parts = explode(" ", $_POST["name"]);
    if (count($parts) > 1) {
      unset($parts[count($parts) - 1]);
      return join(" ", $parts);
    }
    elseif (count($parts) > 0) {
      return $parts[0];
    }
    return "";
  }
  
  private function getLastName() {
    $parts = explode(" ", $_POST["name"]);
    if (count($parts) > 1) {
      return $parts[count($parts) - 1];
    }
    return "";
  }
}
 
?>