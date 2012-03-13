<html>
  <head>
    <title>Synergy{$title}</title>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="description" content="{t}Synergy lets you share your keyboard and mouse between multiple computers on your desk.{/t}" />
    <link rel="stylesheet" type="text/css" href="/main.css" />
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
          <a href="/">{t}Home{/t}</a> |
          <a href="/download/">{t}Download{/t}</a> |
          <a href="/info/">{t}Info{/t}</a> |
          <a href="/support/">{t}Support{/t}</a> |
          <a href="/search/">{t}Search{/t}</a> |
          <a href="/tracker/">{t}Tracker{/t}</a>
        </div>
      </div>
      <div class="content">
        {$content}
      </div>
      <div class="footer">
        <a href="{$url}?hl=en_US">English</a> |
        <a href="{$url}?hl=de_DE">Deutsch</a> |
        <a href="{$url}?hl=fr_FR">Français</a> |
        <a href="{$url}?hl=ja_JP">日本語</a> |
        <a href="{$url}?hl=zh_CN">中文 (简体)</a> |
        <a href="{$url}?hl=ru_RU">Русский</a>
      </div>
    </div>
  </body>
</html>
