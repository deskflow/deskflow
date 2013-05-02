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

class PaymentException extends \Exception {
  public function __construct($message, $code = 0, Exception $previous = null) {
    parent::__construct($message, $code, $previous);
  }
}

class Payment {

  public function __construct($settings) {
    $this->settings = $settings;
    $this->paypal = new Paypal(
      $settings["paypal"]["user"],
      $settings["paypal"]["password"],
      $settings["paypal"]["signature"],
      $settings["paypal"]["endpoint"]);
    $this->mysql = $this->getMysql();
  }

  public function process() {
    
    if ($this->getCardNumber() == "") {
      throw new PaymentException("No credit card number was provided.");
    }
    
    if ($this->getFirstName() == "") {
      throw new PaymentException("No name was provided.");
    }
    
    if ($_POST["cvv2"] == "") {
      throw new PaymentException("No security code (CVV2) was provided.");
    }
    
    $init = array(
      "IPADDRESS" => $_SERVER["REMOTE_ADDR"],
      "PAYMENTACTION" => "Sale"
    );

    $card = array(
      "CREDITCARDTYPE" => $this->getCardType(),
      "ACCT" => $this->getCardNumber(),
      "EXPDATE" => $this->getExpiryDate(),
      "CVV2" => $_POST["cvv2"]
    );

    $payer = array(
      "FIRSTNAME" => $this->getFirstName(),
      "LASTNAME" => $this->getLastName()
    );

    $order = array(
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
    
    $request = $init + $card + $payer + $order + $item;
    //echo "<!--";
    //var_dump($request);
    //echo "-->";
    $id = $this->saveRequest($_POST["userId"], $request);
    
    $response = $this->paypal->request("DoDirectPayment", $request);
    $this->saveResponse($id, $response);
    //echo "<!--";
    //var_dump($response);
    //echo "-->";

    if (is_array($response) && $response["ACK"] == "Success") {
      $transactionId = $response["TRANSACTIONID"];
    }
    
    return $response;
  }
  
  private function saveRequest($userId, $request) {
    
    // mask sensitive card details in case our db is attacked.
    $request["ACCT"] = str_pad(substr($request["ACCT"], 12), 16, "*", STR_PAD_LEFT);
    $request["CVV2"] = "***";
    
    $json = json_encode($request);
    
    $result = $this->mysql->query(sprintf(
      "insert into creditcard (userId, request, requestDate) ".
      "values (%d, '%s', now())",
      (int)$userId,
      $this->mysql->escape_string($json)
    ));
    
    if ($result == null) {
      throw new \Exception($this->mysql->error);
    }
    
    return $this->mysql->insert_id;
  }
  
  private function saveResponse($id, $response) {
    
    $json = json_encode($response);
    
    $result = $this->mysql->query(sprintf(
      "update creditcard set ".
      "response = '%s', responseDate = now(), result = '%s' ".
      "where id = %d",
      $this->mysql->escape_string($json),
      $this->mysql->escape_string($response["ACK"]),
      $id
    ));
    
    if ($result == null) {
      throw new \Exception($this->mysql->error);
    }
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
    throw new PaymentException("Unrecognised credit card number: " . $n);
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
  
  public function getMysql() {
    $s = $this->settings["database"];
    $sql = new \mysqli($s["host"], $s["user"], $s["pass"], $s["name"]);
    $sql->set_charset("utf8");
    return $sql;
  }
}
 
?>