/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <QIODevice>
#include <QStringList>
#include <QUrl>

namespace deskflow::handoff {

struct Request
{
  QStringList files;
  QString application;
  QUrl url;
};

// Throws std::runtime_error on validation, transfer or filesystem errors.
void send(QIODevice &output, const Request &request);
Request receive(QIODevice &input, const QString &downloads);
bool validDestination(const QString &destination);
QStringList sshArguments(const QString &destination);

} // namespace deskflow::handoff
