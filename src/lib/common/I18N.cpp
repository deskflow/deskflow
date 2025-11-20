/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "I18N.h"

#include "common/Constants.h"
#include "common/Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QList>
#include <QMap>
#include <QObject>
#include <QTranslator>

I18N *I18N::instance()
{
  static I18N m;
  return &m;
}

I18N::I18N(QObject *parent) : QObject{parent}
{
  const QList<QDir> appTrDirs{
      {QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), QStringLiteral("translations"))},
      {QStringLiteral("%1/../translations").arg(QCoreApplication::applicationDirPath())},
      {QStringLiteral("%1/../Resources/translations").arg(QCoreApplication::applicationDirPath())},
      {QStringLiteral("%1/../share/%2/translations").arg(QCoreApplication::applicationDirPath(), kAppId)},
      {QStringLiteral("%1/.local/share/%2/translations").arg(QDir::homePath(), kAppId)},
      {QStringLiteral("/usr/local/share/%1/translations").arg(kAppId)},
      {QStringLiteral("/usr/share/%1/translations").arg(kAppId)}
  };
  const QStringList appTrFilter{QStringLiteral("%1*.qm").arg(kAppId)};

  for (const auto &dir : appTrDirs) {
    if (!dir.entryList(appTrFilter, QDir::Files, QDir::Name).isEmpty()) {
      m_appTrPath = dir.absolutePath();
      break;
    }
  }

  if (m_appTrPath.isEmpty()) {
    qInfo() << "no app translations found";
  }

  const QList<QDir> qtTrDirs{
      {QStringLiteral("%1/%2").arg(QCoreApplication::applicationDirPath(), QStringLiteral("translations"))},
      {QStringLiteral("%1/../Resources/translations").arg(QCoreApplication::applicationDirPath())},
      {QStringLiteral("%1/../qt-depends/translations").arg(QCoreApplication::applicationDirPath())},
      {QStringLiteral("%1/../share/qt/translations").arg(QCoreApplication::applicationDirPath())},
      {QStringLiteral("%1/.local/share/%2/translations").arg(QDir::homePath(), QStringLiteral("qt"))},
      {QStringLiteral("/usr/local/share/qt/translations")},
      {QStringLiteral("/usr/share/qt/translations")}
  };
  const QStringList qtTrFilter{QStringLiteral("qt_*.qm")};

  for (const auto &dir : qtTrDirs) {
    if (!dir.entryList(qtTrFilter, QDir::Files, QDir::Name).isEmpty()) {
      m_qtTrPath = dir.absolutePath();
      break;
    }
  }

  if (m_qtTrPath.isEmpty()) {
    qInfo() << "no qt translations found";
  }

  detectLanguages();

  if (Settings::value(Settings::Core::Language).isNull()) {
    auto appTranslator = new QTranslator(this);
    if (appTranslator->load(QLocale(), kAppId, "_", m_appTrPath)) {
      m_currentTranslations.append(appTranslator);
      QCoreApplication::installTranslator(appTranslator);
    }

    m_currentLang = appTranslator->translate("i18n", "LocalizedName");
    if (m_currentLang.isEmpty())
      m_currentLang = QStringLiteral("English");

    auto qtTranslator = new QTranslator(this);
    if (qtTranslator->load(QLocale(), QStringLiteral("qt"), "_", m_qtTrPath)) {
      m_currentTranslations.append(qtTranslator);
      QCoreApplication::installTranslator(qtTranslator);
    }
  } else {
    m_currentLang = Settings::value(Settings::Core::Language).toString();
    const auto translations = m_translations.value(m_currentLang);
    for (const auto &translation : translations) {
      auto translator = new QTranslator(this);
      if (translator->load(translation)) {
        m_currentTranslations.append(translator);
        QCoreApplication::installTranslator(translator);
      }
    }
  }
}

QStringList I18N::detectedLanguages()
{
  return instance()->m_translations.keys();
}

QString I18N::currentLanguage()
{
  return instance()->m_currentLang;
}

void I18N::setLanguage(const QString &langName)
{
  if (langName == instance()->m_currentLang) {
    return;
  }

  if (!instance()->m_translations.contains(langName)) {
    return;
  }

  instance()->m_currentLang = langName;
  Settings::setValue(Settings::Core::Language, langName);

  for (const auto &translation : std::as_const(instance()->m_currentTranslations))
    QCoreApplication::removeTranslator(translation);

  qDeleteAll(instance()->m_currentTranslations);
  instance()->m_currentTranslations.clear();

  const auto translations = instance()->m_translations.value(langName);
  for (const auto &translation : translations) {
    auto translator = new QTranslator(instance());
    if (translator->load(translation)) {
      instance()->m_currentTranslations.append(translator);
      QCoreApplication::installTranslator(translator);
    }
  }

  Q_EMIT instance()->languageChanged(langName);
}

void I18N::reDetectLanguages()
{
  instance()->detectLanguages();
}

void I18N::detectLanguages()
{
  const auto oldList = m_translations;
  m_translations.clear();

  QStringList nameFilter = {QStringLiteral("%1_*.qm").arg(kAppId)};
  QMap<QString, QString> appTranslations;
  QMap<QString, QString> shortToNative;
  QStringList detectedLangCodes;
  QDir dir(m_appTrPath);
  QStringList langList = dir.entryList(nameFilter, QDir::Files, QDir::Name);

  for (const QString &translation : std::as_const(langList)) {
    QTranslator translator;
    std::ignore = translator.load(translation, dir.absolutePath());
    const auto longCode = translator.language();
    //: Replace with your Language name
    //: This is a required string
    QString nativeLang = translator.translate("i18n", "LocalizedName");
    if (nativeLang.isEmpty())
      nativeLang = QStringLiteral("English");

    QString shortCode;
    if (longCode.startsWith(QStringLiteral("zh")) || longCode.startsWith(QStringLiteral("pt")))
      shortCode = longCode;
    else
      shortCode = longCode.mid(0, 2);

    appTranslations.insert(shortCode, translator.filePath());
    shortToNative.insert(shortCode, nativeLang);
    detectedLangCodes.append(QStringLiteral("qt_%1.qm").arg(shortCode));
  }

  dir.setPath(m_qtTrPath);
  const static auto qtTrNameLen = 3; // length of qt_
  langList = dir.entryList(detectedLangCodes, QDir::Files, QDir::Name);

  QMap<QString, QString> qtTranslations;
  for (const QString &translation : std::as_const(langList)) {
    QString lang = translation.mid(qtTrNameLen, translation.lastIndexOf('.') - qtTrNameLen);
    qtTranslations.insert(lang, QStringLiteral("%1/%2").arg(m_qtTrPath, translation));
  }

  const QStringList keys = appTranslations.keys();
  for (const QString &lang : keys)
    m_translations.insert(shortToNative.value(lang), {appTranslations.value(lang), qtTranslations.value(lang)});

  if (oldList != m_translations)
    Q_EMIT langaugesChanged(m_translations.keys());
}
