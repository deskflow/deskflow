/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "KeyModifierID.h"

#include <QObject>
#include <QString>

inline static const auto kModifierNameAlt = "alt";
inline static const auto kModifierNameAltGr = "altgr";
inline static const auto kModifierNameCtrl = "ctrl";
inline static const auto kModifierNameMeta = "meta";
inline static const auto kModifierNameNone = "none";
inline static const auto kModifierNameShift = "shift";
inline static const auto kModifierNameSuper = "super";

enum class KeyboardModifier : int8_t
{
  DefaultMod = -1,
  Shift,
  Ctrl,
  Alt,
  Meta,
  Super,
  AltGr,
  None
};
Q_DECLARE_METATYPE(KeyboardModifier);

static QString keyboardModifierToOption(const KeyboardModifier modifier)
{
  using enum KeyboardModifier;
  switch (modifier) {
  case Alt:
    return kModifierNameAlt;
  case AltGr:
    return kModifierNameAltGr;
  case Ctrl:
    return kModifierNameCtrl;
  case Meta:
    return kModifierNameMeta;
  case None:
    return kModifierNameNone;
  case Shift:
    return kModifierNameShift;
  case Super:
    return kModifierNameSuper;
  default:
    return {};
  }
}

static QString valueToKeyboardModifierOption(int modifier)
{
  return keyboardModifierToOption(static_cast<KeyboardModifier>(modifier));
}

static KeyboardModifier keyboardModifierFromString(const QString &modifier)
{
  using enum KeyboardModifier;
  if (modifier.compare(kModifierNameShift, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("Shift")) == 0)
    return Shift;
  if (modifier.compare(kModifierNameCtrl, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("Ctrl")) == 0)
    return Ctrl;
  if (modifier.compare(kModifierNameAlt, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("Alt")) == 0)
    return Alt;
  if (modifier.compare(kModifierNameMeta, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("Meta")) == 0)
    return Meta;
  if (modifier.compare(kModifierNameSuper, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("Super")) == 0)
    return Super;
  if (modifier.compare(kModifierNameAltGr, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("AltGr")) == 0)
    return AltGr;
  if (modifier.compare(kModifierNameNone, Qt::CaseInsensitive) == 0 ||
      modifier.localeAwareCompare(QObject::tr("None")) == 0)
    return None;
  return DefaultMod;
}

static int modifierValueFromString(const QString &modifier)
{
  return static_cast<int>(keyboardModifierFromString(modifier));
}

static int32_t modifierIDValueFromString(const QString &modifier)
{
  auto mod = keyboardModifierToOption(keyboardModifierFromString(modifier));
  if (mod == kModifierNameShift)
    return static_cast<int32_t>(kKeyModifierIDShift);
  if (mod == kModifierNameCtrl)
    return static_cast<int32_t>(kKeyModifierIDControl);
  if (mod == kModifierNameAlt)
    return static_cast<int32_t>(kKeyModifierIDAlt);
  if (mod == kModifierNameAltGr)
    return static_cast<int32_t>(kKeyModifierIDAltGr);
  if (mod == kModifierNameMeta)
    return static_cast<int32_t>(kKeyModifierIDMeta);
  if (mod == kModifierNameSuper)
    return static_cast<int32_t>(kKeyModifierIDSuper);
  if (mod == kModifierNameNone)
    return static_cast<int32_t>(kKeyModifierIDNull);
  return static_cast<int32_t>(-1);
}
