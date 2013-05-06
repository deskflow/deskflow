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

namespace Synergy\Controllers;

require_once "Controller.php";
require_once "php/Payment.class.php";
require_once "php/SessionManager.class.php";

use Exception;
 
class PremiumController extends Controller {
  
  private static $defaultAmount = 10;
  
  var $loginInvalid;
  var $email;
  var $user;

  public function __construct($smarty, $website) {
    parent::__construct($smarty);
    $this->website = $website;
    $this->settings = $website->settings;
    $this->session = $website->session;
    $this->voteCost = (int)$this->settings["premium"]["voteCost"];
    $this->googleWallet = $this->settings["googlewallet"];
  }
  
  public function isUserPremium() {
    if ($this->isLoggedIn()) {
      $this->loadUser();
      return $this->user->fundsCount != 0;
    }
    else {
      return false;
    }
  }
  
  public function run($path) {
    if (preg_match("/premium\/payment/", $path)) {
      $this->runPayment();
    }
    else {
      $this->runIndex();
    }
  }
  
  public function runIndex() {
    if (isset($_GET["ipn"])) {
      $this->ipn();
      return;
    }
    
    if (isset($_GET["gwnotify"])) {
      $this->googleWalletNotify();
      return;
    }
    
    if ($this->isLoggedIn()) {
      $this->loadUser();
    
      if (isset($_GET["vote"])) {
        $this->vote();
      }
      else if (isset($_GET["logout"])) {
        $this->logout();
      }
    }
    else if (isset($_GET["login"])) {
      $this->auth();
    }
    
    $this->showView("premium/index");
  }
  
  function runPayment() {
  
    $this->loadUser();
  
    $amount = isset($_POST["amount"]) ? $_POST["amount"] : self::$defaultAmount;
    $userId = $this->user->id;
    
    $smarty = $this->website->smarty;
    $smarty->assign("startYear", date("Y"));
    $smarty->assign("endYear", date("Y") + 20);
    $smarty->assign("amount", $amount);
    $smarty->assign("userId", $userId);
    
    $monthList = array();
    for ($i = 0; $i < 11; $i++) {
      $number = $i + 1;
      $text = date("F", mktime(0, 0, 0, $number, 10));
      $monthList[] = array(
        "number" => $number,
        "text" => str_pad($number, 2, "0", STR_PAD_LEFT) . " - " . $text
      );
    }
    $smarty->assign("monthList", $monthList);
    
    $failed = false;
    $success = false;
    
    if (isset($_POST["number"])) {
      $payment = new \Synergy\Payment($this->settings);
      try {
        $response = $payment->process();      
        $success = $response["ACK"] == "Success";
        $failed = !$success;
        
        if ($success) {
          $funds = (float)$response["AMT"];
          $userId = $userId;
          $this->assignVotesFromFunds($userId, $funds);
        }
        else if ($failed) {
          $smarty->assign("failMessage", $response["L_LONGMESSAGE0"]);
        }
      }
      catch (\Synergy\PaymentException $e) {
        $failed = true;
        $smarty->assign("failMessage", $e->getMessage());
      }
    }
    
    $card = array();
    $card["number"] = isset($_POST["number"]) ? $_POST["number"] : null;
    $card["name"] = isset($_POST["name"]) ? $_POST["name"] : null;
    $card["month"] = isset($_POST["month"]) ? $_POST["month"] : null;
    $card["year"] = isset($_POST["year"]) ? $_POST["year"] : null;
    $card["cvv2"] = isset($_POST["cvv2"]) ? $_POST["cvv2"] : null;
    $smarty->assign("card", $card);
    
    $smarty->assign("showForm", !$success);
    $smarty->assign("showFailed", $failed);
    $smarty->assign("showSuccess", $success);
    
    $this->showView("premium/payment");
  }
  
  public function auth() {    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where email = '%s' and password = '%s'",
      $mysql->escape_string($_POST["email"]),
      md5($_POST["password"])
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
    if ($result->num_rows != 0) {
      $this->user = $result->fetch_object();
      $this->login($_POST["email"], false);
    }
    else {
      $this->email = $_POST["email"];
      $this->loginInvalid = true;
    }
  }
  
  public function loadUser($email = null, $force = false) {
    if ($email == null) {
      $email = $_SESSION["email"];
    }
    if ($this->user == null || $force) {
      try {
        $this->user = $this->getUser($email);
      }
      catch (Exception $ex) {
        $this->logout();
        throw $ex;
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
      $result["userId"] = $this->user->id;
    }
    
    $result["error"] = $error;
    return json_encode($result);
  }
  
  public function vote() {
    
    if (preg_match("/(\d+)/", $_POST["issue"], $issueMatches) != 1) {
      return;
    }
    $issueId = (int)$issueMatches[1];
    
    if (preg_match("/(\d+)/", $_POST["votes"], $votesMatches) != 1) {
      return;
    }
    $voteCount = (int)$votesMatches[1];
    
    if ($voteCount > $this->user->votesFree) {
      $voteCount = $this->user->votesFree;
      if ($voteCount == 0) {
        // TODO: show error.
        return;
      }
    }
    
    // update in memory for page (on next page load it's loaded from db).
    $this->user->votesFree -= $voteCount;
    
    $mysql = $this->getMysql();
    $result = $mysql->multi_query(sprintf(
      "insert into vote (userId, issueId, voteCount) values (%d, %d, %d) ".
      "on duplicate key update voteCount = voteCount + %d; ".
      "update user set votesFree = votesFree - %d where id = %d",
      $this->user->id,
      $issueId,
      $voteCount,
      $voteCount,
      $voteCount,
      $this->user->id
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
  
  public function login($email, $load = true) {
    $_SESSION["email"] = $email;
    if ($load) {
      $this->loadUser($email, true);
    }
  }
  
  public function logout() {
    $this->user = null;
    unset($_SESSION["email"]);
  }
  
  public function isLoggedIn() {
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
  
  public function getAllVotes($limit = null) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select issueId, sum(voteCount) as voteCount from vote ".
      "where voteCount != 0 ".
      "group by issueId ".
      "order by voteCount desc ".
      (($limit != null) ? sprintf("limit 0, %d ", $limit) : "")
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
      "insert into user (name, email, password, fundsSignup, created) ".
      "values ('%s', '%s', '%s', %f, now())",
      $mysql->escape_string($_POST["name"]),
      $mysql->escape_string($_POST["email1"]),
      md5($_POST["password1"]),
      (float)$_POST["amount"]
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
  
  public function getMysql() {
    $s = $this->settings["database"];
    $sql = new \mysqli($s["host"], $s["user"], $s["pass"], $s["name"]);
    $sql->set_charset("utf8");
    return $sql;
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
  
  public function saveIpnData($userId, $email, $data, $check, $amount) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "insert into paypal (userId, email, ipnData, ipnCheck, amount, created) ".
      "values (%d, '%s', '%s', '%s', %f, now())",
      (int)$userId,
      $mysql->escape_string($email),
      $mysql->escape_string($data),
      $mysql->escape_string($check),
      (float)$amount
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
  
  public function assignVotesFromFunds($userId, $funds) {
    $votes = $funds / $this->voteCost;
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "update user set votesFree = votesFree + %d, ".
      "fundsTotal = fundsTotal + %f, ".
      "fundsCount = fundsCount + 1 ".
      "where id = %d",
      (int)$votes,
      (float)$funds,
      (int)$userId
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }

  public function ipn() {
    // read the post from PayPal system and add "cmd"
    $req = "cmd=" . urlencode("_notify-validate");

    foreach ($_POST as $key => $value) {
      $value = urlencode(stripslashes($value));
      $req .= "&$key=$value";
    }

    // ask paypal to verify the ipn request.
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, "https://www.paypal.com/cgi-bin/webscr");
    curl_setopt($ch, CURLOPT_HEADER, 0);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $req);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, 1);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array("Host: www.paypal.com"));
    $res = curl_exec($ch);
    curl_close($ch);

    // assign posted variables to local variables
    $item_name = $_POST["item_name"];
    $item_number = $_POST["item_number"];
    $payment_status = $_POST["payment_status"];
    $payment_amount = $_POST["mc_gross"];
    $payment_currency = $_POST["mc_currency"];
    $txn_id = $_POST["txn_id"];
    $receiver_email = $_POST["receiver_email"];
    $payer_email = $_POST["payer_email"];
    $userId = $_POST["custom"];
    
    $this->saveIpnData($userId, $payer_email, json_encode($_POST), $res, $payment_amount);
    
    if ($res == "VERIFIED" && $payment_status == "Completed") {
      $this->assignVotesFromFunds($userId, $payment_amount);
    }
  }
  
  public function convertCurrency() {
    $response = file_get_contents("http://www.google.com/ig/calculator?hl=en&q=" . $_GET["currency"]);
    
    $matches = array();
    preg_match_all("/(\d+(\.\d+)*)/", $response, $matches);
    
    $data = array();
    if (count($matches) > 0) {
      $data["from"] = $matches[0][0];
      $data["to"] = $matches[0][1];
    }
    return json_encode($data);
  }
  
  public function googleWalletNotify() {
    
    $postData =
      '<notification-history-request xmlns="http://checkout.google.com/schema/2">'.
      '<serial-number>' . $_POST["serial-number"] . '</serial-number>'.
      '</notification-history-request>';
    
    $headers = array(
      "Authorization: Basic " . base64_encode($this->googleWallet["id"] . ":" . $this->googleWallet["key"]),
      "Content-Type: application/xml; charset=UTF-8",
      "Accept: application/xml; charset=UTF-8",
      "User-Agent: synergy-web");
    
    $ch = curl_init(sprintf(
      "https://checkout.google.com/api/checkout/v2/reports/Merchant/%d/",
      $this->googleWallet["id"]));
    
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $postData);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    
    $data = curl_exec($ch);
    
    // if notification isn't about a new order, just ignore it.
    if (!strstr($data, "new-order-notification")) {
      printf("<pre>Ignoring order.\n\n%s</pre>", $data);
      exit;
    }    
    
    $matches = array();
    preg_match("/<merchant\-private\-data>(.*),(.*)<\/merchant\-private\-data>/", $data, $matches);
    $userId = null;
    $amount = null;
    if (count($matches) > 2) {
      $userId = $matches[1];
      $amount = $matches[2];
    }
    
    $matches = array();
    preg_match("/<email>(.*)<\/email>/", $data, $matches);
    $email = null;
    if (count($matches) > 0) {
      $email = $matches[1];
    }
    
    $this->saveGoogleWalletNotify($userId, $email, $amount, $data);
    
    $this->assignVotesFromFunds($userId, $amount);
  }
  
  public function saveGoogleWalletNotify($userId, $email, $amount, $data) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "insert into gwallet (userId, email, amount, data, created) ".
      "values (%d, '%s', %f, '%s', now())",
      (int)$userId,
      $mysql->escape_string($email),
      (float)$amount,
      $mysql->escape_string($data)
    ));
    if ($result == null) {
      throw new Exception($mysql->error);
    }
  }
}
 
 ?>
 