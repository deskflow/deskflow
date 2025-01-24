/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QVariant>

#include "gui/proxy/QSettingsProxy.h"

/// @brief Screen configuration base class
class ScreenConfig
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;

public:
  enum class Modifier
  {
    DefaultMod = -1,
    Shift,
    Ctrl,
    Alt,
    Meta,
    Super,
    None,
    NumModifiers
  };
  enum class SwitchCorner
  {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    NumSwitchCorners
  };
  enum class Fix
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
  void readSettings(QSettingsProxy &settings, T1 &array, const QString &arrayName, const T2 &defaultValue)
  {
    int entries = settings.beginReadArray(arrayName + "Array");
    array.clear();
    for (int i = 0; i < entries; i++) {
      settings.setArrayIndex(i);
      QVariant v = settings.value(arrayName, defaultValue);
      array.append(v.value<T2>());
    }
    settings.endArray();
  }

  template <typename T1, typename T2>
  void readSettings(QSettingsProxy &settings, T1 &array, const QString &arrayName, const T2 &defaultValue, int entries)
  {
    Q_ASSERT(array.size() >= entries);
    settings.beginReadArray(arrayName + "Array");
    for (int i = 0; i < entries; i++) {
      settings.setArrayIndex(i);
      QVariant v = settings.value(arrayName, defaultValue);
      array[i] = v.value<T2>();
    }
    settings.endArray();
  }

  template <typename T> void writeSettings(QSettingsProxy &settings, const T &array, const QString &arrayName) const
  {
    settings.beginWriteArray(arrayName + "Array");
    for (int i = 0; i < array.size(); i++) {
      settings.setArrayIndex(i);
      settings.setValue(arrayName, array[i]);
    }
    settings.endArray();
  }

public:
  static const char *modifierName(int idx)
  {
    return m_ModifierNames[idx];
  }
  static const char *fixName(int idx)
  {
    return m_FixNames[idx];
  }
  static const char *switchCornerName(int idx)
  {
    return m_SwitchCornerNames[idx];
  }

private:
  static const char *m_ModifierNames[];
  static const char *m_FixNames[];
  static const char *m_SwitchCornerNames[];
};
