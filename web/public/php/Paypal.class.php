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

// http://coding.smashingmagazine.com/2011/09/05/getting-started-with-the-paypal-api/
class PayPal {
  
  private $version = "74.0";

  public function __construct($user, $password, $signature, $endPoint) {
    $this->endPoint = $endPoint;
    $this->credentials = array(
      "USER" => $user,
      "PWD" => $password,
      "SIGNATURE" => $signature,
    );
  }

  public function request($method, $params = array()) {
    
    if (empty($method)) {
      throw new \Exception("API method is missing");
    }

    $requestParams = array(
      "METHOD" => $method,
      "VERSION" => $this->version
    ) + $this->credentials;
    
    $request = http_build_query($requestParams + $params);
    
    $curlOptions = array(
      CURLOPT_URL => $this->endPoint,
      CURLOPT_VERBOSE => 1,
      CURLOPT_SSL_VERIFYPEER => true,
      CURLOPT_SSL_VERIFYHOST => 2,
      CURLOPT_CAINFO => dirname(__FILE__) . "/cacert.pem",
      CURLOPT_RETURNTRANSFER => 1,
      CURLOPT_POST => 1,
      CURLOPT_POSTFIELDS => $request
    );

    $ch = curl_init();
    curl_setopt_array($ch,$curlOptions);

    $response = curl_exec($ch);

    $error = curl_errno($ch);
    curl_close($ch);

    if ($error != 0) {
      throw new \Exception("curl error: " + curl_error($ch));
    }
    else {
      // name value pair to array
      $responseArray = array();
      parse_str($response,$responseArray);
      
      return $responseArray;
    }
  }
}