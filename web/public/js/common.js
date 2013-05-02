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

var defaultPremiumSliderIndex = 1;
var defaultPremiumValue = 1;

function log(s) {
  if ("console" in self && "log" in console) console.log(s);
}

function formError(errors, error) {
  errors.fadeIn();
  errors.find("p").html("Form error: " + error);
}

function getPremiumValueFromIndex(index) {
  var value = index;
  if (value > 20) {
    // 20-100: from 100 to 1000 (exponentially).
    value = Math.round(Math.pow(value, 1.5) / 10) * 10;
  }
  else if (value > 10) {
    // 10-20: from 10 to 100 (increments of 10).
    value = (value - 10) * 10;
  }
  return value;
}

function getPremiumAmountFromText(text) {
  var valueArray = /\d+(\.\d+)*/.exec(text);
  if (valueArray == null) {
    return defaultPremiumValue;
  }
  value = parseFloat(valueArray[0]);
  log(value);
  if (value < 1) {
    return 1;
  }
  return value;
}

function getPremiumIndexFromText(text) {
  var value = getPremiumAmountFromText(text);
  if (value > 10) {
    value = 10 + value / 10;
  }
  return value;
}

function premiumError(signup, message) {
  error = signup.find("div.error");
  error.find("p").html(message);
  error.fadeIn();
  signup.find("input[type='button']").attr("disabled", false);
}

function getPremiumAmount() {
  return getPremiumAmountFromText($("div.premium input#amount").val());
}

function submitPremiumForm() {
  var signup = $("div.signup-dialog");
  signup.find("input[type='button']").attr("disabled", true);
  
  var amount = getPremiumAmount();
  log(amount);
  
  $.ajax({
    dataType: "json",
    type: "post",
    url: "?register",
    data: {
      amount: amount,
      name: signup.find("input#name").val(),
      email1: signup.find("input#email1").val(),
      email2: signup.find("input#email2").val(),
      password1: signup.find("input#password1").val(),
      password2: signup.find("input#password2").val()
    },
    success: function(message) {
      log(message);
      if (message.error) {
        premiumError(signup, message.error);
      }
      else {
        var paypal = signup.find("form#paypal");
        paypal.find("input[name='amount']").val(amount);
        paypal.find("input[name='custom']").val(message.userId);
        
        var google = signup.find("form#google");
        google.find("input[name='item_name_1']").val("Synergy Premium ($" + amount + " USD)");
        google.find("input[name='item_description_1']").val(
          "Your Synergy Premium account will still be credited with $" + amount + " USD.");
        google.find("input[name='shopping-cart.merchant-private-data']").val(
          message.userId + "," + amount);
          
        var paypal = signup.find("form#creditcard");
        paypal.find("input[name='amount']").val(amount);
        
        signup.find("div.step1").hide();
        signup.find("div.step2").fadeIn();
      }
    },
    error: function(xhr, textStatus, error) {
      log(xhr.statusText);
      log(textStatus);
      log(error);
      premiumError(signup, "An error occurred while communicating with server.");
    }
  });
}

function initSlider() {
  $("div.slider").slider({
    min: 1,
    value: defaultPremiumSliderIndex,
    slide: function(event, ui) {
      var value = getPremiumValueFromIndex(ui.value);
      $("input#amount").val("$" + value);
    }
  });

  $("input#amount").keyup(function() {
    var index = getPremiumIndexFromText(this.value);
    $("div.slider").slider("value", index);
  });
}

function downloadOptions() {

  if ($("div.download-premium").length == 0) {
    return;
  }
  
  var signup = $("div.signup-dialog");
  
  initSlider();

  $("div.info-dialog").dialog({
    autoOpen: false,
    resizable: false,
    modal: true,
    width: "500px",
  });

  signup.dialog({
    autoOpen: false,
    resizable: false,
    modal: true,
    width: "500px",
  });
  
  $(".ui-dialog-titlebar").hide();

  $("a#show-info").click(function() {
    $("div.info-dialog").dialog("open");
  });

  $("div.info-dialog a.close").click(function() {
    $("div.info-dialog").dialog("close");
  });

  //$("a#show-signup").click(function() {
    var amount = getPremiumAmount();
    signup.find("span#amount2").html("$" + amount.toFixed(2));
    signup.dialog("open");
//  });

// temp
  signup.find("div.step1").hide();
  signup.find("div.step2").fadeIn();

  signup.find("input.cancel").click(function() {
    signup.dialog("close");
  });
  
  signup.find("input.ok").click(function() {
    submitPremiumForm();
  });
  
  $("form#google input[type='image']").click(function() {
    var amount = getPremiumAmount();
    
    $.ajax({
      dataType: "json",
      url: "?currency=" + amount + "USD%3D%3FGBP",
      type: "get",
      success: function(message) {
        log(message);
        $("form#google input[name='item_price_1']").val(message.to);
        $("form#google").submit();
      },
      error: function(xhr, textStatus, error) {
        log(xhr.statusText);
        log(textStatus);
        log(error);
        alert("An error occurred while communicating with the Google currency API.");
      }
    });
    
    return false;
  });
}

function premiumPage() {
  if ($("div.premium-page").length == 0) {
    return;
  }
  
  initSlider();
  
  $("div.contribute input[type='image']").click(function() {
    $("form#paypal input[name='amount']").val(getPremiumAmountFromText($("input#amount").val()));
    log($("form#paypal input[name='amount']"));
  });
}

$(function() {
  downloadOptions();
  premiumPage();
  
  $("form.creditcard input#ok").click(function() {
    $(this).attr('disabled','disabled');
    $(this).val("Please wait...");
    $(this).parent().parent().submit();
  });
});
