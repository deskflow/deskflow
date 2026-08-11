/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QVariant>

#include "common/QSettingsProxy.h"

/// @brief Screen configuration base class
class ScreenConfig
{

public:
  enum class Modifier : int8_t
  {
    DefaultMod = -1,
    Shift,
    Ctrl,
    Alt,
    Meta,
    Super,
    AltGr,
    None,
    NumModifiers
  };
  enum class SwitchCorner : int8_t
  {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    NumSwitchCorners
  };
  enum class Fix : int8_t
  {
    CapsLock,
    NumLock,
    ScrollLock,
    XTest,
    NumFixes
  };

protected:
  explicit ScreenConfig() = default;
  ~ScreenConfig() = default;

  template <typename T1, typename T2>
  void readSettings(
      QSettingsProxy &settings, T1 &array, const QString &arrayName, const QString &valueName, const T2 &defaultValue,
      int entries
  )
  {
    Q_ASSERT(array.size() >= entries);
    settings.beginReadArray(arrayName);
    for (int i = 0; i < entries; i++) {
      settings.setArrayIndex(i);
      QVariant v = settings.value(valueName, defaultValue);
      array[i] = v.value<T2>();
    }
    settings.endArray();
  }

  template <typename T>
  void writeSettings(QSettingsProxy &settings, const T &array, const QString &arrayName, const QString &valueName) const
  {
    settings.beginWriteArray(arrayName);
    for (int i = 0; i < array.size(); i++) {
      settings.setArrayIndex(i);
      settings.setValue(valueName, array[i]);
    }
    settings.endArray();
  }
};
