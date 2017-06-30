/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define TRAY_RETRY_COUNT 10
#define TRAY_RETRY_WAIT 2000

#include "QSynergyApplication.h"
#include "LicenseManager.h"
#include "MainWindow.h"
#include "AppConfig.h"
#include "SetupWizard.h"

#include <QtCore>
#include <QtGui>
#include <QSettings>
#include <QMessageBox>

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif

#ifdef Q_OS_DARWIN
#include <cstdlib>
#endif

class QThreadImpl : public QThread {
public:
    static void
    msleep (unsigned long msecs) {
        QThread::msleep (msecs);
    }
};

int waitForTray ();

#if defined(Q_OS_MAC)
bool checkMacAssistiveDevices ();
#endif

int
main (int argc, char* argv[]) {
#ifdef Q_OS_DARWIN
    /* Workaround for QTBUG-40332 - "High ping when QNetworkAccessManager is
     * instantiated" */
    ::setenv ("QT_BEARER_POLL_TIMEOUT", "-1", 1);
#endif
    QCoreApplication::setOrganizationName ("Synergy");
    QCoreApplication::setOrganizationDomain ("http://symless.com/");
    QCoreApplication::setApplicationName ("Synergy");

    QSynergyApplication app (argc, argv);

#if defined(Q_OS_MAC)

    if (app.applicationDirPath ().startsWith ("/Volumes/")) {
        QMessageBox::information (NULL, "Synergy", "Please drag Synergy to the "
                                                   "Applications folder, and "
                                                   "open it from there.");
        return 1;
    }

    if (!checkMacAssistiveDevices ()) {
        return 1;
    }
#endif

    if (!waitForTray ()) {
        return -1;
    }

#ifndef Q_OS_WIN
    QApplication::setQuitOnLastWindowClosed (false);
#endif

    QSettings settings;
    AppConfig appConfig (&settings);
    qRegisterMetaType<Edition> ("Edition");
    LicenseManager licenseManager (&appConfig);

    app.switchTranslator (appConfig.language ());

    MainWindow mainWindow (settings, appConfig, licenseManager);
    SetupWizard setupWizard (mainWindow, true);

    if (appConfig.wizardShouldRun ()) {
        setupWizard.show ();
    } else {
        mainWindow.open ();
    }

    return app.exec ();
}

int
waitForTray () {
    // on linux, the system tray may not be available immediately after logging
    // in,
    // so keep retrying but give up after a short time.
    int trayAttempts = 0;
    while (true) {
        if (QSystemTrayIcon::isSystemTrayAvailable ()) {
            break;
        }

        if (++trayAttempts > TRAY_RETRY_COUNT) {
            QMessageBox::critical (
                NULL,
                "Synergy",
                QObject::tr (
                    "System tray is unavailable, don't close your window."));
            return true;
        }

        QThreadImpl::msleep (TRAY_RETRY_WAIT);
    }
    return true;
}

#if defined(Q_OS_MAC)
bool
checkMacAssistiveDevices () {
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090 // mavericks

    // new in mavericks, applications are trusted individually
    // with use of the accessibility api. this call will show a
    // prompt which can show the security/privacy/accessibility
    // tab, with a list of allowed applications. synergy should
    // show up there automatically, but will be unchecked.

    if (AXIsProcessTrusted ()) {
        return true;
    }

    const void* keys[]      = {kAXTrustedCheckOptionPrompt};
    const void* trueValue[] = {kCFBooleanTrue};
    CFDictionaryRef options =
        CFDictionaryCreate (NULL, keys, trueValue, 1, NULL, NULL);

    bool result = AXIsProcessTrustedWithOptions (options);
    CFRelease (options);
    return result;

#else

    // now deprecated in mavericks.
    bool result = AXAPIEnabled ();
    if (!result) {
        QMessageBox::information (
            NULL,
            "Synergy",
            "Please enable access to assistive devices "
            "System Preferences -> Security & Privacy -> "
            "Privacy -> Accessibility, then re-open Synergy.");
    }
    return result;

#endif
}
#endif
