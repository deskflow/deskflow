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
require_once "php/Models/PaymentInfo.php";
require_once "php/SessionManager.class.php";
require_once "php/Exceptions.php";

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
    if (preg_match("/premium\/?$/", $path)) {
      $this->runIndex();
    }
    else if (preg_match("/premium\/register\/?$/", $path)) {
      $this->runRegister();
    }
    else if (preg_match("/premium\/payment\/?$/", $path)) {
      $this->runPayment();
    }
    else if (preg_match("/premium\/reset\/?$/", $path)) {
      $this->runResetPassword();
    }
    else if (preg_match("/premium\/json\/auth\/?$/", $path)) {
      $this->runJsonAuth();
    }
    else {
      $this->showPageNotFound();
    }
  }
  
  public function runIndex() {
    if (isset($_GET["ipn"])) {
      $this->ipn();
      return;
    }
    else if (isset($_GET["gwnotify"])) {
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
    
    if ($this->isLoggedIn()) {
      $this->smarty->assign("userVotes", $this->getUserVotes());
      $this->smarty->assign("userPayments", $this->getUserPayments());
      $this->smarty->assign("user", $this->user);
    }
    
    $this->smarty->assign("title", T_("Synergy Premium"));
    $this->showView("premium/index");
  }
  
  private function runRegister() {
    $smarty = $this->website->smarty;
    
    if (isset($_GET["notify"])) {
    
      if (!$this->isLoggedIn()) {
        throw new \Exception("user is not logged in");
      }
      
      if ($_GET["notify"] == "prompt") {
        $smarty->assign("title", T_("Synergy Premium - Notify"));
        $smarty->assign("showForm", false);
        $smarty->assign("showNotify", true);
        $this->showView("premium/register");
      }
      else {
        if ($_GET["notify"] == "yes") {
          $this->updateNotifyByEmail(true);
        }
        else if ($_GET["notify"] == "no") {
          $this->updateNotifyByEmail(false);
        }
        $result["paymentUrl"] = $this->getPremiumPaymentUrl();
        header("Location: ../payment/?amount=" . urlencode("$") . $this->getAmount());
      }
      return;
    }
    
    if ($this->isLoggedIn()) {
      header("Location: ../payment/");
      return;
    }
    
    $error = null;
    if (isset($_POST["name"])) {
      
      $mysql = $this->getMysql();
      $error = $this->registerValidate($mysql);
      
      if ($error == null) {
        $this->registerSql($mysql);
        $this->login($_POST["email1"]);
        header("Location: ./?notify=prompt&amount=" . urlencode("$") . $this->getAmount());
        exit;
      }
    }
    
    $smarty->assign("name", $this->getPostValue("name"));
    $smarty->assign("email1", $this->getPostValue("email1"));
    $smarty->assign("email2", $this->getPostValue("email2"));
    $smarty->assign("password1", $this->getPostValue("password1"));
    $smarty->assign("password2", $this->getPostValue("password2"));
    $smarty->assign("amount", $this->getAmount());
    $smarty->assign("error", $error);
    $smarty->assign("title", T_("Synergy Premium - Register"));
    $smarty->assign("showForm", true);
    $smarty->assign("showNotify", false);
    $this->showView("premium/register");
  }
  
  private function updateNotifyByEmail($value) {
    $this->loadUser();
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "update user set ".
      "notifyByEmail = %d ".
      "where id = %d",
      $value,
      $this->user->id
    ));
  }
  
  private function getAmount() {
    $amount = self::$defaultAmount;
    if (isset($_POST["amount"])) {
      $amount = $_POST["amount"];
    }
    else if (isset($_GET["amount"])) {
      $amount = $_GET["amount"];
    }
    
    if (!strncmp($amount, "$", strlen("$"))) {
      $amount = substr($amount, 1);
    }
    
    return $amount > 0 ? (float)$amount : 1;
  }
  
  private function runPayment() {
    
    if (isset($_GET["currency"])) {
      exit($this->convertCurrency());
    }
    
    if (!$this->isLoggedIn()) {
      header("Location: ../");
      exit;
    }
    
    $this->loadUser();
  
    $amount = $this->getAmount();
    $userId = $this->user->id;
    
    $smarty = $this->website->smarty;
    $smarty->assign("startYear", date("Y"));
    $smarty->assign("endYear", date("Y") + 20);
    $smarty->assign("amount", $amount);
    $smarty->assign("userId", $userId);
    $smarty->assign("user", $this->user);
    
    $monthList = array();
    for ($i = 0; $i < 12; $i++) {
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
        $response = $payment->process($userId);
        $success = $response["ACK"] == "Success";
        $failed = !$success;
        
        if ($success) {
          $funds = (float)$response["AMT"];
          $userId = $userId;
          $this->assignVotesFromFunds($userId, $funds);
        }
        else if ($failed) {
          $smarty->assign("cardFailMessage", $response["L_LONGMESSAGE0"]);
        }
      }
      catch (\Synergy\PaymentException $e) {
        $failed = true;
        $smarty->assign("cardFailMessage", $e->getMessage());
      }
    }
    
    $card = array();
    $card["number"] = isset($_POST["number"]) ? $_POST["number"] : null;
    $card["name"] = isset($_POST["name"]) ? $_POST["name"] : null;
    $card["month"] = isset($_POST["month"]) ? $_POST["month"] : null;
    $card["year"] = isset($_POST["year"]) ? $_POST["year"] : null;
    $card["cvv2"] = isset($_POST["cvv2"]) ? $_POST["cvv2"] : null;
    $smarty->assign("card", $card);
    
    $smarty->assign("showCardForm", !$success);
    $smarty->assign("showCardFailed", $failed);
    $smarty->assign("showCardSuccess", $success);
    
    $smarty->assign("title", T_("Synergy Premium - Payment"));
    $this->showView("premium/payment");
  }
  
  public function runResetPassword() {    
    $error = null;
    $token = isset($_GET["token"]) ? $_GET["token"] : null;
    $email = $this->getPostValue("email");
    $passwordForm = $token != null;
    $emailForm = $token == null;
    $emailSuccess = false;
    
    $user = null;
    if ($token != null) {
      $user = $this->getUserByResetToken($token);
    }
    
    if ($this->isPostBack()) {
      try {
        if ($token != null) {
          $this->setPasswordFromForm($user->email);
          $this->login($user->email, false);
          header("Location: ../");
          exit;
        }
        else {
          $this->sendPasswordResetEmail($email);
          $emailSuccess = true;
          $emailForm = false;
        }
      }
      catch (\Synergy\ValidationError $e) {
        $error = $e->getMessage();
      }
    }
  
    $smarty = $this->website->smarty;
    $smarty->assign("error", $error);
    $smarty->assign("emailSuccess", $emailSuccess);
    $smarty->assign("emailForm", $emailForm);
    $smarty->assign("passwordForm", $passwordForm);
    $smarty->assign("email", $email);
    $smarty->assign("title", T_("Synergy Premium - Reset Password"));
    $this->showView("premium/reset");
  }
  
  private function runJsonAuth() {
    $request = json_decode($_POST["json"]);
    $email = $request->email;
    $password = $request->password;
    
    $response = new \stdClass;
    try {
      $response->result = $this->auth(false, $email, $password);
      if ($response->result) {
        $this->recordGuiLogin($email);
      }
    }
    catch (\Exception $ex) {
      $response->error = $ex->getMessage();
    }
    
    echo json_encode($response);
  }
  
  private function recordGuiLogin($email) {
    $userAgent = $_SERVER["HTTP_USER_AGENT"];
    
    if (!preg_match("/Synergy GUI ([\\d.]+)/", $userAgent, $matches)) {
      return;
    }
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "update user set ".
      "lastGuiLogin = now(), lastGuiVersion = '%s' ".
      "where email = '%s'",
      $matches[1],
      $mysql->escape_string($email)
    ));
  }
  
  private function setPasswordFromForm($email) {
    $password1 = $this->getPostValue("password1");
    $password2 = $this->getPostValue("password2");
    
    if ($password1 != $password2) {
      throw new \Synergy\ValidationError(T_("The password fields do not match."));
    }
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "update user set ".
      "resetToken = NULL, password = '%s' ".
      "where email = '%s'",
      md5($password1),
      $mysql->escape_string($email)
    ));
  }
  
  private function sendPasswordResetEmail($email) {
    if ($email == null) {
      throw new \Synergy\ValidationError(T_("Email field was empty."));
    }
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where email = '%s'",
      $mysql->escape_string($email)
    ));
      
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    else {
      if ($result->num_rows != 0) {
        $token = $this->createResetToken($email);
        $resetUrl = sprintf("https://%s?token=%s", $_SERVER["SERVER_NAME"] . $_SERVER["REQUEST_URI"], $token);
        $subject = T_("Password reset");
        $message = T_("Hello,\n\nPlease click the following link to reset your password.\n\n") . $resetUrl;
        $headers = sprintf("From: %s <%s>", T_("Synergy Website"), $this->settings["general"]["websiteEmail"]);
        $result = mail($email, $subject, $message, $headers);
        
        if (!$result) {
          throw new \Exception("mail function failed.");
        }
      }
      else {
        throw new \Synergy\ValidationError(T_("Invalid email address, please check and try again."));
      }
    }
  }
  
  private function createResetToken($email) {
    $token = sha1(mt_rand() . $this->settings["general"]["tokenSeed"]);
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "update user set resetToken = '%s' where email = '%s'",
      $token,
      $mysql->escape_string($email)
    ));
    return $token;
  }
  
  public function auth($login = true, $email = null, $password = null) {
    
    if ($email == null) {
      $email = $_POST["email"];
    }
    
    if ($password == null) {
      $password = $_POST["password"];
    }
    
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where email = '%s' and password = '%s'",
      $mysql->escape_string($email),
      md5($password)
    ));
    
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    
    if ($result->num_rows != 0) {
      $this->user = $result->fetch_object();
      if ($login) {
        $this->login($email, false);
      }
      return true;
    }
    else {
      $this->email = $email;
      $this->loginInvalid = true;
      return false;
    }
  }
  
  public function loadUser($email = null, $force = false) {
    if ($email == null) {
      $email = $this->getUserEmail();
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
  
  private function getUserEmail() {
    $isAdmin = $_SESSION["email"] == $this->settings["general"]["adminEmail"];
    if ($isAdmin & isset($_GET["as"])) {
      return $_GET["as"];
    }
    return $_SESSION["email"];
  }

  public function getUser($email) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where email = '%s'",
      $mysql->escape_string($email)
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    if ($result->num_rows != 0) {
      return $result->fetch_object();
    }
    else {
      throw new \Exception("failed to get user by email: " . $email);
    }
  }

  public function getUserByResetToken($resetToken) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select * from user where resetToken = '%s'",
      $mysql->escape_string($resetToken)
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    if ($result->num_rows != 0) {
      return $result->fetch_object();
    }
    else {
      throw new \Exception("failed to get user by reset token: " . $resetToken);
    }
  }
  
  public function checkUser() {
    $result = array();
    if ($this->isLoggedIn()) {
      $this->loadUser();
      $result["userId"] = $this->user->id;
      $result["paymentUrl"] = $this->getPremiumPaymentUrl();
    }
    return json_encode($result);
  }
  
  private function getPremiumPaymentUrl() {
    $site = $this->settings["general"]["secureSite"];
    return $site . "/premium/payment/";
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
      throw new \Exception($mysql->error);
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
      throw new \Exception($mysql->error);
    }
    $votes = array();
    while ($vote = $result->fetch_object()) {
      array_push($votes, $vote);
    }
    return $votes;
  }
  
  public function getUserPayments() {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select userId, created, amount, result, method from ".
      "(".
      "  select userId, created, amount, ipnCheck as result, ".
      "  'paypal' as method ".
      "  from paypal ".
      
      "  union ".
      
      "  select userId, created, amount, ".
      "  ExtractValue(data, '//financial-order-state') as result, ".
      "  'google' as method ".
      "  from gwallet ".
      
      "  union ".
      
      "  select userId, created, amount, result, ".
      "  'card' as method ".
      "  from creditcard ".
      
      ") as payments ".
      "where userId = %d ".
      "order by created asc",
      $this->user->id
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
    }
    $payments = array();
    while ($paymentData = $result->fetch_object()) {
      $payments[] = new \Synergy\Models\PaymentInfo($paymentData);
    }
    return $payments;
  }
  
  public function getAllVotes($limit = null) {
    $mysql = $this->getMysql();
    $result = $mysql->query(sprintf(
      "select v.issueId, sum(v.voteCount) as voteCountSum, i.status ".
      "from vote as v ".
      "left join issue as i on i.id = v.issueId ".
      "where v.voteCount != 0 ".
      "group by v.issueId ".
      "order by voteCountSum desc ".
      (($limit != null) ? sprintf("limit 0, %d ", $limit) : "")
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
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
      $this->getAmount()
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
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
      $error = T_("The email address does not appear to be valid.");
    }
    else if ($this->isEmailInuse($mysql, $_POST["email1"])) {
      $error = T_("Welcome back, you've already registered. Please <a href=\"../\">log in</a> to your account.");
    }
    else if ($_POST["email1"] != $_POST["email2"]) {
      $error = T_("The confirm email must match the email address.");
    }
    else if ($_POST["password1"] == "") {
      $error = T_("Please enter a password.");
    }
    else if ($_POST["password1"] != $_POST["password2"]) {
      $error = T_("The confirm password must match the password.");
    }
    return $error;
  }
  
  public function isEmailInUse($mysql, $email) {
    $result = $mysql->query(sprintf(
      "select count(*) from user where email = '%s'",
      $mysql->escape_string($email)
    ));
    if ($result == null) {
      throw new \Exception($mysql->error);
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
      throw new \Exception($mysql->error);
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
      throw new \Exception($mysql->error);
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
      throw new \Exception($mysql->error);
    }
  }
}
 
 ?>
 