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
#ifndef WINVNCCONF_CONNECTIONS
#define WINVNCCONF_CONNECTIONS

#include <vector>

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/ModuleFileName.h>
#include <rfb/Configuration.h>
#include <rfb/Blacklist.h>
#include <network/TcpSocket.h>

static rfb::IntParameter http_port("HTTPPortNumber",
  "TCP/IP port on which the server will serve the Java applet VNC Viewer ", 5800);
static rfb::IntParameter port_number("PortNumber",
  "TCP/IP port on which the server will accept connections", 5900);
static rfb::StringParameter hosts("Hosts",
  "Filter describing which hosts are allowed access to this server", "+");
static rfb::BoolParameter localHost("LocalHost",
  "Only accept connections from via the local loop-back network interface", false);

namespace rfb {

  namespace win32 {

    class ConnHostDialog : public Dialog {
    public:
      ConnHostDialog() : Dialog(GetModuleHandle(0)) {}
      bool showDialog(const TCHAR* pat) {
        pattern.replaceBuf(tstrDup(pat));
        return Dialog::showDialog(MAKEINTRESOURCE(IDD_CONN_HOST));
      }
      void initDialog() {
        if (_tcslen(pattern.buf) == 0)
          pattern.replaceBuf(tstrDup("+"));

        if (pattern.buf[0] == _T('+'))
          setItemChecked(IDC_ALLOW, true);
        else if (pattern.buf[0] == _T('?'))
          setItemChecked(IDC_QUERY, true);
        else
          setItemChecked(IDC_DENY, true);

        setItemString(IDC_HOST_PATTERN, &pattern.buf[1]);
        pattern.replaceBuf(0);
      }
      bool onOk() {
        TCharArray host = getItemString(IDC_HOST_PATTERN);
        TCharArray newPat(_tcslen(host.buf)+2);
        if (isItemChecked(IDC_ALLOW))
          newPat.buf[0] = _T('+');
        else if (isItemChecked(IDC_QUERY))
          newPat.buf[0] = _T('?');
        else
          newPat.buf[0] = _T('-');
        newPat.buf[1] = 0;
        _tcscat(newPat.buf, host.buf);

        network::TcpFilter::Pattern pat(network::TcpFilter::parsePattern(CStr(newPat.buf)));
        pattern.replaceBuf(TCharArray(network::TcpFilter::patternToStr(pat)).takeBuf());
        return true;
      }
      const TCHAR* getPattern() {return pattern.buf;}
    protected:
      TCharArray pattern;
    };

    class ConnectionsPage : public PropSheetPage {
    public:
      ConnectionsPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_CONNECTIONS)), regKey(rk) {}
      void initDialog() {
        vlog.debug("set IDC_PORT %d", (int)port_number);
        setItemInt(IDC_PORT, port_number ? port_number : 5900);
        setItemChecked(IDC_RFB_ENABLE, port_number != 0);
        setItemInt(IDC_IDLE_TIMEOUT, rfb::Server::idleTimeout);
        vlog.debug("set IDC_HTTP_PORT %d", (int)http_port);
        setItemInt(IDC_HTTP_PORT, http_port ? http_port : 5800);
        setItemChecked(IDC_HTTP_ENABLE, http_port != 0);
        enableItem(IDC_HTTP_PORT, http_port != 0);
        setItemChecked(IDC_LOCALHOST, localHost);

        HWND listBox = GetDlgItem(handle, IDC_HOSTS);
        while (SendMessage(listBox, LB_GETCOUNT, 0, 0))
          SendMessage(listBox, LB_DELETESTRING, 0, 0);

        CharArray tmp;
        tmp.buf = hosts.getData();
        while (tmp.buf) {
          CharArray first;
          strSplit(tmp.buf, ',', &first.buf, &tmp.buf);
          if (strlen(first.buf))
            SendMessage(listBox, LB_ADDSTRING, 0, (LPARAM)(const TCHAR*)TStr(first.buf));
        }

        onCommand(IDC_RFB_ENABLE, EN_CHANGE);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_HOSTS:
          {
            DWORD selected = SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_GETCURSEL, 0, 0);
            int count = SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_GETCOUNT, 0, 0);
            bool enable = selected != LB_ERR;
            enableItem(IDC_HOST_REMOVE, enable);
            enableItem(IDC_HOST_UP, enable && (selected > 0));
            enableItem(IDC_HOST_DOWN, enable && (selected < count-1));
            enableItem(IDC_HOST_EDIT, enable);
            setChanged(isChanged());
          }
          return true;

        case IDC_PORT:
          if (cmd == EN_CHANGE) {
            try {
              setItemInt(IDC_HTTP_PORT, rfbPortToHTTP(getItemInt(IDC_PORT)));
            } catch (...) {
            }
          }
        case IDC_HTTP_PORT:
        case IDC_IDLE_TIMEOUT:
          if (cmd == EN_CHANGE)
            setChanged(isChanged());
          return false;

        case IDC_HTTP_ENABLE:
        case IDC_RFB_ENABLE:
        case IDC_LOCALHOST:
          {
            // HTTP port
            enableItem(IDC_HTTP_PORT, isItemChecked(IDC_HTTP_ENABLE) && isItemChecked(IDC_RFB_ENABLE));
            enableItem(IDC_HTTP_ENABLE, isItemChecked(IDC_RFB_ENABLE));

            // RFB port
            enableItem(IDC_PORT, isItemChecked(IDC_RFB_ENABLE));

            // Hosts
            enableItem(IDC_LOCALHOST, isItemChecked(IDC_RFB_ENABLE));

            bool enableHosts = !isItemChecked(IDC_LOCALHOST) && isItemChecked(IDC_RFB_ENABLE);
            enableItem(IDC_HOSTS, enableHosts);
            enableItem(IDC_HOST_ADD, enableHosts);
            if (!enableHosts) {
              enableItem(IDC_HOST_REMOVE, enableHosts);
              enableItem(IDC_HOST_UP, enableHosts);
              enableItem(IDC_HOST_DOWN, enableHosts);
              enableItem(IDC_HOST_EDIT, enableHosts);
            } else {
              onCommand(IDC_HOSTS, EN_CHANGE);
            }
            setChanged(isChanged());
            return false;
          }

        case IDC_HOST_ADD:
          if (hostDialog.showDialog(_T("")))
          {
            const TCHAR* pattern = hostDialog.getPattern();
            if (pattern)
              SendMessage(GetDlgItem(handle, IDC_HOSTS), LB_ADDSTRING, 0, (LPARAM)pattern);
          }
          return true;

        case IDC_HOST_EDIT:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            TCharArray pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.buf);

            if (hostDialog.showDialog(pattern.buf)) {
              const TCHAR* newPat = hostDialog.getPattern();
              if (newPat) {
                item = SendMessage(listBox, LB_FINDSTRINGEXACT, item, (LPARAM)pattern.buf);
                if (item != LB_ERR) {
                  SendMessage(listBox, LB_DELETESTRING, item, 0); 
                  SendMessage(listBox, LB_INSERTSTRING, item, (LPARAM)newPat);
                  SendMessage(listBox, LB_SETCURSEL, item, 0);
                  onCommand(IDC_HOSTS, EN_CHANGE);
                }
              }
            }
          }
          return true;

        case IDC_HOST_UP:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            TCharArray pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.buf);
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            SendMessage(listBox, LB_INSERTSTRING, item-1, (LPARAM)pattern.buf);
            SendMessage(listBox, LB_SETCURSEL, item-1, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }
          return true;

        case IDC_HOST_DOWN:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            TCharArray pattern(SendMessage(listBox, LB_GETTEXTLEN, item, 0)+1);
            SendMessage(listBox, LB_GETTEXT, item, (LPARAM)pattern.buf);
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            SendMessage(listBox, LB_INSERTSTRING, item+1, (LPARAM)pattern.buf);
            SendMessage(listBox, LB_SETCURSEL, item+1, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }
          return true;

        case IDC_HOST_REMOVE:
          {
            HWND listBox = GetDlgItem(handle, IDC_HOSTS);
            int item = SendMessage(listBox, LB_GETCURSEL, 0, 0);
            SendMessage(listBox, LB_DELETESTRING, item, 0);
            onCommand(IDC_HOSTS, EN_CHANGE);
          }

        }
        return false;
      }
      bool onOk() {
        regKey.setInt(_T("PortNumber"), isItemChecked(IDC_RFB_ENABLE) ? getItemInt(IDC_PORT) : 0);
        regKey.setInt(_T("IdleTimeout"), getItemInt(IDC_IDLE_TIMEOUT));
        regKey.setInt(_T("HTTPPortNumber"), isItemChecked(IDC_HTTP_ENABLE) && isItemChecked(IDC_RFB_ENABLE)
                                            ? getItemInt(IDC_HTTP_PORT) : 0);
        regKey.setInt(_T("LocalHost"), isItemChecked(IDC_LOCALHOST));
        regKey.setString(_T("Hosts"), TCharArray(getHosts()).buf);
        return true;
      }
      bool isChanged() {
        try {
          CharArray new_hosts = getHosts();
          CharArray old_hosts = hosts.getData();
          return (strcmp(new_hosts.buf, old_hosts.buf) != 0) ||
              (localHost != isItemChecked(IDC_LOCALHOST)) ||
              (port_number != getItemInt(IDC_PORT)) ||
              (http_port != getItemInt(IDC_HTTP_PORT)) ||
              ((http_port!=0) != (isItemChecked(IDC_HTTP_ENABLE)!=0)) ||
              (rfb::Server::idleTimeout != getItemInt(IDC_IDLE_TIMEOUT));
        } catch (rdr::Exception) {
          return false;
        }
      }
      char* getHosts() {
        int bufLen = 1, i;
        HWND listBox = GetDlgItem(handle, IDC_HOSTS);
        for (i=0; i<SendMessage(listBox, LB_GETCOUNT, 0, 0); i++)
          bufLen+=SendMessage(listBox, LB_GETTEXTLEN, i, 0)+1;
        TCharArray hosts_str(bufLen);
        hosts_str.buf[0] = 0;
        TCHAR* outPos = hosts_str.buf;
        for (i=0; i<SendMessage(listBox, LB_GETCOUNT, 0, 0); i++) {
          outPos += SendMessage(listBox, LB_GETTEXT, i, (LPARAM)outPos);
          outPos[0] = ',';
          outPos[1] = 0;
          outPos++;
        }
        return strDup(hosts_str.buf);
      }
      int rfbPortToHTTP(int rfbPort) {
        int offset = -100;
        if (http_port)
          offset = http_port - port_number;
        int httpPort = rfbPort + offset;
        if (httpPort <= 0)
          httpPort = rfbPort;
        return httpPort;
      }

    protected:
      RegKey regKey;
      ConnHostDialog hostDialog;
    };

  };

};

#endif