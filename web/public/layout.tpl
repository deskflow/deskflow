<html lang="{$lang}" xml:lang="{$lang}">
  <head>
    <title>{$title}</title>
    <meta http-equiv="X-UA-Compatible" content="IE=9" />
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
    <meta http-equiv="Content-Language" content="{$lang}"/>
    <meta name="description" content="{t}Synergy lets you share your keyboard and mouse between multiple computers on your desk.{/t}" />
    <link rel="stylesheet" type="text/css" href="/css/main.css" />
    <link rel="stylesheet" type="text/css" href="/css/main-{$langDir}.css" />
    <script type="text/javascript" src="/js/jquery-1.7.2.min.js"></script>
    <script type="text/javascript" src="/js/common.js"></script>
    <script type="text/javascript">

      var _gaq = _gaq || [];
      _gaq.push(['_setAccount', 'UA-10292290-10']);
      _gaq.push(['_setDomainName', 'synergy-foss.org']);
      _gaq.push(['_trackPageview']);

      (function() {
        var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
        ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
        var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
      })();

    </script>
    <script type="text/javascript">
    /* <![CDATA[ */
        (function() {
            var s = document.createElement('script'), t = document.getElementsByTagName('script')[0];

            s.type = 'text/javascript';
            s.async = true;
            s.src = 'http://api.flattr.com/js/0.6/load.js?mode=auto';

            t.parentNode.insertBefore(s, t);
        })();
    /* ]]> */
    </script>
  </head>
  <body>
    <div class="layout">
      <div class="menu">
        <div class="pages">
          <a href="{$baseUrl}/">{t}Home{/t}</a> |
          <a href="{$baseUrl}/download/">{t}Download{/t}</a> |
          <a href="{$baseUrl}/help/">{t}Help{/t}</a> |
          <a href="{$baseUrl}/search/">{t}Search{/t}</a> |
          <a href="{$baseUrl}/info/">{t}Info{/t}</a> |
          <a href="{$baseUrl}/corporate/">{t}Corporate{/t}</a> |
          <a href="/tracker/">{t}Tracker{/t}</a> |
          <a href="/tracker/projects/synergy/wiki/DevInfo">{t}Developer{/t}</a>
        </div>
      </div>
      <div class="content">
        {$content}
      </div>
      <div class="footer">
        {if !$langIsEnglish}
        <p><a href="http://www.getlocalization.com/synergy/">{t}Improve this translation{/t}</a></p>
        {/if}
        <p class="lang">
          <a href="/?hl=en">English</a>,
          <a href="/?hl=cs">Čeština</a>,
          <a href="/?hl=cy">Cymraeg</a>,
          <a href="/?hl=de">Deutsch</a>,
          <a href="/?hl=es">Español</a>,
          <a href="/?hl=fr">Français</a>,
          <a href="/?hl=it">Italiano</a>,
          <a href="/?hl=nl">Nederlands</a>,
          <a href="/?hl=no">Norsk</a>,
          <a href="/?hl=pl">Polski</a>,
          <a href="/?hl=pt">Português</a>,
          <a href="/?hl=ro">Română</a>,
          <a href="/?hl=sl">Slovenščina</a>,
          <a href="/?hl=fi">Suomi</a>,
          <a href="/?hl=sv">Svenska</a>,
          <a href="/?hl=vi">Tiếng Việt</a>,
          <a href="/?hl=ru">Русский</a>,
          <a href="/?hl=uk">Український</a>,
          <a href="/?hl=he">עברית</a>,
          <a href="/?hl=ar">العربية</a>,
          <a href="/?hl=th">ภาษาไทย</a>,
          <a href="/?hl=zh">中文 (简体)</a>,
          <a href="/?hl=zh-tw">中国 (台湾)</a>,
          <a href="/?hl=ja">日本語</a>,
          <a href="/?hl=ko">한국의</a>
        </p>
      </div>
    </div>
    <!-- Quantcast Tag -->
    <script type="text/javascript">
    var _qevents = _qevents || [];

    (function() {
    var elem = document.createElement('script');
    elem.src = (document.location.protocol == "https:" ? "https://secure" : "http://edge") + ".quantserve.com/quant.js";
    elem.async = true;
    elem.type = "text/javascript";
    var scpt = document.getElementsByTagName('script')[0];
    scpt.parentNode.insertBefore(elem, scpt);
    })();

    _qevents.push({
    qacct:"p-GZM8r2V0pKfSk"
    });
    </script>

    <noscript>
    <div style="display:none;">
    <img src="//pixel.quantserve.com/pixel/p-GZM8r2V0pKfSk.gif" border="0" height="1" width="1" alt="Quantcast"/>
    </div>
    </noscript>
    <!-- End Quantcast tag -->
  </body>
</html>
