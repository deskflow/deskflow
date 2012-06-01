<div class="download">

  <div class="title">
    <div class="logo">
      <img src="/img/icon.png" /><h1 class="title">Synergy</h1>
    </div>
  </div>
  
  <div class="inner-content">
    
    <p><b>{t}Latest release:{/t}</b> {$cur14} {$cur14State} - {$curDate}</p>
    
    <div class="top">
      
      {$custom}
      
      <div class="list">
        <p>{t}Please use version 1.4 for easier setup. Use version 1.3 if you experience problems.{/t}</p>
        <div class="item">
          <h3><img src="/img/windows-icon.png" /> {t}Windows{/t}</h3>
          <p>
            <b>{$cur14} {$cur14State}:</b> <a href="?file=synergy-{$cur14}-Windows-x86.exe">32-bit</a>, <a href="?file=synergy-{$cur14}-Windows-x64.exe">64-bit</a><br />
            <b>{$cur13} {$cur13State}:</b> <a href="?file=synergy-{$cur13}-Windows-x86.exe">32-bit</a>, <a href="?file=synergy-{$cur13}-Windows-x64.exe">64-bit</a>
          </p>
        </div>
        <div class="item">
          <h3><img src="/img/apple-icon.png" /> {t}Mac OS X{/t}</h3>
          <p>
            <b>{$cur14} {$cur14State}:</b> <a href="?file=synergy-{$cur14}-MacOSX107-x86_64.dmg">10.7</a>, <a href="?file=synergy-{$cur14}-MacOSX106-i386.dmg">10.6</a>, <a href="?file=synergy-{$cur14}-MacOSX105-i386.dmg">10.5</a>, <a href="?file=synergy-{$cur14}-MacOSX104-Universal.dmg">10.4</a><br />
            <b>{$cur13} {$cur13State}:</b> <a href="?file=synergy-{$cur13}-MacOSX107-Universal.zip">10.7</a>, <a href="?file=synergy-{$cur13}-MacOSX106-Universal.zip">10.6</a>, <a href="?file=synergy-{$cur13}-MacOSX105-Universal.zip">10.5</a>, <a href="?file=synergy-{$cur13}-MacOSX104-Universal.zip">10.4</a>
          </p>
        </div>
        <div class="item">
          <h3><img src="/img/ubuntu-icon.png" /> <img src="/img/debian-icon.png" /> {t}Ubuntu/Debian{/t}</h3>
          <p>
            <b>{$cur14} {$cur14State}:</b> <a href="?file=synergy-{$cur14}-Linux-i686.deb">32-bit</a>, <a href="?file=synergy-{$cur14}-Linux-x86_64.deb">64-bit</a><br />
            <b>{$cur13} {$cur13State}:</b> <a href="?file=synergy-{$cur13}-Linux-i686.deb">32-bit</a>, <a href="?file=synergy-{$cur13}-Linux-x86_64.deb">64-bit</a>
          </p>
        </div>
        <div class="item">
          <h3><img src="/img/fedora-icon.png" /> <img src="/img/redhat-icon.png" /> {t}Fedora/Red Hat{/t}</h3>
          <p>
            <b>{$cur14} {$cur14State}:</b> <a href="?file=synergy-{$cur14}-Linux-i686.rpm">32-bit</a>, <a href="?file=synergy-{$cur14}-Linux-x86_64.rpm">64-bit</a><br />
            <b>{$cur13} {$cur13State}:</b> <a href="?file=synergy-{$cur13}-Linux-i686.rpm">32-bit</a>, <a href="?file=synergy-{$cur13}-Linux-x86_64.rpm">64-bit</a>
          </p>
        </div>
        <div class="item">
          <h3><img src="/img/source-icon.png" /> {t}Source code{/t}</h3>
          <p>
            <b>{$cur14} {$cur14State}:</b> <a href="?file=synergy-{$cur14}-Source.tar.gz">tar.gz</a><br />
            <b>{$cur13} {$cur13State}:</b> <a href="?file=synergy-{$cur13}-Source.tar.gz">tar.gz</a>
          </p>
        </div>
      </div>
      
    </div>
    
    <div class="alt">
      
      <div class="list">
      
        <h2>{t}Previous 1.4 releases{/t}</h2>
        
        {foreach from=$ver14b item=ver}
        
        <h3>{$ver} {t}Beta{/t}</h3>
        <p>
          <a href="?file=synergy-{$ver}-Windows-x86.exe">{$ver} Windows 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Windows-x64.exe">{$ver} Windows 64-bit</a><br />
          <a href="?file=synergy-{$ver}-MacOSX107-x86_64.dmg">{$ver} Mac OS X 10.7 64-bit</a><br />
          <a href="?file=synergy-{$ver}-MacOSX106-i386.dmg">{$ver} Mac OS X 10.6 32-bit</a><br />
          <a href="?file=synergy-{$ver}-MacOSX105-i386.dmg">{$ver} Mac OS X 10.5 32-bit</a><br />
          <a href="?file=synergy-{$ver}-MacOSX104-Universal.dmg">{$ver} Mac OS X 10.4 Universal</a><br />
          <a href="?file=synergy-{$ver}-Linux-i686.deb">{$ver} Linux (deb) 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-x86_64.deb">{$ver} Linux (deb) 64-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-i686.rpm">{$ver} Linux (rpm) 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-x86_64.rpm">{$ver} Linux (rpm) 64-bit</a><br />
          <a href="?file=synergy-{$ver}-Source.tar.gz">{$ver} Source code</a>
        </p>
        
        {/foreach}
        
        {foreach from=$ver14a item=ver}
        
        <h3>{$ver} {t}Beta{/t}</h3>
        <p>
          <a href="?file=synergy-{$ver}-Windows-x86.exe">{$ver} Windows 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Windows-x64.exe">{$ver} Windows 64-bit</a><br />
          <a href="?file=synergy-{$ver}-MacOSX106-Universal.zip">{$ver} Mac OS X 10.6 Universal</a><br />
          <a href="?file=synergy-{$ver}-MacOSX105-Universal.zip">{$ver} Mac OS X 10.5 Universal</a><br />
          <a href="?file=synergy-{$ver}-MacOSX104-Universal.zip">{$ver} Mac OS X 10.4 Universal</a><br />
          <a href="?file=synergy-{$ver}-Linux-i686.deb">{$ver} Linux (deb) 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-x86_64.deb">{$ver} Linux (deb) 64-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-i686.rpm">{$ver} Linux (rpm) 32-bit</a><br />
          <a href="?file=synergy-{$ver}-Linux-x86_64.rpm">{$ver} Linux (rpm) 64-bit</a><br />
          <a href="?file=synergy-{$ver}-Source.tar.gz">{$ver} Source code</a>
        </p>
        
        {/foreach}

        <h3>1.4.1 {t}Beta{/t}</h3>
        <p><a href="?file=synergy-1.4.1-Windows-x86.exe">1.4.1 Windows 32-bit</a></p>
        
        <h3>1.4.0 {t}Beta{/t}</h3>
        <p><a href="?file=synergy-1.4.0-Windows-x86.exe">1.4.0 Windows 32-bit</a></p>

      </div>
      
      <div class="list">
    
        <h2>{t}Previous 1.3 releases{/t}</h2>

        <h3>1.3.7 {t}Stable{/t}</h3>
        <p>
          <a href="?file=synergy-1.3.7-Windows-x86.exe">1.3.7 Windows 32-bit</a><br />
          <a href="?file=synergy-1.3.7-Windows-x64.exe">1.3.7 Windows 64-bit</a><br />
          <a href="?file=synergy-1.3.7-MacOSX106-Universal.zip">1.3.7 Mac OS X 10.6 Universal</a><br />
          <a href="?file=synergy-1.3.7-MacOSX105-Universal.zip">1.3.7 Mac OS X 10.5 Universal</a><br />
          <a href="?file=synergy-1.3.7-MacOSX104-Universal.zip">1.3.7 Mac OS X 10.4 Universal</a><br />
          <a href="?file=synergy-1.3.7-Linux-i686.deb">1.3.7 Linux (deb) 32-bit</a><br />
          <a href="?file=synergy-1.3.7-Linux-x86_64.deb">1.3.7 Linux (deb) 64-bit</a><br />
          <a href="?file=synergy-1.3.7-Linux-i686.rpm">1.3.7 Linux (rpm) 32-bit</a><br />
          <a href="?file=synergy-1.3.7-Linux-x86_64.rpm">1.3.7 Linux (rpm) 64-bit</a><br />
          <a href="?file=synergy-1.3.7-Source.tar.gz">1.3.7 Source code</a>
        </p>

        <h3>1.3.6 {t}Stable{/t}</h3>
        <p>
          <a href="?file=synergy-1.3.6-Windows-x86.exe">1.3.6 Windows 32-bit</a><br />
          <a href="?file=synergy-1.3.6-Windows-x64.exe">1.3.6 Windows 64-bit</a><br />
          <a href="?file=synergy-1.3.6p2-MacOSX-Universal.zip">1.3.6 Mac OS X Universal</a><br />
          <a href="?file=synergy-1.3.6-Linux-i686.deb">1.3.6 Linux (deb) 32-bit</a><br />
          <a href="?file=synergy-1.3.6-Linux-x86_64.deb">1.3.6 Linux (deb) 64-bit</a><br />
          <a href="?file=synergy-1.3.6-Linux-i686.rpm">1.3.6 Linux (rpm) 32-bit</a><br />
          <a href="?file=synergy-1.3.6-Linux-x86_64.rpm">1.3.6 Linux (rpm) 64-bit</a><br />
          <a href="?file=synergy-1.3.6-Source.tar.gz">1.3.6 Source code</a>
        </p>

        <h3>1.3.4 {t}Stable{/t}</h3>
        <p>
          <a href="?file=synergy-1.3.4-Windows-x86.exe">1.3.4 Windows 32-bit</a><br />
          <a href="?file=synergy-1.3.4-MacOSX-i386.dmg">1.3.4 Mac OS X Intel 32-bit</a><br /><a href="?file=synergy-1.3.4-Linux-i686.deb">1.3.4 Linux (deb) 32-bit</a><br /><a href="?file=synergy-1.3.4-Linux-x86_64.deb">1.3.4 Linux (deb) 64-bit</a><br /><a href="?file=synergy-1.3.4-Linux-i686.rpm">1.3.4 Linux (rpm) 32-bit</a><br /><a href="?file=synergy-1.3.4-Linux-x86_64.rpm">1.3.4 Linux (rpm) 64-bit</a><br /><a href="?file=synergy-1.3.4-Source.tar.gz">1.3.4 Source code</a></p>

        <h3>1.3.3 {t}Stable{/t}</h3>
        <p>
          <a href="?file=synergy-1.3.3-Windows-x86.exe">1.3.3 Windows 32-bit</a></p>

        <h3>1.3.1 {t}Stable{/t}</h3>
        <p>
          <a href="?file=synergy-1.3.1-Windows-x86.exe">1.3.1 Windows 32-bit</a><br />
          <a href="?file=synergy-1.3.1-Linux-i386.rpm">1.3.1 Linux (rpm) 32-bit</a><br /><a href="?file=synergy-1.3.1-MacOSX-i386.tar.gz">1.3.1 Mac OS X Intel 32-bit</a><br /><a href="?file=synergy-1.3.1-Source.tar.gz">1.3.1 Source code</a>
        </p>
        
      </div>

    </div>
    
    <h2>{t}Other projects{/t}</h2>
    <p>{t url="/pm/projects/synergy/wiki/IOS_client"}<a href="%1">iOS client by Matthias Ringwald</a> (iPad/iPhone){/t}</p>

    <h2>{t}Related sites{/t}</h2>
    <p>
      <a href="http://synergy-foss.org/nightly">{t}Nightly builds{/t}</a> (<em>{t}use at your own risk{/t}</em>)<br />
      <a href="http://code.google.com/p/synergy/downloads/list">Google Code - synergy</a><br />
      <a href="http://sourceforge.net/projects/synergy2/files/Binaries/">SourceForge - synergy2</a>
    </p>
  
  </div>

</div>
