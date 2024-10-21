# Server Config Examples

The `deskflow-server` command accepts the `-c` or `--config` option, which takes one argument,
the path to a server configuration file. The format is non-standard but similar to YAML.

Comments begin with the `#` character and continue to the end of line.
Comments may appear anywhere the syntax permits.

Each `section` element must have a matching `end` element.

## Stacked Example

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

## Horizontal Example

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

## Span Example

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

# Example file for `--config-toml` arg.

```
[server.args]
no-daemon = true
no-tray = true
debug = "DEBUG"
name = "moe"
address = ":24800"

[client.args]
no-daemon = true
no-tray = true
debug = "DEBUG2"
name = "larry"
_last = "moe:24800"
```


# Example Environment

Example contents for the .env file


```

#
# App
#

# Shows the test menu in the GUI (on by default in debug mode)
# DESKFLOW_TEST_MENU=true

# Version checker URL to use (useful for testing)
# DESKFLOW_VERSION_URL="https://api.deskflow.org/version?fake=1.100.0"

# Enable debug logging in the GUI (on by default in debug mode)
# DESKFLOW_GUI_DEBUG=true

# Enable verbose logging in the GUI (always off by default)
# DESKFLOW_GUI_VERBOSE=true

# Reset all settings and delete all data on startup
# DESKFLOW_RESET_ALL=true

#
# Build
#

# [Windows] The version of Qt to build against (overrides the value in the config.yml file)
# QT_VERSION=1.2.3

#
# Packaging (optional)
#

# [Linux] Build extra packages (self-extracting tar.gz and tar.gz)
# LINUX_EXTRA_PACKAGES=true

# [Linux] Run the package command as a different user (requires sudo)
# LINUX_PACKAGE_USER=build

# [Windows] Base64 encoded PFX code signing certificate
# WINDOWS_PFX_CERTIFICATE="very-long-base64-encoded-string"

# [Windows] Password for the PFX code signing certificate
# WINDOWS_PFX_PASSWORD="super-secret-password"

# [macOS] Certificate ID for the Developer ID Application code signing certificate
# APPLE_CODESIGN_ID="Developer ID Application: Acme Inc (ABC123XYZ9)"

# [macOS] Base64 encoded P12 code signing certificate
# APPLE_P12_CERTIFICATE="very-long-base64-encoded-string"

# [macOS] Password for the P12 code signing certificate
# APPLE_P12_PASSWORD="super-secret-password"

# [macOS] Apple Team ID 
# https://developer.apple.com/account/#/membership
# APPLE_TEAM_ID="ABC123XYZ9"

# [macOS] Apple ID used to notarize the app
# APPLE_NOTARY_USER="example@example.com"

# [macOS] App-specific password for the Apple ID
# https://support.apple.com/en-gb/102654
# APPLE_NOTARY_PASSWORD="super-secret-password"

```
