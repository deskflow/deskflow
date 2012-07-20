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

$(function() {
  premiumForm();
});
