/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMessageLogContext>
#include <QString>

#include <common/Enums.h>

class QWidget;

namespace deskflow::gui::messages {

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void raiseCriticalDialog();

void showFirstServerStartMessage(QWidget *parent);

void showFirstConnectedMessage(QWidget *parent, bool closeToTray, bool enableService, bool isServer);

void showCloseReminder(QWidget *parent);

bool showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth = false);

bool showClearSettings(QWidget *parent);

void showReadOnlySettings(QWidget *parent, const QString &systemSettingsPath);

bool showUpdateCheckOption(QWidget *parent);

bool showDaemonOffline(QWidget *parent);

} // namespace deskflow::gui::messages
