/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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
