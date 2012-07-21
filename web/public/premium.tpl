<div class="premium">

  <div class="title">
    <div class="logo">
      <img src="/img/icon.png" /><h1 class="title">Synergy</h1>
    </div>
  </div>
  
  <div class="inner-content">
  
    <h2>{t}Synergy Premium{/t}</h2>
    
    {if $messageSent}
    
    <p>{t}Thank you for your interest!{/t}</p>
    
    {else}

    <p>
{t}We want to introduce a new special version of Synergy, called
<b>Synergy Premium</b>.{/t}
    </p>
    
    <p>
{t}Signing up allows you to have a direct effect on the improvement of Synergy.
Contributions allow us to spend more time and resources fixing bugs and adding
new features, but we think that you should get something in return.{/t}
    </p>

    <ul>
      <li>{t}For every $10 you contribute, you get a premium vote.{/t}</li>
      <li>{t}You can assign your votes to bugs or features.{/t}</li>
      <li>{t}The highest voted bug or feature is looked at next.{/t}</li>
    </ul>
    
    <h3>{t}Are you interested?{/t}</h3>
    <p>{t}Please complete this form, we will not spam your email address.{/t}</p>
    
    <form method="post" id="premium">
      <div class="errors"><p></p></div>
      <div class="box">
        <p>
          <label for="name">{t}Your name:{/t}</label>
          <input id="name" name="name" type="text" />
        </p>
        <p>
          <label for="email1">{t}Email address:{/t}</label>
          <input id="email1" name="email1" type="text" />
        </p>
        <p>
          <label for="email2">{t}Confirm email:{/t}</label>
          <input id="email2" name="email2" type="text" />
        </p>
        <p>
          <label for="comments">{t}Comments:{/t}</label>
          <textarea id="comments" name="comments"></textarea>
        </p>
        <p>
          <label for="human">{t}Are you human?{/t}</label>
          <input id="human" name="human" type="text" />
          {t}Please say <b>yes</b> (in English).{/t}
        </p>
      </div>
      <div class="buttons">
        <input type="submit" value="{t}OK{/t}" />
      </div>
    </form>
    
    {/if}

</div>
