<?php /* Smarty version Smarty-3.1.8, created on 2012-03-13 03:22:49
         compiled from "layout.tpl" */ ?>
<?php /*%%SmartyHeaderCode:25384f5ea7493b40d4-25180318%%*/if(!defined('SMARTY_DIR')) exit('no direct access allowed');
$_valid = $_smarty_tpl->decodeProperties(array (
  'file_dependency' => 
  array (
    '20948a66d0c8039cfa4e5dcc8bb02fa9741ed0b3' => 
    array (
      0 => 'layout.tpl',
      1 => 1331608888,
      2 => 'file',
    ),
  ),
  'nocache_hash' => '25384f5ea7493b40d4-25180318',
  'function' => 
  array (
  ),
  'version' => 'Smarty-3.1.8',
  'unifunc' => 'content_4f5ea7493cd669_20442712',
  'variables' => 
  array (
    'title' => 0,
    'content' => 0,
    'url' => 0,
  ),
  'has_nocache_code' => false,
),false); /*/%%SmartyHeaderCode%%*/?>
<?php if ($_valid && !is_callable('content_4f5ea7493cd669_20442712')) {function content_4f5ea7493cd669_20442712($_smarty_tpl) {?><?php if (!is_callable('smarty_block_t')) include 'C:\\Projects\\synergy\\web\\public\\smarty\\libs\\plugins\\block.t.php';
?>﻿<html>
  <head>
    <title>Synergy<?php echo $_smarty_tpl->tpl_vars['title']->value;?>
</title>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="description" content="<?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Synergy lets you share your keyboard and mouse between multiple computers on your desk.<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
" />
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
          <a href="/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Home<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a> |
          <a href="/download/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Download<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a> |
          <a href="/info/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Info<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a> |
          <a href="/support/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Support<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a> |
          <a href="/search/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Search<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a> |
          <a href="/tracker/"><?php $_smarty_tpl->smarty->_tag_stack[] = array('t', array()); $_block_repeat=true; echo smarty_block_t(array(), null, $_smarty_tpl, $_block_repeat);while ($_block_repeat) { ob_start();?>
Tracker<?php $_block_content = ob_get_clean(); $_block_repeat=false; echo smarty_block_t(array(), $_block_content, $_smarty_tpl, $_block_repeat);  } array_pop($_smarty_tpl->smarty->_tag_stack);?>
</a>
        </div>
      </div>
      <div class="content">
        <?php echo $_smarty_tpl->tpl_vars['content']->value;?>

      </div>
      <div class="footer">
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=en_US">English</a> |
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=de_DE">Deutsch</a> |
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=fr_FR">Français</a> |
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=ja_JP">日本語</a> |
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=zh_CN">中文 (简体)</a> |
        <a href="<?php echo $_smarty_tpl->tpl_vars['url']->value;?>
?hl=ru_RU">Русский</a>
      </div>
    </div>
  </body>
</html>
<?php }} ?>