/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

#ifdef NDEBUG
const bool kDebug = false;
#else
const bool kDebug = true;
#endif

namespace deskflow::gui {

class Logger : public QObject
{
  Q_OBJECT

public:
  static Logger &instance()
  {
    return s_instance;
  }

  void loadEnvVars();
  void handleMessage(const QtMsgType type, const QString &fileLine, const QString &message);
  void logVerbose(const QString &message) const;

signals:
  void newLine(const QString &line);

private:
  static Logger s_instance;
  bool m_debug = kDebug;
  bool m_verbose = false;
};

inline void logVerbose(const QString &message)
{
  Logger::instance().logVerbose(message);
}

} // namespace deskflow::gui
