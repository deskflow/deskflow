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

function log(s) {
  if ("console" in self && "log" in console) console.log(s);
}

function formError(errors, error) {
  errors.fadeIn();
  errors.find("p").html("Form error: " + error);
}

function premiumForm() {

  form = $("form#premium");
  errors = $("form div.errors");
  
  if (form) {
    form.find("div.buttons input").click(function() {
      email1 = form.find("#email1").val();
      if (form.find("#company").val() == "") {
        formError(errors, "Company missing.");
        return false;
      }
      else if (form.find("#name").val() == "") {
        formError(errors, "Your name is missing.");
        return false;
      }
      else if (email1 == "") {
        formError(errors, "Email address is missing.");
        return false;
      }
      else if (email1.indexOf("@") == -1) {
        formError(errors, "Email address is invalid.");
        return false;
      }
      else if (email1 != form.find("#email2").val()) {
        formError(errors, "Confirm email field does not match.");
        return false;
      }
      else if (form.find("#human").val().toLowerCase() != "yes") {
        formError(errors, "No robots allowed (please say \"yes\").");
        return false;
      }
    });
  }
  
  errors.fadeOut();
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
  var valueArray = /\d+/.exec(text);
  if (valueArray == null) {
    return 10;
  }
  return parseInt(valueArray[0]);
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

function submitPremiumForm() {
  var signup = $("div.signup-dialog");
  signup.find("input[type='button']").attr("disabled", true);
  
  $.ajax({
    dataType: "json",
    type: "post",
    url: "?register",
    data: {
      amount: signup.find("input#amount2").val(),
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
        var amount = getPremiumAmountFromText($("div.premium input#amount").val());
        signup.find("input#amount3").val(amount);
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

function downloadOptions() {

  if ($("div.download-premium").length == 0) {
    return;
  }
  
  var signup = $("div.signup-dialog");
  
  $("div.premium div.slider").slider({
    min: 1,
    value: 10,
    slide: function(event, ui) {
      var value = getPremiumValueFromIndex(ui.value);
      $("div.premium input#amount").val("$" + value);
    }
  });

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

  $("div.premium input#amount").keyup(function() {
    var index = getPremiumIndexFromText(this.value);
    $("div.premium div.slider").slider("value", index);
  });

  $("a#show-info").click(function() {
    $("div.info-dialog").dialog("open");
  });

  $("div.info-dialog a.close").click(function() {
    $("div.info-dialog").dialog("close");
  });

  $("a#show-signup").click(function() {
    signup.find("span#amount2").html($("div.premium input#amount").val());
    signup.dialog("open");
  });

  signup.find("input.cancel").click(function() {
    signup.dialog("close");
  });
  
  signup.find("input.ok").click(function() {
    submitPremiumForm();
  });
}

$(function() {
  premiumForm();
  downloadOptions();
});
