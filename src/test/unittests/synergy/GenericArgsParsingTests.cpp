/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "synergy/ArgParser.h"
#include "synergy/ArgsBase.h"
#include "test/mock/synergy/MockApp.h"

#include "test/global/gtest.h"

using namespace synergy;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool g_helpShowed    = false;
bool g_versionShowed = false;

void
showMockHelp () {
    g_helpShowed = true;
}

void
showMockVersion () {
    g_versionShowed = true;
}

TEST (GenericArgsParsingTests, parseGenericArgs_logLevelCmd_setLogLevel) {
    int i                          = 1;
    const int argc                 = 3;
    const char* kLogLevelCmd[argc] = {"stub", "--debug", "DEBUG"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kLogLevelCmd, i);

    String logFilter (argsBase.m_logFilter);

    EXPECT_EQ ("DEBUG", logFilter);
    EXPECT_EQ (2, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_logFileCmd_saveLogFilename) {
    int i                         = 1;
    const int argc                = 3;
    const char* kLogFileCmd[argc] = {"stub", "--log", "mock_filename"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kLogFileCmd, i);

    String logFile (argsBase.m_logFile);

    EXPECT_EQ ("mock_filename", logFile);
    EXPECT_EQ (2, i);
}

TEST (GenericArgsParsingTests,
      parseGenericArgs_logFileCmdWithSpace_saveLogFilename) {
    int i                                  = 1;
    const int argc                         = 3;
    const char* kLogFileCmdWithSpace[argc] = {
        "stub", "--log", "mo ck_filename"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kLogFileCmdWithSpace, i);

    String logFile (argsBase.m_logFile);

    EXPECT_EQ ("mo ck_filename", logFile);
    EXPECT_EQ (2, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_noDeamonCmd_daemonFalse) {
    int i                          = 1;
    const int argc                 = 2;
    const char* kNoDeamonCmd[argc] = {"stub", "-f"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kNoDeamonCmd, i);

    EXPECT_FALSE (argsBase.m_daemon);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_deamonCmd_daemonTrue) {
    int i                        = 1;
    const int argc               = 2;
    const char* kDeamonCmd[argc] = {"stub", "--daemon"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kDeamonCmd, i);

    EXPECT_EQ (true, argsBase.m_daemon);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_nameCmd_saveName) {
    int i                      = 1;
    const int argc             = 3;
    const char* kNameCmd[argc] = {"stub", "--name", "mock"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kNameCmd, i);

    EXPECT_EQ ("mock", argsBase.m_name);
    EXPECT_EQ (2, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_noRestartCmd_restartFalse) {
    int i                           = 1;
    const int argc                  = 2;
    const char* kNoRestartCmd[argc] = {"stub", "--no-restart"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kNoRestartCmd, i);

    EXPECT_FALSE (argsBase.m_restartable);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_restartCmd_restartTrue) {
    int i                         = 1;
    const int argc                = 2;
    const char* kRestartCmd[argc] = {"stub", "--restart"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kRestartCmd, i);

    EXPECT_EQ (true, argsBase.m_restartable);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_backendCmd_backendTrue) {
    int i                         = 1;
    const int argc                = 2;
    const char* kBackendCmd[argc] = {"stub", "-z"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kBackendCmd, i);

    EXPECT_EQ (true, argsBase.m_backend);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_noHookCmd_noHookTrue) {
    int i                        = 1;
    const int argc               = 2;
    const char* kNoHookCmd[argc] = {"stub", "--no-hooks"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kNoHookCmd, i);

    EXPECT_EQ (true, argsBase.m_noHooks);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_helpCmd_showHelp) {
    g_helpShowed               = false;
    int i                      = 1;
    const int argc             = 2;
    const char* kHelpCmd[argc] = {"stub", "--help"};

    NiceMock<MockApp> app;
    ArgParser argParser (&app);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);
    ON_CALL (app, help ()).WillByDefault (Invoke (showMockHelp));

    argParser.parseGenericArgs (argc, kHelpCmd, i);

    EXPECT_EQ (true, g_helpShowed);
    EXPECT_EQ (1, i);
}


TEST (GenericArgsParsingTests, parseGenericArgs_versionCmd_showVersion) {
    g_versionShowed               = false;
    int i                         = 1;
    const int argc                = 2;
    const char* kVersionCmd[argc] = {"stub", "--version"};

    NiceMock<MockApp> app;
    ArgParser argParser (&app);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);
    ON_CALL (app, version ()).WillByDefault (Invoke (showMockVersion));

    argParser.parseGenericArgs (argc, kVersionCmd, i);

    EXPECT_EQ (true, g_versionShowed);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_noTrayCmd_disableTrayTrue) {
    int i                        = 1;
    const int argc               = 2;
    const char* kNoTrayCmd[argc] = {"stub", "--no-tray"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kNoTrayCmd, i);

    EXPECT_EQ (true, argsBase.m_disableTray);
    EXPECT_EQ (1, i);
}

TEST (GenericArgsParsingTests, parseGenericArgs_ipcCmd_enableIpcTrue) {
    int i                     = 1;
    const int argc            = 2;
    const char* kIpcCmd[argc] = {"stub", "--ipc"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kIpcCmd, i);

    EXPECT_EQ (true, argsBase.m_enableIpc);
    EXPECT_EQ (1, i);
}

#ifndef WINAPI_XWINDOWS
TEST (GenericArgsParsingTests,
      parseGenericArgs_dragDropCmdOnNonLinux_enableDragDropTrue) {
    int i                          = 1;
    const int argc                 = 2;
    const char* kDragDropCmd[argc] = {"stub", "--enable-drag-drop"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kDragDropCmd, i);

    EXPECT_EQ (true, argsBase.m_enableDragDrop);
    EXPECT_EQ (1, i);
}
#endif

#ifdef WINAPI_XWINDOWS
TEST (GenericArgsParsingTests,
      parseGenericArgs_dragDropCmdOnLinux_enableDragDropFalse) {
    int i                          = 1;
    const int argc                 = 2;
    const char* kDragDropCmd[argc] = {"stub", "--enable-drag-drop"};

    ArgParser argParser (NULL);
    ArgsBase argsBase;
    argParser.setArgsBase (argsBase);

    argParser.parseGenericArgs (argc, kDragDropCmd, i);

    EXPECT_FALSE (argsBase.m_enableDragDrop);
    EXPECT_EQ (1, i);
}
#endif
