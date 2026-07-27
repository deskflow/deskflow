/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Settings.h"

#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>
#import <Foundation/NSValue.h>

// Qt headers must come after Foundation to avoid symbol conflicts
#include <QString>
#include <QVariant>

#include <cstring>

bool Settings::isManaged(const QString &key)
{
  // objectIsForcedForKey: reports only values coming from a forced domain, so a
  // value the user wrote themselves is never mistaken for an enforced policy
  return [[NSUserDefaults standardUserDefaults] objectIsForcedForKey:key.toNSString()];
}

QVariant Settings::managedValue(const QString &key)
{
  if (!isManaged(key))
    return QVariant();

  id value = [[NSUserDefaults standardUserDefaults] objectForKey:key.toNSString()];
  if (!value)
    return QVariant();

  if ([value isKindOfClass:[NSNumber class]]) {
    // NSNumber wraps BOOL as __NSCFBoolean, so the encoded type is the only way
    // to tell a policy bool from a policy int
    const char *encodedType = [value objCType];
    if (strcmp(encodedType, @encode(BOOL)) == 0 || strcmp(encodedType, @encode(bool)) == 0)
      return QVariant([value boolValue]);
    return QVariant([value intValue]);
  }

  if ([value isKindOfClass:[NSString class]])
    return QVariant(QString::fromNSString(value));

  return QVariant();
}
