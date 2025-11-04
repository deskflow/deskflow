/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

namespace deskflow::gui {

class Logger : public QObject
{
  Q_OBJECT

public:
  static Logger *instance()
  {
    static Logger m;
    return &m;
  }

  void loadEnvVars();
  void handleMessage(const QtMsgType type, const QString &fileLine, const QString &message);

Q_SIGNALS:
  void newLine(const QString &line);

private:
#ifdef NDEBUG
  bool m_debug = false;
#else
  bool m_debug = true;
#endif
};

} // namespace deskflow::gui
