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
{t}We believe software should be free, but not just free as in "free beer",
free as in "free speech" (to quote Richard Stallman).{/t}
    </p>

    <p>
{t}We also believe that funding free software is a good thing. Funding
(no matter how small) allows free software organizations to spend more time
and resources improving software, fixing bugs and adding new features.{/t}
    </p>

    <p>
{t}We want to introduce a new special version of Synergy, called
<b>Synergy Premium</b>.{/t}
    </p>

    <p><b>{t}You will get:{/t}</b></p>
    <ul>
      <li>{t}Access to the premium support channel.{/t}</li>
      <li>{t}Your name on the about screen and this page.{/t}</li>
      <li>{t}The satisfaction of knowing you improved Synergy.{/t}</li>
    </ul>

    <p><b>{t}What does it cost?{/t}</b></p>
    <ul>
      <li>{t}At least $10, but you set the maximum.{/t}</li>
      <li>{t}It can be a one-off or a subscription, your choice.{/t}</li>
    </ul>
    
    <h3>{t}Are you interested?{/t}</h3>
    <p>{t}Please let us know you are interested by completing this form.{/t}</p>
    
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
