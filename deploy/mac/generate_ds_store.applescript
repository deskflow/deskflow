-- SPDX-FileCopyrightText: 2020 Chris Rizzitello <sithlord48@gmail.com>
-- SPDX-License-Identifier: MIT

on run argv
  set image_name to item 1 of argv

  tell application "Finder"
  tell disk image_name

    -- wait for the image to finish mounting
    set open_attempts to 0
    repeat while open_attempts < 5
      try
        open
          delay 5
          set open_attempts to 5
        close
      on error errStr number errorNumber
        set open_attempts to open_attempts + 1
        delay 10
      end try
    end repeat

    -- open the image the first time and save a DS_Store with just
    -- background and icon setup
    open
      set current view of container window to icon view
      set theViewOptions to the icon view options of container window
      set background picture of theViewOptions to file ".background:background.tiff"
      set arrangement of theViewOptions to not arranged
      set icon size of theViewOptions to 100
      set text size of theViewOptions to 16
    close

    open
      tell container window
        set sidebar width to 0
        set statusbar visible to false
        set toolbar visible to false
        set pathbar visible to false
        set the bounds to { 200, 120, 800, 520 }
        set position of item "Deskflow.app" to { 144, 190 }
        set position of item "Applications" to { 455, 190 }
      end tell
    close
  end tell
end tell
end run
