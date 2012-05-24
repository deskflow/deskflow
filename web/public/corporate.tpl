<div class="corporate">

  <div class="title">
    <div class="logo">
      <img src="/img/icon.png" /><h1 class="title">Synergy</h1>
    </div>
  </div>
  
  <div class="inner-content">

    <h2>{t}Corporate{/t}</h2>
    
    {if $messageSent}
    
    <p>
{t}Thanks for contacting us, we'll get back to you as soon as we read
your message!{/t}
    </p>
    
    {else}
    
    <p>
{t}If you are a user within a corporation and have the authority
to pay for product support, please contact us by using the form
below. Thank you for using Synergy!{/t}
    </p>
    
    <form method="post" id="corporate">
      <div class="errors"><p></p></div>
      <div class="box">
        <p>
          <label for="company">Company name:</label>
          <input id="company" name="company" type="text" />
        </p>
        <p>
          <label for="name">Your name:</label>
          <input id="name" name="name" type="text" />
        </p>
        <p>
          <label for="email1">Email address:</label>
          <input id="email1" name="email1" type="text" />
        </p>
        <p>
          <label for="email2">Confirm email:</label>
          <input id="email2" name="email2" type="text" />
        </p>
        <p>
          <label for="phone">Phone number:</label>
          <input id="phone" name="phone" type="text" />
        </p>
        <p>
          <label for="details">Details:</label>
          <textarea id="details" name="details"></textarea>
        </p>
      </div>
      <div class="buttons">
        <input type="submit" value="Send" />
      </div>
    </form>
    
    {/if}
    
  </div>

</div>
