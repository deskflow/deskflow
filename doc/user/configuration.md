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


| Option                |    Valid Values    | Description |
|:----------------------|:------------------:|:-----------|
| binary                | Filename           | The filename of the binary to call for client mode. This binary exists in the same path as the GUI |
| invertScrollDirection | `true` or `false`  | Invert scroll on this client [default: false] |
| languageSync          | `true` or `false`  | Sync to server language [default: true] |
| remoteHost            | `IP` or `hostname` | The remote host last connected to | 
| xdpRestoreToken       | UUID               | Restore token provided by XDG portals |


### Core
This section contains general options it will begin with `[core]`

|Option         | Valid Values|Description|
|:--------------|:-----------:|:-----------|
| coreMode      | `0` or `1` or `2` | The mode to start in 0: None, 1: Client, 2: Server [default: 0]|
| interface     | IP Address        | Preferred IP to use for network communication. By default the server board casts on any available address |
| lastVersion   | M.m.p.t           | The version last run used for checking for updates |
| port          | port #            | Port to use when connecting [default: 24800 |
| preventSleep  | `true` or `false` | Prevent sleep when Deskflow is active [default: false] |
| processMode   | `1` or `0`        | The mode we use to start the process Service or Desktop |
| screenName    | string            | Name used to identify the screen [default: machine's hostname] |
| startedBefore | `true` or `false `| Have we started client or server before. Used in logic when deciding to show some dialogs.
| updateUrl     | URL               | The URL to use when checking for a new version number, it should return a version [default: https://api.deskflow.org/version]|

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

|Option             | Valid Values      |Description|
|:------------------|:-----------------:|:-----------|
| autoHide          | `true` or `false` | When true the app will hide itself on start up [default: false] |
| enableUpdateCheck | `true` or `false` | When true check the update URL to see if a new version was released on start up [default: false] |
| closeReminder     | `true` or `false` | Used to track if we have shown the reminder that when you close the app it remain running in the background  [default: true]|
| logExpanded       | `true` or `false` | Should the log section of the GUI be opened [default: false] |
| symbolicTrayIcon  | `true` or `false` | When true use the monocolor (symbolic) icon false uses a colorful icon for the tray |
| windowGeometry    | QRect             | Geometry of the window used to restore the window geometry after exiting the app |

### Log

This section contains options used by the application logging it will begin with `[log]`

|Option |    Valid Values   |Description|
|:------|:-----------------:|:-----------|
| file   | Filepath          | The file to write the log into |
| level  | Valid log level   | Log level to use |
| toFile | `true` or `false` | When true the log will be written to the value of the `file` option |


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
| binary             | Filename          | The name of the binary to call for client mode. This binary exists in same path as the Deskflow GUI |
| configVisible      | `true` or `false` | Used internally to track when the severs has a configuration dialog showing.|
| externalConfig     | `true` or `false` | When true use the external config path |
| externalConfigFile | Filepath          | Path the server config file if it does not exist the GUI will it generated based on the `internalConfig` section.|

### InternalConfig

This section contains options used when in server mode it will begin with `[internalConfig]`
block of a server config file as seen below. This section is used by the GUI to generate a server configuration

```
[internalConfig]
clipboardSharing=true
clipboardSharingSize=@Variant(\0\0\0\x84\0\0\0\0\0\0<\0)
disableLockToScreen=false
hasHeartbeat=false
hasSwitchDelay=false
hasSwitchDoubleTap=false
heartbeat=5000
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
numColumns=5
numRows=3
protocol=1
relativeMouseMoves=false
screens\1\name=
screens\10\aliasArray\size=0
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
screens\7\aliasArray\size=0
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
screens\8\aliasArray\size=0
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
screens\9\aliasArray\size=0
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
switchCornerArray\1\switchCorner=false
switchCornerArray\2\switchCorner=false
switchCornerArray\3\switchCorner=false
switchCornerArray\4\switchCorner=false
switchCornerArray\size=4
switchCornerSize=0
switchDelay=250
switchDoubleTap=250
win32KeepForeground=false
```


# Server Config

The `deskflow-server` command accepts the `-c` or `--config` option, which takes one argument,
the path to a server configuration file. When using the GUI the `internalConfig` section of the GUI settings will be exported as the server configuration.
The configuration file is plain text and case-sensitive. The file is broken into sections, and each section has the form:
```
section: ''name''
	''arg'' = ''value''
end
```

Comments are introduced by ''#'' and continue to the end of the line. ''name'' must be one of the following:

* ''screens''
* ''aliases''
* ''links''
* ''options''

The file is parsed top to bottom and names cannot be used before they've been defined in the <code>screens</code> or <code>aliases</code> sections. So the <code>links</code> and <code>aliases</code> must appear after the <code>screens</code> and <code>links</code> cannot refer to aliases unless the <code>aliases</code> appear before the <code>links</code>.

### The screens section

''args'' is a list of screen names, one name per line, each followed by a colon. Names are arbitrary strings but they must be unique. The hostname of each computer is recommended. (This is the computer's network name on win32 and the name reported by the program hostname on Unix and OS X. Note that OS X may append .local to the name you gave your computer; e.g. somehost.local.) There must be a screen name for the server and each client. Each screen can specify a number of options. Options have the form name = value and are listed one per line after the screen name.

```
section: screens
	moe:
	larry:
		halfDuplexCapsLock = true
		halfDuplexNumLock = true
	curly:
		meta = alt
end
```

This declares three screens named ''moe'', ''larry'', and ''curly''. Screen ''larry'' has half-duplex ''Caps Lock'' and ''Num Lock'' keys (see below) and screen ''curly'' converts the ''Meta'' modifier key to the ''Alt'' modifier key.

#### screen options

A screen can have the following options:
|Option | Valid Values| Description|
|:----------|:-----------:|:-----------|
|halfDuplexCapsLock| `true` or `false` | This computer has a ''Caps Lock'' key that doesn't report a press and a release event when the user presses it but instead reports a press event when it's turned on and a release event when it's turned off. If ''Caps Lock'' acts strangely on all screens then you may need to set this option to true on the server screen. If it acts strangely on one screen then that screen may need the option set to true.|
|halfDuplexNumLock | `true` or `false` | This computer has a ''Num Lock'' key that doesn't report a press and a release event when the user presses it but instead reports a press event when it's turned on and a release event when it's turned off. If ''Num Lock'' acts strangely on all screens then you may need to set this option to true on the server screen. If it acts strangely on one screen then that screen may need the option set to true.|
|halfDuplexScrollLock| `true` or `false`| This computer has a ''Scroll Lock'' key that doesn't report a press and a release event when the user presses it but instead reports a press event when it's turned on and a release event when it's turned off. If ''Scroll Lock'' acts strangely on all screens then you may need to set this option to true on the server screen. If it acts strangely on one screen then that screen may need the option set to true.|
|xtestIsXineramaUnaware| `true` or `false`| This option works around a bug in the XTest extension when used in combination with Xinerama. It affects X11 clients only. Not all versions of the XTest extension are aware of the Xinerama extension. As a result, they do not move the mouse correctly when using multiple Xinerama screens. This option is currently ''true'' by default. If you know your XTest extension is Xinerama aware then set this option to ''false''.|
|preserveFocus| `true` or `false` | When true don't drop focus when switching screens
|switchCorners| corners |See <a href="#switch-corners">switchCorners</a> below.|
|switchCornerSize | integer | see switchCornerSize below.|
|shift | shift ctrl alt meta super none | Map the server's shift modifer to different key on a client screen|
|ctrl  | shift ctrl alt meta super none | Map the server's ctrl modifer to different key on a client screen|
|alt | shift ctrl alt meta super none | Map the server's alt modifer to different key on a client screen|
|meta|  shift ctrl alt meta super none | Map the server's meta modifer to different key on a client screen|
|super|  shift ctrl alt meta super none | Map the server's super modifer to different key on a client screen|

### aliases section

''args'' is a list of screen names just like in the ''screens'' section except each screen is followed by a list of aliases, one per line, not followed by a colon. An ''alias'' is a screen name and must be unique. During screen name lookup each alias is equivalent to the screen name it aliases. So a client can connect using its canonical screen name or any of its aliases.

```
section: aliases
	larry:
		larry.stooges.com
	curly:
		shemp
end
```

Screen ''larry'' is also known as ''larry.stooges.com'' and can connect as either name. Screen ''curly'' is also known as ''shemp'' (hey, it's just an example).

### links secion

''args'' is a list of screen names just like in the ''screens'' section except each screen is followed by a list of links, one per line. Each link has the form:
```
 {left|right|up|down}[<range>] = name[<range>]
```

A link indicates which screen is adjacent in the given direction.

Each side of a link can specify a range which defines a portion of an edge. A range on the direction is the portion of edge you can leave from while a range on the screen is the portion of edge you'll enter into. Ranges are optional and default to the entire edge. All ranges on a particular direction of a particular screen must not overlap.

A ''range'' is written as <code>(start,end)</code>. Both ''start'' and ''end'' are percentages in the range 0 to 100, inclusive. The start must be less than the end. 0 is the left or top of an edge and 100 is the right or bottom.

```
section: links
	moe:
		right        = larry
		up(50,100)   = curly(0,50)
	larry:
		left         = moe
		up(0,50)     = curly(50,100)
	curly:
		down(0,50)   = moe
		down(50,100) = larry(0,50)
end
```

This indicates that screen ''larry'' is to the right of screen ''moe'' (so moving the cursor off the right edge of ''moe'' would make it appear at the left edge of ''larry''), the left half of curly is above the right half of ''moe'', ''moe'' is to the left of ''larry'' (edges are not necessarily symmetric so you have to provide both directions), the right half of curly is above the left half of ''larry'', all of ''moe'' is below the left half of ''curly'', and the left half of ''larry'' is below the right half of ''curly''.

Note that links do not have to be symmetrical; for instance, here the edge between ''moe'' and ''curly'' maps to different ranges depending on if you're going up or down. In fact links don't have to be bidirectional. You can configure the right of ''moe'' to go to ''larry'' without a link from the left of ''larry'' to ''moe''. It's possible to configure a screen with no outgoing links; the cursor will get stuck on that screen unless you have a hot key configured to switch off of that screen.

### options section

''args'' is a list of lines of the form <code>name = value</code>. These set the global options.

```
section: options
	protocol = barrier
	heartbeat = 5000
	switchDelay = 500
end
```

#### List of options allowed in options section

| Options | Value Values| Description|
|:--------|:-----------:|:-----------|
|protocol | barrier or synergy| The protocol to use when saying hello to clients. Can be set to barrier or synergy. If not set barrier is used as the default |
|heartbeat| integer (N) | The server will expect each client to send a message no less than every `N` milliseconds. If no message arrives from a client within `3N` seconds the server forces that client to disconnect. If deskflow fails to detect clients disconnecting while the server is sleeping or vice versa, try using this option. |
|switchCorners | none top-left top-right bottom-left bottom-right left right top bottom all | Deskflow won't switch screens when the mouse reaches the edge of the screen if it's in a listed corner. The size of all corners is given by the `switchCornerSize` option. The first name in the list is one of the above names and defines the initial set of corners. Subsequent names are prefixed with + or - to add the corner to or remove the corner from the set, respectively. For example: `all -left +top-left` starts will all corners, removes the left corners (top and bottom) then adds the top-left back in, resulting in the top-left, bottom-left and bottom-right corners.|
|switchCornerSize | integer (N) | Sets the size of all corners in pixels. The cursor must be within `N` pixels of the corner to be considered to be in the corner.|
|switchDelay | integer| Deskflow won't switch screens when the mouse reaches the edge of a screen unless it stays on the edge for `N` milliseconds. This helps prevent unintentional switching when working near the edge of a screen.|
|switchDoubleTap| integer(N) | Deskflow won't switch screens when the mouse reaches the edge of a screen unless it's moved away from the edge and then back to the edge within `N` milliseconds. With the option you have to quickly tap the edge twice to switch. This helps prevent unintentional switching when working near the edge of a screen.|
|screenSaverSync| `true` or `false`| ''Note: Removed in v1.14.1'' If set to ''false'' then Deskflow won't synchronize screen savers. Client screen savers will start according to their individual configurations. The server screen saver won't start if there is input, even if that input is directed toward a client screen.|
|relativeMouseMoves| `true` or `false`| If set to ''true'' then secondary screens move the mouse using relative rather than absolute mouse moves when and only when the cursor is locked to the screen (by ''Scroll Lock'' or a configured hot key). This is intended to make Deskflow work better with certain games. If set to ''false'' or not set then all mouse moves are absolute.|
|clipboardSharing| `true` or `false`|If set to ''true'' then clipboard sharing will be enabled and the ''clipboardSharingSize'' setting will be used. If set to false, then clipboard sharing will be disabled and the the ''clipboardSharingSize'' setting will be ignored.|
|clipboardSharingSize| integer (N)| Deskflow will send a maximum of `N` kilobytes of clipboard data to another computer when the mouse transitions to that computer.|
|win32KeepForeground | `true` or `false`| If set to ''true'' (the default), Deskflow will grab the foreground focus on a Windows server (thereby putting all other windows in the background) upon switching to a client. If set to ''false'', it will leave the currently foreground window in the foreground. Deskflow grabs the focus to avoid issues with other apps interfering with Deskflow's ability to read the hardware inputs. |
|keystroke(key) | actions | Binds the ''key'' combination key to the given ''actions''. ''key'' is an optional list of modifiers (''shift'', ''control'', ''alt'', ''meta'' or ''super'') optionally followed by a character or a key name, all separated by + (plus signs). You must have either modifiers or a character/key name or both. See below for `valid key names` and `actions`. Keyboard hot keys are handled while the cursor is on the primary screen and secondary screens. Separate actions can be assigned to press and release.|
|mousebutton(button) | actions| Binds the modifier and mouse button combination ''button'' to the given ''actions''. ''button'' is an optional list of modifiers (''shift'', ''control'', ''alt'', ''meta'' or ''super'') followed by a button number. The primary button (the left button for right handed users) is button 1, the middle button is 2, etc. Actions can be found below. Mouse button actions are not handled while the cursor is on the primary screen. You cannot use these to perform an action while on the primary screen. Separate actions can be assigned to press and release.|


You can use both the ''switchDelay'' and ''switchDoubleTap'' options at the same time. Deskflow will switch when either requirement is satisfied.

##### Actions

Actions are two lists of individual actions separated by commas. The two lists are separated by a '';'' (semicolon). Either list can be empty and if the second list is empty then the semicolon is optional. The first list lists actions to take when the condition becomes true (e.g. the hot key or mouse button is pressed) and the second lists actions to take when the condition becomes false (e.g. the hot key or button is released). The condition becoming true is called activation and becoming false is called deactivation. Allowed individual actions are:

* `keystroke(key[,screens])`

* `keyDown(key[,screens])`

* `keyUp(key[,screens])`


: Synthesizes the modifiers and key given in ''key'' which has the same form as described in the ''keystroke'' option. If given, ''screens'' lists the screen or screens to direct the event to, regardless of the active screen. If not given then the event is directed to the active screen only.
: ''keyDown'' synthesizes a key press and ''keyUp'' synthesizes a key release. ''keystroke'' synthesizes a key press on activation and a release on deactivation and is equivalent to a ''keyDown'' on activation and ''keyUp'' on deactivation.
: ''screens'' is either ''*'' (asterisk) to indicate all screens or a '':'' (colon) separated list of screen names. (Note that the screen name must have already been encountered in the configuration file so you'll probably want to put ''actions'' at the bottom of the file.)

* `mousebutton(button)`
* `mouseDown(button)`
* `mouseUp(button)`
: Synthesizes the modifiers and mouse button given in ''button'' which has the same form as described in the ''mousebutton'' option.
: ''mouseDown'' synthesizes a mouse press and ''mouseUp'' synthesizes a mouse release. ''mousebutton'' synthesizes a mouse press on activation and a release on deactivation and is equivalent to a ''mouseDown'' on activation and ''mouseUp'' on deactivation.

* `lockCursorToScreen(mode)`
: Locks the cursor to or unlocks the cursor from the active screen. ''mode'' can be ''off'' to unlock the cursor, ''on'' to lock the cursor, or ''toggle'' to toggle the current state. The default is ''toggle''. If the configuration has no ''lockCursorToScreen'' action and ''Scroll Lock'' is not used as a hot key then ''Scroll Lock'' toggles cursor locking.

* `switchToScreen(screen)`
: Jump to screen with name or alias ''screen''.

* `switchInDirection(dir)`
: Switch to the screen in the direction ''dir'', which may be one of ''left'', ''right'', ''up'' or ''down''.

* `switchToNextScreen()`
: Cycle to the next screen in the configuration order. If at the last screen, cycles back to the first screen.

##### Keynames
Valid key names are:

<details><summary>Valid Key Names</summary>
* AppMail
* AppMedia
* AppUser1
* AppUser2
* AudioDown
* AudioMute
* AudioNext
* AudioPlay
* AudioPrev
* AudioStop
* AudioUp
* BackSpace
* Begin
* Break
* Cancel
* CapsLock
* Clear
* Delete
* Down
* Eject
* End
* Escape
* Execute
* F1
* F2
* F3
* F4
* F5
* F6
* F7
* F8
* F9
* F10
* F11
* F12
* F13
* F14
* F15
* F16
* F17
* F18
* F19
* F20
* F21
* F22
* F23
* F24
* F25
* F26
* F27
* F28
* F29
* F30
* F31
* F32
* F33
* F34
* F35
* Find
* Help
* Home
* Insert
* KP_0
* KP_1
* KP_2
* KP_3
* KP_4
* KP_5
* KP_6
* KP_7
* KP_8
* KP_9
* KP_Add
* KP_Begin
* KP_Decimal
* KP_Delete
* KP_Divide
* KP_Down
* KP_End
* KP_Enter
* KP_Equal
* KP_F1
* KP_F2
* KP_F3
* KP_F4
* KP_Home
* KP_Insert
* KP_Left
* KP_Multiply
* KP_PageDown
* KP_PageUp
* KP_Right
* KP_Separator
* KP_Space
* KP_Subtract
* KP_Tab
* KP_Up
* Left
* LeftTab
* Linefeed
* Menu
* NumLock
* PageDown
* PageUp
* Pause
* Print
* Redo
* Return
* Right
* ScrollLock
* Select
* Sleep
* Space
* SysReq
* Tab
* Undo
* Up
* WWWBack
* WWWFavorites
* WWWForward
* WWWHome
* WWWRefresh
* WWWSearch
* WWWStop
* Space
* Exclaim
* DoubleQuote
* Number
* Dollar
* Percent
* Ampersand
* Apostrophe
* ParenthesisL
* ParenthesisR
* Asterisk
* Plus
* Comma
* Minus
* Period
* Slash
* Colon
* Semicolon
* Less
* Equal
* Greater
* Question
* At
* BracketL
* Backslash
* BracketR
* Circumflex
* Underscore
* Grave
* BraceL
* Bar
* BraceR
* Tilde
</details>

Additionally, a name of the form `\uXXXX` where ''XXXX'' is a hexadecimal number is interpreted as a unicode character code. Key and modifier names are case-insensitive. Keys that don't exist on the keyboard or in the default keyboard layout will not work.

### Example textual configuration file

This example comes from doc/deskflow-basic.conf

```
# sample deskflow configuration file
#
# comments begin with the # character and continue to the end of
# line.  comments may appear anywhere the syntax permits.
# +-------+  +--------+ +---------+
# |Laptop |  |Desktop1| |iMac     |
# |       |  |        | |         |
# +-------+  +--------+ +---------+

section: screens
	# three hosts named:  Laptop, Desktop1, and iMac
	# These are the nice names of the hosts to make it easy to write the config file
	# The aliases section below contain the "actual" names of the hosts (their hostnames)
	Laptop:
	Desktop1:
	iMac:
end

section: links
	# iMac is to the right of Desktop1
	# Laptop is to the left of Desktop1
	Desktop1:
		right(0,100) = iMac # the numbers in parentheses indicate the percentage of the screen's edge to be considered active for switching)
		left  = Laptop
		shift = shift (shift, alt, super, meta can be mapped to any of the others)
	# Desktop1 is to the right of Laptop
	Laptop:
		right = Desktop1
	# Desktop1 is to the left of iMac
	iMac:
		left  = Desktop1
end
section: aliases
	# The "real" name of iMac is John-Smiths-iMac-3.local. 
	# If we wanted we could remove this alias and instead use John-Smiths-iMac-3.local everywhere iMac is above. 
	# Hopefully it should be easy to see why using an alias is nicer
	iMac:
		John-Smiths-iMac-3.local
end
```

#### Cursor Wrapping

The text config allows screens to be wrapped around. For example, with two machines (a server and a client), the mouse can go off the right of the server onto the left side of the client, then off the right side of the client back onto the left side of server. This config also uses ''Ctrl''+''Super''+(''left arrow''/''right arrow'') to switch between machines on keypress.

```
# Physical monitor arrangement, with machine names as used by Deskflow.
#  +----------+----------+
#  | syn-serv | syn-cli  |
#  |          |          |
#  +----------+----------+ 
 
section: screens
	syn-serv:
	syn-cli:
end
section: links
	syn-serv:
		left = syn-cli     # "wrapping" arrangement
		right = syn-cli    # "normal" arrangement
	syn-cli:
		left = syn-serv    # "normal"
		right = syn-serv   # "wrapping"
end
section: options
		keystroke(control+super+right) = switchInDirection(right) # Switch screens on keypress 
<!--		keystroke(control+super+left) = switchInDirection(left) -->
end
```

### AltGr key

The following screen config allows the mapping for ''Alt'' to ''AltGr''. Although this may not work, see [https://github.com/deskflow/deskflow-core/issues/4411 bug #4411].
```
section: screens
	client1:
		altgr = alt          # mapping to fix AltGr key not working on windows clients (e.g. @-Symbol etc.).
end
```

See also: the man page for ''deskflow-core''.

### Stacked Example

Stack one computer's screen on top of another's.

```
#           +-------+
#           | curly |
#           |       |
#           +-------+
# +-------+ +-------+
# | moe   | | larry |
# |       | |       |
# +-------+ +-------+

section: screens
	# three hosts named: moe, larry, and curly
	moe:
	larry:
	curly:
end

section: links
	# larry is to the right of moe and curly is above moe.
	moe:
		right = larry
		up    = curly

	# moe is to the left of larry and curly is above larry.
	larry:
		left  = moe
		up    = curly

	# larry is below curly.
	curly:
		down  = larry
end

section: aliases
	# curly is also known as shemp
	curly:
		shemp
end
```

### Horizontal Example

Align all screens horizontally.

```
# +-------+ +-------+ +-------+
# | moe   | | larry | | curly |
# |       | |       | |       |
# +-------+ +-------+ +-------+

section: screens
	# three hosts named: moe, larry, and curly
	moe:
	larry:
	curly:
end

section: links
	# curly is to the right of larry and moe is to the left of larry.
	larry:
		right = curly
		left  = moe

	# larry is to the right of moe.
	moe:
		right = larry

	# larry is to the left of curly.
	curly:
		left  = larry
end

```

### Span Example

Span two screens on one computer across the screens of two computers.

```
# +-------+ +-------+
# | curly | | curly |
# |       | |       |
# +-------+ +-------+
# +-------+ +-------+
# | moe   | | larry |
# |       | |       |
# +-------+ +-------+

section: screens
	# three hosts named: moe, larry, and curly
	moe:
	larry:
	curly:
end

section: links
	# larry is to the right of moe and curly is above moe.
	moe:
		right = larry
		up    = curly

	# moe is to the left of larry and curly is above larry.
	larry:
		left  = moe
		up    = curly

	# larry is below curly.
	curly:
		down  = larry
end
```
