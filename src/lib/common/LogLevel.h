/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include <QObject>
#include <QString>

class LogLevel : private QObject
{
  Q_OBJECT
  //! Log levels
  /*!
  The logging priority levels in order of highest to lowest priority.
  */
public:
  enum class Level
  {
    Print = -1, //!< For print only (no file or time)
    Fatal,      //!< For fatal errors
    Error,      //!< For serious errors
    Warning,    //!< For minor errors and warnings
    Info,       //!< For informational messages
    Debug,      //!< For important debugging messages
    Verbose     //!< For verbose debugging messages
  };
  Q_ENUM(Level)

  static QString toOption(const LogLevel::Level &level)
  {
    return toOption(static_cast<int>(level));
  }

  static QString toOption(const int &level)
  {
    if (level < 0 || level > m_levelOptions.size())
      return "";
    return m_levelOptions.at(level);
  }

  static LogLevel::Level fromOption(const QString &level)
  {
    const auto index = m_levelOptions.indexOf(level, 0, Qt::CaseInsensitive);
    if (index < 0 || index >= m_levelOptions.count())
      return LogLevel::Level::Info;
    return LogLevel::Level(index);
  }

  static QString toString(const LogLevel::Level &level)
  {
    return toString(static_cast<int>(level));
  }

  static QString toString(const int &level)
  {
    if (level < 0 || level > m_levelNames.size())
      return "";
    return tr(m_levelNames.at(level).toUtf8());
  }

  static QStringList logLevelOptions()
  {
    return m_levelOptions;
  }

  static QStringList logLevelNames()
  {
    return m_levelNames;
  }

private:
  // clang-format off
  /**
   * @brief m_levelOptions, Valid values for the log level options, Never translated
   */
  inline static const QStringList m_levelOptions {
    QStringLiteral("FATAL")
    , QStringLiteral("ERROR")
    , QStringLiteral("WARNING")
    , QStringLiteral("INFO")
    , QStringLiteral("DEBUG")
    , QStringLiteral("VERBOSE")
  };

  /**
   * @brief m_levelNames, Strings used to present level names to the user, Expected to be translated
   */
  inline static const QStringList m_levelNames = {
    QT_TR_NOOP("Fatal")
    , QT_TR_NOOP("Error")
    , QT_TR_NOOP("Warning")
    , QT_TR_NOOP("Info")
    , QT_TR_NOOP("Debug")
    , QT_TR_NOOP("Verbose")
  };
  // clang-format on
};
