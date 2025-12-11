/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class I18NTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void creationTest();
  void detectedLangTest();
  void check639NameTest_validMapValues();
  void check639NameTest_invalidName();
  void toNativeNameTest_validMapValues();
  void toNativeNameTest_invalidName();
  void setLangTest_validLangs();
  void setLangTest_invalidLang();
  void setLangTest_currentLang();
  void reDetectTest();

private:
  QString m_myTDir;
  inline static const QString m_settingsPathTemp = QStringLiteral("tmp/test");
  inline static const QString m_settingsFile = QStringLiteral("%1/Deskflow.conf").arg(m_settingsPathTemp);
  inline static const QString m_stateFile = QStringLiteral("%1/Deskflow.state").arg(m_settingsPathTemp);
  inline static const QMap<QString, QString> m_langMap = {
      {QStringLiteral("English"), QStringLiteral("en")},  {QStringLiteral("Español"), QStringLiteral("es")},
      {QStringLiteral("Italiano"), QStringLiteral("it")}, {QStringLiteral("日本語"), QStringLiteral("ja")},
      {QStringLiteral("Русский"), QStringLiteral("ru")},  {QStringLiteral("简体中文"), QStringLiteral("zh_CN")}
  };
};
