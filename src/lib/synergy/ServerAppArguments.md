# ServerApp - List of Command Line Arguments

As obtained by ```parseServerArgs()```

## Common Arguments

As obtained by ```updateCommonArgs()```

*m_name*
The name of the host as reported by the platform.

*m_pname*
Name of the process (synergys\[.exe])

## Platform Arguments

### Windows

**--service** (deprecated, the program ends if specified)

**--exit-pause**
*m_pauseOnExit*

Will wait for a key to be pressed before ending execution.

**--stop-on-desk-switch**
*m_stopOnDeskSwitch*

Passed-in to the server screen at its creation. Shuts down the service when the cursor crosses over.

### X-Windows

**--display**
*m_display*
Identifies the X server to work on.

**--no-xinitthreads**
*m_disableXInitThreads*

Passed-in to the server screen at creation. Avoids calling XInitThreads at the screen constructor.

## Generic Arguments

As collected by ```parseGenericArgs()```

**-d** / **--debug**
*m_logFilter* (string)

If present, one value out of the following strings (each string contains all previous information levels): 
    "FATAL",
    "ERROR",
    "WARNING",
    "NOTE",
    "INFO",
    "DEBUG",
    "DEBUG1",
    "DEBUG2",
    "DEBUG3",
    "DEBUG4",
    "DEBUG5".

**-l** / **--log**
*m_logFile*

Uses FileLogOutputter to send log to that file. The file will be open/closed for each write operation. When reaching 1Mb, the file will be renamed with the same name +".1".

**-f** / **--no-daemon**
*m_daemon* false

**--daemon**
*m_daemon* true

With m_daemon true, the main loop will be wrapped around a call to "daemonise" and a system logger created.

**-n** / **--name**
*m_name* (Screen name)

Used to identify the server's screen.

**-1** / **--no-restart**
*m_restartable* false

**--restart**
*m_restartable* true

With this flag true, ```initServer()``` and ```startServer()``` will setup a one-time timer on the queue for restarting in case of any failures.
The time to wait is 10 seconds in the case of the particular issue "XSocketAddressInUse" but in all other cases will be zero.

**--no-hooks**
*m_noHooks* true

Applies only to MS Windows, avoids using hooks.

**--help**

Shows help.

**--version**

Shows version, then exits.

**--no-tray**
m_disableTray true

Avoids the creation of a task bar receiver.

**--ipc**
m_enableIpc true

Implements the event queue over IPC.

**--server**
**--client**
(accepted but ignored)

**--enable-drag-drop**
*m_enableDragDrop*

Ignored for XWindows, or MS Windows below Vista. It enables steps required for drag and drop.

**--enable-crypto**
*m_enableCrypto* true

Enables secure data sockets.

**--profile-dir**
*m_profileDirectory*

If the profile directory is not passed-in, then it is inferred. For XWindows as "~/.synergy" or else "~/Library/Synergy".

**--plugin-dir**
*m_pluginDirectory*

If unspecified, it is inferred. For XWindows "~/plugins" or else "~/Plugins".

**--tls-cert**
*m_tlsCertFile*

If unspecified and used, then it is sought for as prifleDirectory/SSL/Synergy.pem

## Uncategorised

**-a** / **--address**
*m_synergyAddress*

Used as the listening address.

**-c** / **--config**
*m_configFile*

Configuration file path.

"" / **--serial-key**
*m_serial*

Serial key.

## Deprecated

(accepted but effectively ignored)

**--crypto-pass**

**--res-w**

**--res-h**

**--prm-wc**

**--prm-hc**
