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
