/*
 * barrier -- mouse and keyboard sharing utility
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

#include "ServerConfig.h"
#include "Hotkey.h"
#include "MainWindow.h"
#include "AddClientDialog.h"

#include <QtCore>
#include <QMessageBox>
#include <QAbstractButton>
#include <QPushButton>

static const struct
{
     int x;
     int y;
     const char* name;
} neighbourDirs[] =
{
    {  1,  0, "right" },
    { -1,  0, "left" },
    {  0, -1, "up" },
    {  0,  1, "down" },

};

const int serverDefaultIndex = 7;

ServerConfig::ServerConfig(QSettings* settings, int numColumns, int numRows ,
                QString serverName, MainWindow* mainWindow) :
    m_pSettings(settings),
    m_Screens(),
    m_NumColumns(numColumns),
    m_NumRows(numRows),
    m_ServerName(serverName),
    m_IgnoreAutoConfigClient(false),
    m_EnableDragAndDrop(false),
    m_ClipboardSharing(true),
    m_pMainWindow(mainWindow)
{
    Q_ASSERT(m_pSettings);

    loadSettings();
}

ServerConfig::~ServerConfig()
{
    saveSettings();
}

bool ServerConfig::save(const QString& fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    save(file);
    file.close();

    return true;
}

void ServerConfig::save(QFile& file) const
{
    QTextStream outStream(&file);
    outStream << *this;
}

void ServerConfig::init()
{
    switchCorners().clear();
    screens().clear();

    // m_NumSwitchCorners is used as a fixed size array. See Screen::init()
    for (int i = 0; i < NumSwitchCorners; i++)
        switchCorners() << false;

    // There must always be screen objects for each cell in the screens QList. Unused screens
    // are identified by having an empty name.
    for (int i = 0; i < numColumns() * numRows(); i++)
        addScreen(Screen());
}

void ServerConfig::saveSettings()
{
    settings().beginGroup("internalConfig");
    settings().remove("");

    settings().setValue("numColumns", numColumns());
    settings().setValue("numRows", numRows());

    settings().setValue("hasHeartbeat", hasHeartbeat());
    settings().setValue("heartbeat", heartbeat());
    settings().setValue("relativeMouseMoves", relativeMouseMoves());
    settings().setValue("screenSaverSync", screenSaverSync());
    settings().setValue("win32KeepForeground", win32KeepForeground());
    settings().setValue("hasSwitchDelay", hasSwitchDelay());
    settings().setValue("switchDelay", switchDelay());
    settings().setValue("hasSwitchDoubleTap", hasSwitchDoubleTap());
    settings().setValue("switchDoubleTap", switchDoubleTap());
    settings().setValue("switchCornerSize", switchCornerSize());
    settings().setValue("ignoreAutoConfigClient", ignoreAutoConfigClient());
    settings().setValue("enableDragAndDrop", enableDragAndDrop());
    settings().setValue("clipboardSharing", clipboardSharing());

    writeSettings(settings(), switchCorners(), "switchCorner");

    settings().beginWriteArray("screens");
    for (int i = 0; i < screens().size(); i++)
    {
        settings().setArrayIndex(i);
        screens()[i].saveSettings(settings());
    }
    settings().endArray();

    settings().beginWriteArray("hotkeys");
    for (int i = 0; i < hotkeys().size(); i++)
    {
        settings().setArrayIndex(i);
        hotkeys()[i].saveSettings(settings());
    }
    settings().endArray();

    settings().endGroup();
}

void ServerConfig::loadSettings()
{
    settings().beginGroup("internalConfig");

    setNumColumns(settings().value("numColumns", 5).toInt());
    setNumRows(settings().value("numRows", 3).toInt());

    // we need to know the number of columns and rows before we can set up ourselves
    init();

    haveHeartbeat(settings().value("hasHeartbeat", false).toBool());
    setHeartbeat(settings().value("heartbeat", 5000).toInt());
    setRelativeMouseMoves(settings().value("relativeMouseMoves", false).toBool());
    setScreenSaverSync(settings().value("screenSaverSync", true).toBool());
    setWin32KeepForeground(settings().value("win32KeepForeground", false).toBool());
    haveSwitchDelay(settings().value("hasSwitchDelay", false).toBool());
    setSwitchDelay(settings().value("switchDelay", 250).toInt());
    haveSwitchDoubleTap(settings().value("hasSwitchDoubleTap", false).toBool());
    setSwitchDoubleTap(settings().value("switchDoubleTap", 250).toInt());
    setSwitchCornerSize(settings().value("switchCornerSize").toInt());
    setIgnoreAutoConfigClient(settings().value("ignoreAutoConfigClient").toBool());
    setEnableDragAndDrop(settings().value("enableDragAndDrop", true).toBool());
    setClipboardSharing(settings().value("clipboardSharing", true).toBool());

    readSettings(settings(), switchCorners(), "switchCorner", false, NumSwitchCorners);

    int numScreens = settings().beginReadArray("screens");
    Q_ASSERT(numScreens <= screens().size());
    for (int i = 0; i < numScreens; i++)
    {
        settings().setArrayIndex(i);
        screens()[i].loadSettings(settings());
    }
    settings().endArray();

    int numHotkeys = settings().beginReadArray("hotkeys");
    for (int i = 0; i < numHotkeys; i++)
    {
        settings().setArrayIndex(i);
        Hotkey h;
        h.loadSettings(settings());
        hotkeys().append(h);
    }
    settings().endArray();

    settings().endGroup();
}

int ServerConfig::adjacentScreenIndex(int idx, int deltaColumn, int deltaRow) const
{
    if (screens()[idx].isNull())
        return -1;

    // if we're at the left or right end of the table, don't find results going further left or right
    if ((deltaColumn > 0 && (idx+1) % numColumns() == 0)
            || (deltaColumn < 0 && idx % numColumns() == 0))
        return -1;

    int arrayPos = idx + deltaColumn + deltaRow * numColumns();

    if (arrayPos >= screens().size() || arrayPos < 0)
        return -1;

    return arrayPos;
}

QTextStream& operator<<(QTextStream& outStream, const ServerConfig& config)
{
    outStream << "section: screens" << endl;

    foreach (const Screen& s, config.screens())
        if (!s.isNull())
            s.writeScreensSection(outStream);

    outStream << "end" << endl << endl;

    outStream << "section: aliases" << endl;

    foreach (const Screen& s, config.screens())
        if (!s.isNull())
            s.writeAliasesSection(outStream);

    outStream << "end" << endl << endl;

    outStream << "section: links" << endl;

    for (int i = 0; i < config.screens().size(); i++)
        if (!config.screens()[i].isNull())
        {
            outStream << "\t" << config.screens()[i].name() << ":" << endl;

            for (unsigned int j = 0; j < sizeof(neighbourDirs) / sizeof(neighbourDirs[0]); j++)
            {
                int idx = config.adjacentScreenIndex(i, neighbourDirs[j].x, neighbourDirs[j].y);
                if (idx != -1 && !config.screens()[idx].isNull())
                    outStream << "\t\t" << neighbourDirs[j].name << " = " << config.screens()[idx].name() << endl;
            }
        }

    outStream << "end" << endl << endl;

    outStream << "section: options" << endl;

    if (config.hasHeartbeat())
        outStream << "\t" << "heartbeat = " << config.heartbeat() << endl;

    outStream << "\t" << "relativeMouseMoves = " << (config.relativeMouseMoves() ? "true" : "false") << endl;
    outStream << "\t" << "screenSaverSync = " << (config.screenSaverSync() ? "true" : "false") << endl;
    outStream << "\t" << "win32KeepForeground = " << (config.win32KeepForeground() ? "true" : "false") << endl;
    outStream << "\t" << "clipboardSharing = " << (config.clipboardSharing() ? "true" : "false") << endl;

    if (config.hasSwitchDelay())
        outStream << "\t" << "switchDelay = " << config.switchDelay() << endl;

    if (config.hasSwitchDoubleTap())
        outStream << "\t" << "switchDoubleTap = " << config.switchDoubleTap() << endl;

    outStream << "\t" << "switchCorners = none ";
    for (int i = 0; i < config.switchCorners().size(); i++)
        if (config.switchCorners()[i])
            outStream << "+" << config.switchCornerName(i) << " ";
    outStream << endl;

    outStream << "\t" << "switchCornerSize = " << config.switchCornerSize() << endl;

    foreach(const Hotkey& hotkey, config.hotkeys())
        outStream << hotkey;

    outStream << "end" << endl << endl;

    return outStream;
}

int ServerConfig::numScreens() const
{
    int rval = 0;

    foreach(const Screen& s, screens())
        if (!s.isNull())
            rval++;

    return rval;
}

int ServerConfig::autoAddScreen(const QString name)
{
    int serverIndex = -1;
    int targetIndex = -1;
    if (!findScreenName(m_ServerName, serverIndex)) {
        if (!fixNoServer(m_ServerName, serverIndex)) {
            return kAutoAddScreenManualServer;
        }
    }
    if (findScreenName(name, targetIndex)) {
        // already exists.
        return kAutoAddScreenIgnore;
    }

    int result = showAddClientDialog(name);

    if (result == kAddClientIgnore) {
        return kAutoAddScreenIgnore;
    }

    if (result == kAddClientOther) {
        addToFirstEmptyGrid(name);
        return kAutoAddScreenManualClient;
    }

    bool success = false;
    int startIndex = serverIndex;
    int offset = 1;
    int dirIndex = 0;

    if (result == kAddClientLeft) {
        offset = -1;
        dirIndex = 1;
    }
    else if (result == kAddClientUp) {
        offset = -5;
        dirIndex = 2;
    }
    else if (result == kAddClientDown) {
        offset = 5;
        dirIndex = 3;
    }


    int idx = adjacentScreenIndex(startIndex, neighbourDirs[dirIndex].x,
                    neighbourDirs[dirIndex].y);
    while (idx != -1) {
        if (screens()[idx].isNull()) {
            m_Screens[idx].setName(name);
            success = true;
            break;
        }

        startIndex += offset;
        idx = adjacentScreenIndex(startIndex, neighbourDirs[dirIndex].x,
                    neighbourDirs[dirIndex].y);
    }

    if (!success) {
        addToFirstEmptyGrid(name);
        return kAutoAddScreenManualClient;
    }

    saveSettings();
    return kAutoAddScreenOk;
}

bool ServerConfig::findScreenName(const QString& name, int& index)
{
    bool found = false;
    for (int i = 0; i < screens().size(); i++) {
        if (!screens()[i].isNull() &&
            screens()[i].name().compare(name) == 0) {
            index = i;
            found = true;
            break;
        }
    }
    return found;
}

bool ServerConfig::fixNoServer(const QString& name, int& index)
{
    bool fixed = false;
    if (screens()[serverDefaultIndex].isNull()) {
        m_Screens[serverDefaultIndex].setName(name);
        index = serverDefaultIndex;
        fixed = true;
    }

    return fixed;
}

int ServerConfig::showAddClientDialog(const QString& clientName)
{
    int result = kAddClientIgnore;

    if (!m_pMainWindow->isActiveWindow()) {
        m_pMainWindow->showNormal();
        m_pMainWindow->activateWindow();
    }

    AddClientDialog addClientDialog(clientName, m_pMainWindow);
    addClientDialog.exec();
    result = addClientDialog.addResult();
    m_IgnoreAutoConfigClient = addClientDialog.ignoreAutoConfigClient();

    return result;
}

void::ServerConfig::addToFirstEmptyGrid(const QString &clientName)
{
    for (int i = 0; i < screens().size(); i++) {
        if (screens()[i].isNull()) {
            m_Screens[i].setName(clientName);
            break;
        }
    }
}
