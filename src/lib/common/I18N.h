/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMap>
#include <QObject>

class QTranslator;
/**
 * @brief The I18N singleton class handles detection and loading of translation files
 */
class I18N : public QObject
{
  Q_OBJECT

public:
  static I18N *instance();
  /**
   * @brief detectedLanguages
   * @return List of detected languages  (native names: English, Español etc..)
   */
  static QStringList detectedLanguages();

  /**
   * @brief nativeTo639Name Convert a native Language name into a 639 name
   * @param nativeName English, Español etc..
   * @return  639 name for the language e, zh_CN , it, etc..)
   */
  static QString nativeTo639Name(QString nativeName);

  /**
   * @brief toNativeName Convert a 639 Name into a Native Language string
   * @param shortName A 639 name en, es etc...
   * @return  native language string for the language
   */
  static QString toNativeName(QString shortName);

  /**
   * @brief currentLanguage
   * @return The current language string (639-1 names i.e en, es)
   */
  static QString currentLanguage();

  /**
   * @brief setLanguage Sets the current language
   * @param langName The language name must be an is 639-1 name
   */
  static void setLanguage(const QString &langName);

  /**
   * @brief detectLanguages Detect new language files
   */
  static void reDetectLanguages();

Q_SIGNALS:
  /**
   * @brief languageChanged Emitted when the current language changes
   * @param language The current language (639-1 names i.e en, es)
   */
  void languageChanged(const QString language);
  /**
   * @brief langaugesChanged Emitted when the detected languages changes
   * @param languages The current list of languages (639-1 names i.e en, es)
   */
  void langaugesChanged(const QStringList languages);

private:
  explicit I18N(QObject *parent = nullptr);

  I18N *operator=(I18N &other) = delete;
  I18N(const I18N &other) = delete;
  ~I18N() override = default;
  void detectLanguages();

  QMap<QString, QStringList> m_translations;
  QMap<QString, QString> m_nameMap;
  QList<QTranslator *> m_currentTranslations;
  QString m_currentLang = QStringLiteral("en");
  QString m_appTrPath;
  QString m_qtTrPath;
};
