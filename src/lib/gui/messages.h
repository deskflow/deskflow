/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
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

NewClientPromptResult showNewClientPrompt(QWidget *parent, const QString &clientName);

bool showClearSettings(QWidget *parent);

void showReadOnlySettings(QWidget *parent, const QString &systemSettingsPath);

void showWaylandLibraryError(QWidget *parent);

bool showUpdateCheckOption(QWidget *parent);

} // namespace deskflow::gui::messages
