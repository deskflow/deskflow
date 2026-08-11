# GUI Config

 Deskflow will automatically figure out where to save settings and other files.

## Search paths

Deskflow will look for settings in several places depending on your operating system.
The search order for a setting file depends on your operating system

### Linux

  1. `<XDG_CONFIG_HOME>/Deskflow/Deskflow.conf`
  2. `~/.config/Deskflow/Deskflow.conf`
  3. `/etc/Deskflow/Deskflow.conf`
 
A new settings file will be created in the user path if no settings file is found.
The path of the settings file will be used as the base for all other config files.

### macOS
 
  1. `~/Library/Deskflow/Deskflow.conf`
  2. `/Library/Deskflow/Deskflow.conf`
 
A new settings file will be created in the user path if no settings file is found.
The path of the settings file will be used as the base for all other config files.

### Windows

  1. `<install-path>/settings/Deskflow.conf`
  2. Windows Registry `HKCU\Software\Deskflow\Deskflow`

Windows will save to the install dir if settings are loaded from there. If not, it saves any other config files in: `C:\ProgramData\Deskflow\`

When using settings from the install dir, the service mode will not be available.

## Valid GUI Keys

The GUI config file contains several sections.
Each section is formatted the same.
Option-value pairs are only written if the value is not the default value.

```
[section]
option=value
```

### Client

This section contains options used when in client mode. 
It will begin with `[client]`

| Option                    |    Valid Values    | Description |
|:--------------------------|:------------------:|:-----------|
| dynamicConnectionInterval | `true` or `false`  | Use dynamic connection retry times based on number of previously failed attempts [default: false] |
| languageSync              | `true` or `false`  | Sync to server language [default: true] |
| remoteHost                | `IP` or `hostname` | The remote host(s) to connect to. Use a comma separated list when you want to try several hosts |
| yScrollScale              | Double 0.1 - 10.0  | Vertical mouse scrolling will be scaled by this amount on the client [default: 1.0] |
| xScrollScale              | Double 0.1 - 10.0  | Horizontal mouse scrolling will be scaled by this amount on the client [default: 1.0] |
| invertYScroll             | `true` or `false`  | Invert vertical scroll on this client [default: false] |
| invertXScroll             | `true` or `false`  | Invert horizontal scroll on this client [default: false] |

### Core

This section contains general options it will begin with `[core]`

|Option         | Valid Values|Description|
|:--------------|:-----------:|:-----------|
| coreMode      | `0` or `1` or `2` | The mode to start in 0: None, 1: Client, 2: Server [default: 0]|
| display       |  int              | The XWindow display to use [default: autodetected] |
| interface     | IP Address        | Preferred IP to use for network communication. By default the server board casts on any available address |
| lastVersion   | M.m.p.t           | The version last run used for checking for updates |
| port          | port #            | Port to use when connecting [default: 24800 |
| preventSleep  | `true` or `false` | Prevent sleep when Deskflow is active [default: false] |
| processMode   | `1` or `0`        | The mode we use to start the process Service or Desktop |
| computerName  | string            | Name used to identify the computer [default: machine's hostname] |
| useHooks      | `true` or `false` | If Windows uses hooks or not [default: true] |
| language      | 639 language      | The language to display the GUI in [default: en] |
| enableEnterCommand | `true` or `false` | Should the enter command be triggered when the screen is entered [defaut: false] |
| enterCommand  | command | A command to run when the screen is entered. |
| enableExitCommand | `true` or `false` | Should the exit command be triggered when the screen is exited [defaut: false] |
| exitCommand  | command | A command to run when the screen is exited. |

### Daemon

This section contains options used by the daemon on windows it will begin with `[daemon]`

|Option | Valid Values|Description|
|:----------|:-----------:|:-----------|
| command   | Filename          | The filename of the binary the daemon. This binary exists in the same path as the deskflow GUI |
| elevate   | `true` or `false` | Elevate the daemon app [default: true unless portable mode ] |
| logFile   | Filepath          | Filepath of the daemon log |
| logLevel  | valid log Level,  | Log Level  |

### GUI

This section contains options used by the GUI it will begin with `[gui]`

|Option                          | Valid Values      |Description|
|:-------------------------------|:-----------------:|:-----------|
| autoHide                       | `true` or `false` | When true the app will hide itself on start up [default: false] |
| enableUpdateCheck              | `true` or `false` | When true check the update URL to see if a new version was released on start up [default: false] |
| closeReminder                  | `true` or `false` | Used to track if we have shown the reminder that when you close the app it remain running in the background  [default: true]|
| closeToTray                    | `true` or `false` | When `true` the gui will run in the systemTray when its closed [default: true] |
| logExpanded                    | `true` or `false` | Should the log section of the GUI be opened [default: false] |
| symbolicTrayIcon               | `true` or `false` | When true use the monocolor (symbolic) icon false uses a colorful icon for the tray [default: true] |
| showGenericClientFailureDialog | `true` or `false` | When `true` client connection errors will not show popup error messages [default: true] |
| shownFirstConnectedMessage     | `true` or `false` | When `true` GUI has shown the user the message for connecting the first time [default: false] |
| shownServerFirstStartMessage   | `true` or `false` | When `true` GUI has shown the user the Deskflow server is now running message [default: false] |
| shownVerionInTitle             | `true` or `false` | When `true` GUI will include the version in the window title [default: false] |
| startCoreWithGui               | `true` or `false` | When true the Core will be started with the GUI. It is set to the Core's state on exit. |
| updateCheckUrl                 | URL               | The URL to use when checking for a new version number, it should return a version [default: https://api.deskflow.org/version]|

### Log

This section contains options used by the application logging it will begin with `[log]`

|Option    |    Valid Values   |Description|
|:---------|:-----------------:|:-----------|
| file     | Filepath          | The file to write the log into |
| level    | Valid log level   | Log level to use |
| toFile   | `true` or `false` | When true the log will be written to the value of the `file` option |
| guiDebug | `true` or `false` | When true the log will show the Gui's internal debug messages |

### Security

This section contains options used by the application security it will begin with `[security]`

|Option                 |   Valid Values    |Description|
|:----------------------|:-----------------:|:-----------|
| checkPeerFingerprints | `true` or `false` | When true peers will have their fingerprints confirmed by the user and stored [default: true] |
| certificate           | Filepath          | Path to the certificate to use to encrypt messages.|
| keySize               | `2048` OR `4096`  | Size of the TLS key to use [default: 2048]| 
| tlsEnabled            | `true` or `false` | Are we using TLS encryption when communicating [default: true].|

### Server

This section contains options used when in server mode it will begin with `[server]`

|Option              |    Valid Values   |Description|
|:-------------------|:-----------------:|:-----------|
| clipboardSize      | int > 0           | Deskflow will send a maximum of `N` megabytes of clipboard data to another computer when the mouse transitions to that computer.|
| defaultLockToComputerState| `true` or `false` | When this is true the cursor is locked to the new computer when switching (default: false)|
| disableLockToComputer| `true` or `false` | If false pressing scroll lock will toggle your cursor to be locked to current computer. (default: false) |
| enableClipboard    | `true` or `false` | When `true` the clipboard will be shared with all clients If set to ''true'' then clipboard shared and the ''clipboardSharingSize'' setting will be used. If set to false, then clipboard sharing will be disabled and the the ''clipboardSharingSize'' setting will be ignored.|
| enableHeartbeat    | `true` or `false` | Send a heartbeat to connected clients; this has been replaced by internal keep alive (default: false)|
| enableSwitchDelay  | `true` or `false` | Switching will be delayed by the set value (default: false)|
| enableSwitchDoubleTap  | `true` or `false` | Enables the doubletap to switch method (default: false)|
| gridHeight         | int               | Height of the server's intenal grid used for the computer layout (default: 3)|
| gridWidth          | int               | Width of the server's intenal grid used for the computer layout (default: 5) |
| heartbeat          | int               | The server will expect each client to send a message no less than every `N` milliseconds. If no message arrives from a client within `3N` seconds the server forces that client to disconnect. If deskflow fails to detect clients disconnecting while the server is sleeping or vice versa, try using this option.|
| protocol           | `barrier` or `synergy` | The protocol to use when saying hello to clients. Can be set to barrier or synergy. If not set barrier is used as the default |
|relativeMouseMoves  | `true` or `false` | If set to ''true'' then secondary computers move the mouse using relative rather than absolute mouse moves when and only when the cursor is locked to the computer (by ''Scroll Lock'' or a configured hot key). This is intended to make Deskflow work better with certain games. If set to ''false'' or not set then all mouse moves are absolute.|
| switchDelay        | int               | Deskflow won't switch computers when the mouse reaches edge of a computer unless it stays on the edge for `N` milliseconds. This helps prevent unintentional switching when working near an edge. (default: 250)|
| switchDoubleTap    | int               | Deskflow won't switch computers when the mouse reaches the edge of a computer unless it's moved away from the edge and then back to the edge within `N` milliseconds. With the option you have to quickly tap the edge twice to switch. This helps prevent unintentional switching when working near the edge.|
|win32KeepForeground | `true` or `false` | If set to ''true'' (the default), Deskflow will grab the foreground focus on a Windows server (thereby putting all other windows in the background) upon switching to a client. If set to ''false'', it will leave the currently foreground window in the foreground. Deskflow grabs the focus to avoid issues with other apps interfering with Deskflow's ability to read the hardware inputs. |

 - You can use both the ''switchDelay'' and ''switchDoubleTap'' options at the same time. Deskflow will switch when either requirement is satisfied.

### Screen Settings

Each screen will have a section where its configuration will be stored, if the screen was named "foo" the section will be named `[screen_foo]`

|Option              |    Valid Values    |Description|
|:-------------------|:------------------:|:-----------|
| aliases            | Comma separated list of hostnames | Names here will be used as alternatives for the computer. Names must be valid hostnames. |


### InternalConfig

This section contains the server layout (screens and hotkeys) and begins with `[internalConfig]`.
It is written by the GUI's server configuration dialog and read directly by `deskflow-core` when
running as a server. The `screens` array maps onto the layout grid (see `server/gridWidth` and
`server/gridHeight`): the array index is the grid cell, and adjacent non-empty cells are linked.

```
[internalConfig]
hotkeys\1\actions\1\activeOnRelease=false
hotkeys\1\actions\1\hasScreens=true
hotkeys\1\actions\1\keys\1\key=83
hotkeys\1\actions\1\keys\size=1
hotkeys\1\actions\1\lockCursorToScreen=0
hotkeys\1\actions\1\restartServer=false
hotkeys\1\actions\1\switchInDirection=0
hotkeys\1\actions\1\switchScreenName=void
hotkeys\1\actions\1\type=0
hotkeys\1\actions\1\typeScreenNames\size=0
hotkeys\1\actions\size=1
hotkeys\1\keys\1\key=83
hotkeys\1\keys\size=1
hotkeys\size=1
screens\1\name=
screens\10\fixArray\1\fix=false
screens\10\fixArray\2\fix=false
screens\10\fixArray\3\fix=false
screens\10\fixArray\4\fix=false
screens\10\fixArray\size=4
screens\10\modifierArray\1\modifier=0
screens\10\modifierArray\2\modifier=1
screens\10\modifierArray\3\modifier=2
screens\10\modifierArray\4\modifier=3
screens\10\modifierArray\5\modifier=4
screens\10\modifierArray\6\modifier=5
screens\10\modifierArray\size=6
screens\10\name=null
screens\10\switchCornerArray\1\switchCorner=false
screens\10\switchCornerArray\2\switchCorner=false
screens\10\switchCornerArray\3\switchCorner=false
screens\10\switchCornerArray\4\switchCorner=false
screens\10\switchCornerArray\size=4
screens\10\switchCornerSize=0
screens\11\name=
screens\12\name=
screens\13\name=
screens\14\name=
screens\15\name=
screens\2\name=
screens\3\name=
screens\4\name=
screens\5\name=
screens\6\name=
screens\7\fixArray\1\fix=false
screens\7\fixArray\2\fix=false
screens\7\fixArray\3\fix=false
screens\7\fixArray\4\fix=false
screens\7\fixArray\size=4
screens\7\modifierArray\1\modifier=0
screens\7\modifierArray\2\modifier=1
screens\7\modifierArray\3\modifier=2
screens\7\modifierArray\4\modifier=3
screens\7\modifierArray\5\modifier=4
screens\7\modifierArray\6\modifier=5
screens\7\modifierArray\size=6
screens\7\name=void
screens\7\switchCornerArray\1\switchCorner=false
screens\7\switchCornerArray\2\switchCorner=false
screens\7\switchCornerArray\3\switchCorner=false
screens\7\switchCornerArray\4\switchCorner=false
screens\7\switchCornerArray\size=4
screens\7\switchCornerSize=0
screens\8\fixArray\1\fix=false
screens\8\fixArray\2\fix=false
screens\8\fixArray\3\fix=false
screens\8\fixArray\4\fix=false
screens\8\fixArray\size=4
screens\8\modifierArray\1\modifier=0
screens\8\modifierArray\2\modifier=1
screens\8\modifierArray\3\modifier=2
screens\8\modifierArray\4\modifier=3
screens\8\modifierArray\5\modifier=4
screens\8\modifierArray\6\modifier=5
screens\8\modifierArray\size=6
screens\8\name=chris-Precision-5570
screens\8\switchCornerArray\1\switchCorner=false
screens\8\switchCornerArray\2\switchCorner=false
screens\8\switchCornerArray\3\switchCorner=false
screens\8\switchCornerArray\4\switchCorner=false
screens\8\switchCornerArray\size=4
screens\8\switchCornerSize=0
screens\9\fixArray\1\fix=false
screens\9\fixArray\2\fix=false
screens\9\fixArray\3\fix=false
screens\9\fixArray\4\fix=false
screens\9\fixArray\size=4
screens\9\modifierArray\1\modifier=0
screens\9\modifierArray\2\modifier=1
screens\9\modifierArray\3\modifier=2
screens\9\modifierArray\4\modifier=3
screens\9\modifierArray\5\modifier=4
screens\9\modifierArray\6\modifier=5
screens\9\modifierArray\size=6
screens\9\name=abyss.lan
screens\9\switchCornerArray\1\switchCorner=false
screens\9\switchCornerArray\2\switchCorner=false
screens\9\switchCornerArray\3\switchCorner=false
screens\9\switchCornerArray\4\switchCorner=false
screens\9\switchCornerArray\size=4
screens\9\switchCornerSize=0
screens\size=15
```


# Server Config

Older versions generated a separate server configuration file (`deskflow-server.conf`) in the
synergy.conf text format, and `deskflow-server` could load a hand-written file with
`-c`/`--config`. That file format has been removed: the server now builds its configuration
directly from the settings file described above (`[server]`, `[internalConfig]`, and
`[screen_<name>]` sections). Use the GUI's server configuration dialog to arrange screens,
aliases, and hotkeys.
