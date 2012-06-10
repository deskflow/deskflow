/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <vncconfig/Legacy.h>

#include <rfb/LogWriter.h>
#include <rfb/Password.h>
#include <rfb_win32/CurrentUser.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("Legacy");


void LegacyPage::LoadPrefs()
      {
        // VNC 3.3.3R3 Preferences Algorithm, as described by vncProperties.cpp
        // - Load user-specific settings, based on logged-on user name,
        //   from HKLM/Software/ORL/WinVNC3/<user>.  If they don't exist,
        //   try again with username "Default".
        // - Load system-wide settings from HKLM/Software/ORL/WinVNC3.
        // - If AllowProperties is non-zero then load the user's own
        //   settings from HKCU/Software/ORL/WinVNC3.

        // Get the name of the current user
        TCharArray username;
        try {
          UserName name;
          username.buf = name.takeBuf();
        } catch (rdr::SystemException& e) {
          if (e.err != ERROR_NOT_LOGGED_ON)
            throw;
        }

        // Open and read the WinVNC3 registry key
        allowProperties = true;
        RegKey winvnc3;
        try {
          winvnc3.openKey(HKEY_LOCAL_MACHINE, _T("Software\\ORL\\WinVNC3"));
          int debugMode = winvnc3.getInt(_T("DebugMode"), 0);
          const char* debugTarget = 0; 
          if (debugMode & 2) debugTarget = "file";
          if (debugMode & 4) debugTarget = "stderr";
          if (debugTarget) {
            char logSetting[32];
            sprintf(logSetting, "*:%s:%d", debugTarget, winvnc3.getInt(_T("DebugLevel"), 0));
            regKey.setString(_T("Log"), TStr(logSetting));
          }
 
          TCharArray authHosts;
          authHosts.buf = winvnc3.getString(_T("AuthHosts"), 0);
          if (authHosts.buf) {
            CharArray newHosts;
            newHosts.buf = strDup("");

            // Reformat AuthHosts to Hosts.  Wish I'd left the format the same. :( :( :(
            try {
              CharArray tmp = strDup(authHosts.buf);
              while (tmp.buf) {

                // Split the AuthHosts string into patterns to match
                CharArray first;
                rfb::strSplit(tmp.buf, ':', &first.buf, &tmp.buf);
                if (strlen(first.buf)) {
                  int bits = 0;
                  CharArray pattern(1+4*4+4);
                  pattern.buf[0] = first.buf[0];
                  pattern.buf[1] = 0;

                  // Split the pattern into IP address parts and process
                  rfb::CharArray address;
                  address.buf = rfb::strDup(&first.buf[1]);
                  while (address.buf) {
                    rfb::CharArray part;
                    rfb::strSplit(address.buf, '.', &part.buf, &address.buf);
                    if (bits)
                      strcat(pattern.buf, ".");
                    if (strlen(part.buf) > 3)
                      throw rdr::Exception("Invalid IP address part");
                    if (strlen(part.buf) > 0) {
                      strcat(pattern.buf, part.buf);
                      bits += 8;
                    }
                  }

                  // Pad out the address specification if required
                  int addrBits = bits;
                  while (addrBits < 32) {
                    if (addrBits) strcat(pattern.buf, ".");
                    strcat(pattern.buf, "0");
                    addrBits += 8;
                  }

                  // Append the number of bits to match
                  char buf[4];
                  sprintf(buf, "/%d", bits);
                  strcat(pattern.buf, buf);

                  // Append this pattern to the Hosts value
                  int length = strlen(newHosts.buf) + strlen(pattern.buf) + 2;
                  CharArray tmpHosts(length);
                  strcpy(tmpHosts.buf, pattern.buf);
                  if (strlen(newHosts.buf)) {
                    strcat(tmpHosts.buf, ",");
                    strcat(tmpHosts.buf, newHosts.buf);
                  }
                  delete [] newHosts.buf;
                  newHosts.buf = tmpHosts.takeBuf();
                }
              }

              // Finally, save the Hosts value
              regKey.setString(_T("Hosts"), TStr(newHosts.buf));
            } catch (rdr::Exception) {
              MsgBox(0, _T("Unable to convert AuthHosts setting to Hosts format."),
                     MB_ICONWARNING | MB_OK);
            }
          } else {
            regKey.setString(_T("Hosts"), _T("+"));
          }

          regKey.setBool(_T("LocalHost"), winvnc3.getBool(_T("LoopbackOnly"), false));
          // *** check AllowLoopback?

          if (winvnc3.getBool(_T("AuthRequired"), true))
            regKey.setString(_T("SecurityTypes"), _T("VncAuth"));
          else
            regKey.setString(_T("SecurityTypes"), _T("None"));

          int connectPriority = winvnc3.getInt(_T("ConnectPriority"), 0);
          regKey.setBool(_T("DisconnectClients"), connectPriority == 0);
          regKey.setBool(_T("AlwaysShared"), connectPriority == 1);
          regKey.setBool(_T("NeverShared"), connectPriority == 2);

        } catch(rdr::Exception) {
        }

        // Open the local, default-user settings
        allowProperties = true;
        try {
          RegKey userKey;
          userKey.openKey(winvnc3, _T("Default"));
          vlog.info("loading Default prefs");
          LoadUserPrefs(userKey);
        } catch(rdr::Exception& e) {
          vlog.error("error reading Default settings:%s", e.str());
        }

        // Open the local, user-specific settings
        if (userSettings && username.buf) {
          try {
            RegKey userKey;
            userKey.openKey(winvnc3, username.buf);
            vlog.info("loading local User prefs");
            LoadUserPrefs(userKey);
          } catch(rdr::Exception& e) {
            vlog.error("error reading local User settings:%s", e.str());
          }

          // Open the user's own settings
          if (allowProperties) {
            try {
              RegKey userKey;
              userKey.openKey(HKEY_CURRENT_USER, _T("Software\\ORL\\WinVNC3"));
              vlog.info("loading global User prefs");
              LoadUserPrefs(userKey);
            } catch(rdr::Exception& e) {
              vlog.error("error reading global User settings:%s", e.str());
            }
          }
        }

        // Disable the Options menu item if appropriate
        regKey.setBool(_T("DisableOptions"), !allowProperties);
      }

      void LegacyPage::LoadUserPrefs(const RegKey& key)
      {
        if (key.getBool(_T("HTTPConnect"), true))
          regKey.setInt(_T("HTTPPortNumber"), key.getInt(_T("PortNumber"), 5900)-100);
        else
          regKey.setInt(_T("HTTPPortNumber"), 0);
        regKey.setInt(_T("PortNumber"), key.getBool(_T("SocketConnect")) ? key.getInt(_T("PortNumber"), 5900) : 0);
        if (key.getBool(_T("AutoPortSelect"), false)) {
          MsgBox(0, _T("The AutoPortSelect setting is not supported by this release.")
                    _T("The port number will default to 5900."),
                    MB_ICONWARNING | MB_OK);
          regKey.setInt(_T("PortNumber"), 5900);
        }
        regKey.setInt(_T("IdleTimeout"), key.getInt(_T("IdleTimeout"), 0));

        regKey.setBool(_T("RemoveWallpaper"), key.getBool(_T("RemoveWallpaper")));
        regKey.setBool(_T("RemovePattern"), key.getBool(_T("RemoveWallpaper")));
        regKey.setBool(_T("DisableEffects"), key.getBool(_T("RemoveWallpaper")));

        if (key.getInt(_T("QuerySetting"), 2) != 2) {
          regKey.setBool(_T("QueryConnect"), key.getInt(_T("QuerySetting")) > 2);
          MsgBox(0, _T("The QuerySetting option has been replaced by QueryConnect.")
                 _T("Please see the documentation for details of the QueryConnect option."),
                 MB_ICONWARNING | MB_OK);
        }
        regKey.setInt(_T("QueryTimeout"), key.getInt(_T("QueryTimeout"), 10));

        ObfuscatedPasswd passwd;
        key.getBinary(_T("Password"), (void**)&passwd.buf, &passwd.length, 0, 0);
        regKey.setBinary(_T("Password"), passwd.buf, passwd.length);

        bool enableInputs = key.getBool(_T("InputsEnabled"), true);
        regKey.setBool(_T("AcceptKeyEvents"), enableInputs);
        regKey.setBool(_T("AcceptPointerEvents"), enableInputs);
        regKey.setBool(_T("AcceptCutText"), enableInputs);
        regKey.setBool(_T("SendCutText"), enableInputs);

        switch (key.getInt(_T("LockSetting"), 0)) {
        case 0: regKey.setString(_T("DisconnectAction"), _T("None")); break;
        case 1: regKey.setString(_T("DisconnectAction"), _T("Lock")); break;
        case 2: regKey.setString(_T("DisconnectAction"), _T("Logoff")); break;
        };

        regKey.setBool(_T("DisableLocalInputs"), key.getBool(_T("LocalInputsDisabled"), false));

        // *** ignore polling preferences
        // PollUnderCursor, PollForeground, OnlyPollConsole, OnlyPollOnEvent
        regKey.setBool(_T("UseHooks"), !key.getBool(_T("PollFullScreen"), false));

        if (key.isValue(_T("AllowShutdown")))
          MsgBox(0, _T("The AllowShutdown option is not supported by this release."), MB_ICONWARNING | MB_OK);
        if (key.isValue(_T("AllowEditClients")))
          MsgBox(0, _T("The AllowEditClients option is not supported by this release."), MB_ICONWARNING | MB_OK);

        allowProperties = key.getBool(_T("AllowProperties"), allowProperties);
      }
