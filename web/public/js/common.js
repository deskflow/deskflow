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
  log("text=" + text + " value=" + value);
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
  return getPremiumAmountFromText($("input#amount").val());
}

function postToPayment(url, amount) {
  // send user to secure payment page.
  var tempForm =
    '<form action="' + url + '" method="post">' +
    '<input name="amount" value="' + amount + '" />' +
    '</form>';
  $(tempForm).submit();
}

function submitPremiumForm() {
  var signup = $("div.signup-dialog");
  signup.find("input[type='button']").attr("disabled", true);
  
  var amount = getPremiumAmount();
  log("submit form, amount=" + amount);
  
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
        postToPayment(message.paymentUrl, amount);
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

function initSlider(updateFunc) {
  $("div.slider").slider({
    min: 1,
    value: defaultPremiumSliderIndex,
    slide: function(event, ui) {
      var value = getPremiumValueFromIndex(ui.value);
      $("input#amount").val("$" + value);
      updateFunc();
    }
  });

  $("input#amount").keyup(function() {
    var index = getPremiumIndexFromText(this.value);
    $("div.slider").slider("value", index);
  });
  
  var index = getPremiumIndexFromText($("input#amount").val());
  $("div.slider").slider("value", index);
}

function downloadOptions() {
  
  var signup = $("div.signup-dialog");
  
  initSlider(function() { });

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

  $("a#show-signup").click(function() {
    $.ajax({
      dataType: "json",
      url: "?checkUser",
      type: "get",
      success: function(message) {
        log(message);
        if (message.userId) {
          var amount = getPremiumAmount();
          postToPayment(message.paymentUrl, amount);
        }
        else {
          signup.dialog("open");
        }
      },
      error: function(xhr, textStatus, error) {
        log(xhr.statusText);
        log(textStatus);
        log(error);
        alert("An error occurred while checking for logged in user.");
      }
    });
  });

  signup.find("input.cancel").click(function() {
    signup.dialog("close");
  });
  
  signup.find("input.ok").click(function() {
    submitPremiumForm();
  });
}

function paymentPage() {  
  $("form#creditcard input#ok").click(function() {
    $(this).attr('disabled','disabled');
    $(this).val("Please wait...");
    $(this).parent().parent().submit();
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
  
  initSlider(function() { updateAmounts() });
  $("input#amount").change(function() { updateAmounts() });
  updateAmounts();
}

function updateAmounts() {
  var userId = $("input[name='userId']").val();
  var amount = getPremiumAmountFromText($("input#amount").val());
  log("update amounts, amount=" + amount);

  $("form#paypal input[name='amount']").val(amount);
  $("form#creditcard input[name='amount']").val(amount);

  var title = "Synergy Premium ($" + amount + " USD)";
  var info = "Your Synergy Premium account will be credited with $" + amount + " USD.";
  var custom = userId + "," + amount;
  
  var google = $("form#google");
  google.find("input[name='item_name_1']").val(title);
  google.find("input[name='item_description_1']").val(info);
  google.find("input[name='shopping-cart.merchant-private-data']").val(custom);
}

$(function() {
  if ($("div.download-premium").length != 0) {
    downloadOptions();
  }
  else if ($("div.premium-payment").length != 0) {
    paymentPage();
  }
});
