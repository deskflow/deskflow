/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ManagedSettings.h"

#if defined(Q_OS_MAC)

#import <Foundation/Foundation.h>

// Qt headers -- must come after Foundation to avoid symbol conflicts
#include <QHash>
#include <QString>
#include <QVariant>

#include "Settings.h"

static const QHash<QString, QString> &settingsToMDMKeyMap()
{
  static const QHash<QString, QString> map = {
      {Settings::Security::TlsEnabled, QStringLiteral("TLSEnabled")},
      {Settings::Core::Port, QStringLiteral("ServerPort")},
      {Settings::Log::Level, QStringLiteral("LogLevel")},
      {QStringLiteral("ClipboardSharingEnabled"), QStringLiteral("ClipboardSharingEnabled")},
  };
  return map;
}

static void registerMDMSuite()
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    if (bundleID) {
      [[NSUserDefaults standardUserDefaults] addSuiteNamed:bundleID];
    }
  });
}

static id managedValue(NSString *mdmKey)
{
  registerMDMSuite();
  return [[NSUserDefaults standardUserDefaults] objectForKey:mdmKey];
}

static NSString *toNSString(const QString &str)
{
  return [NSString stringWithUTF8String:str.toUtf8().constData()];
}

static QVariant toQVariant(id val)
{
  if (!val) {
    return QVariant();
  }
  if ([val isKindOfClass:[NSNumber class]]) {
    // Distinguish bool from int -- NSNumber wraps BOOL as __NSCFBoolean
    if (strcmp([val objCType], @encode(BOOL)) == 0 || strcmp([val objCType], @encode(bool)) == 0) {
      return QVariant([val boolValue]);
    }
    return QVariant([val intValue]);
  }
  if ([val isKindOfClass:[NSString class]]) {
    return QVariant(QString::fromNSString(val));
  }
  return QVariant();
}

#endif // Q_OS_MAC

namespace deskflow::settings::admin {

bool isManaged(const QString &settingsKey)
{
#if defined(Q_OS_MAC)
  const auto &map = settingsToMDMKeyMap();
  if (!map.contains(settingsKey)) {
    return false;
  }
  return managedValue(toNSString(map.value(settingsKey))) != nil;
#else
  Q_UNUSED(settingsKey)
  return false;
#endif
}

QVariant value(const QString &settingsKey)
{
#if defined(Q_OS_MAC)
  const auto &map = settingsToMDMKeyMap();
  if (!map.contains(settingsKey)) {
    return QVariant();
  }
  return toQVariant(managedValue(toNSString(map.value(settingsKey))));
#else
  Q_UNUSED(settingsKey)
  return QVariant();
#endif
}

} // namespace deskflow::settings::admin
