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

require "SessionManager.class.php";

use Exception;

class Premium {

  public function __construct($settings, $session) {
    $this->settings = $settings;
    $this->session = $session;
  }
  
  public function run() {
    if (isset($_GET["ipn"])) {
      $this->ipn();
      exit;
    }
    
    if ($this->loggedIn()) {
      $this->user = $this->getUser($_SESSION["email"]);
    
      if (isset($_GET["vote"])) {
        $this->vote();
      }
    }
  }

  public function getUser($email) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where email = '%s'",
      $mysql->escape_string($email)
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
    if ($result->num_rows != 0) {
      return $result->fetch_object();
    }
    else {
      throw new \Exception("failed to get user by email: " . $email);
    }
  }
  
  public function register() {
    $mysql = $this->getMysql();
    $error = $this->registerValidate($mysql);
    
    if ($error == "") {
      $this->registerSql($mysql);
      $this->login($_POST["email1"]);
    }
    
    $result["error"] = $error;
    return json_encode($result);
  }
  
  public function vote() {
    if ($this->user->votesFree <= 0) {
      // TODO: show error.
      return;
    }
    
    if (preg_match("/(\d+)/", $_POST["issue"], $issueMatches) != 1) {
      return;
    }
    $issueId = (int)$issueMatches[1];
    
    if (preg_match("/(\d+)/", $_POST["votes"], $votesMatches) != 1) {
      return;
    }
    $voteCount = (int)$votesMatches[1];
    
    $mysql = $this->getMysql();
    $result = $mysql->multi_query(sprintf(
      "insert into vote (userId, issueId, voteCount) values (%d, %d, %d) ".
      "on duplicate key update voteCount = voteCount + %d; ".
      "update user set votesFree = votesFree - %d",
      $this->user->id,
      $issueId,
      $voteCount,
      $voteCount,
      $voteCount
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
  
  public function login($email) {
    $_SESSION["email"] = $email;
  }
  
  public function loggedIn() {
    return (isset($_SESSION["email"]) && ($_SESSION["email"] != ""));
  }
  
  public function getFreeVoteCount() {
    return $this->user->votesFree;
  }
  
  public function getUserVotes() {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from vote where userId = %d ".
      "order by voteCount desc",
      $this->user->id
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
    $votes = array();
    while ($vote = $result->fetch_object()) {
      array_push($votes, $vote);
    }
    return $votes;
  }
  
  public function getAllVotes() {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select issueId, sum(voteCount) as voteCount from vote ".
      "group by issueId ".
      "order by voteCount desc",
      $this->user->id
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
    $votes = array();
    while ($vote = $result->fetch_object()) {
      array_push($votes, $vote);
    }
    return $votes;
  }
  
  public function registerSql($mysql) {
    $result = $mysql->query(sprintf(
      "insert into user (name, email, password, created) values ('%s', '%s', '%s', now())",
      $mysql->escape_string($_POST["name"]),
      $mysql->escape_string($_POST["email1"]),
      $mysql->escape_string($_POST["password1"])
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
  
  public function getMysql() {
    $s = $this->settings["database"];
    return new \mysqli($s["host"], $s["user"], $s["pass"], $s["name"]);
  }
  
  public function registerValidate($mysql) {
    $error = null;
    if ($_POST["name"] == "") {
      $error = T_("Please enter your name.");
    }
    else if ($_POST["email1"] == "") {
      $error = T_("Please enter your email address.");
    }
    else if (preg_match("/.+@.+/", $_POST["email1"]) != 1) {
      $error = T_("Your email address does not look valid.");
    }
    else if ($this->isEmailInuse($mysql, $_POST["email1"])) {
      $error = T_("That email address is already in use, are you already registered?");
    }
    else if ($_POST["email1"] != $_POST["email2"]) {
      $error = T_("The confirm email textbox must match the email address textbox.");
    }
    else if ($_POST["password1"] == "") {
      $error = T_("Please enter a password.");
    }
    else if ($_POST["password1"] != $_POST["password2"]) {
      $error = T_("The confirm password texbox must match the password textbox.");
    }
    return $error;
  }
  
  public function isEmailInUse($mysql, $email) {
    $result = $mysql->query(sprintf(
      "select count(*) from user where email = '%s'",
      $mysql->escape_string($email)
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
    $row = $result->fetch_array();
    return (int)($row[0]);
  }

  public function ipn() {
    // read the post from PayPal system and add 'cmd'
    $req = 'cmd=' . urlencode('_notify-validate');

    foreach ($_POST as $key => $value) {
      $value = urlencode(stripslashes($value));
      $req .= "&$key=$value";
    }


    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, 'https://www.paypal.com/cgi-bin/webscr');
    curl_setopt($ch, CURLOPT_HEADER, 0);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $req);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, 1);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array('Host: www.paypal.com'));
    $res = curl_exec($ch);
    curl_close($ch);


    // assign posted variables to local variables
    $item_name = $_POST['item_name'];
    $item_number = $_POST['item_number'];
    $payment_status = $_POST['payment_status'];
    $payment_amount = $_POST['mc_gross'];
    $payment_currency = $_POST['mc_currency'];
    $txn_id = $_POST['txn_id'];
    $receiver_email = $_POST['receiver_email'];
    $payer_email = $_POST['payer_email'];


    if (strcmp ($res, "VERIFIED") == 0) {
      // check the payment_status is Completed
      // check that txn_id has not been previously processed
      // check that receiver_email is your Primary PayPal email
      // check that payment_amount/payment_currency are correct
      // process payment
    }
    else if (strcmp ($res, "INVALID") == 0) {
      // log for manual investigation
    }
  }
}

?>
