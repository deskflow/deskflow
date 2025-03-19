/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMessageLogContext>
#include <QString>

class QWidget;

namespace deskflow::gui::messages {

enum class ClientError
{
  AlreadyConnected,
  HostnameError,
  GenericError
};

enum class NewClientPromptResult
{
  Add,
  Ignore
};

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void raiseCriticalDialog();

void showFirstServerStartMessage(QWidget *parent);

void showFirstConnectedMessage(QWidget *parent, bool closeToTray, bool enableService, bool isServer);

void showCloseReminder(QWidget *parent);

void showClientConnectError(QWidget *parent, ClientError error, const QString &address);

NewClientPromptResult
showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth = false);

bool showClearSettings(QWidget *parent);

void showReadOnlySettings(QWidget *parent, const QString &systemSettingsPath);

void showWaylandLibraryError(QWidget *parent);

bool showUpdateCheckOption(QWidget *parent);

} // namespace deskflow::gui::messages
