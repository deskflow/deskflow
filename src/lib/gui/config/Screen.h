/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ScreenConfig.h"

#include "gui/proxy/QSettingsProxy.h"

#include <QIcon>
#include <QList>
#include <QPixmap>
#include <QString>
#include <QStringList>

class QSettings;
class QTextStream;
class ScreenSettingsDialog;

class Screen : public ScreenConfig
{
  using QSettingsProxy = deskflow::gui::proxy::QSettingsProxy;

  friend class ScreenSettingsDialog;
  friend class ScreenSetupModel;
  friend class ScreenSetupView;

  friend QDataStream &operator<<(QDataStream &outStream, const Screen &screen)
  {
    return outStream << screen.name() << screen.switchCornerSize() << screen.aliases() << screen.modifiers()
                     << screen.switchCorners() << screen.fixes() << screen.isServer();
  }

  friend QDataStream &operator>>(QDataStream &inStream, Screen &screen)
  {
    return inStream >> screen.m_Name >> screen.m_SwitchCornerSize >> screen.m_Aliases >> screen.m_Modifiers >>
           screen.m_SwitchCorners >> screen.m_Fixes >> screen.m_isServer;
  }

public:
  explicit Screen();
  explicit Screen(const QString &name);

  const QPixmap &pixmap() const
  {
    return m_Pixmap;
  }
  const QString &name() const
  {
    return m_Name;
  }
  const QStringList &aliases() const
  {
    return m_Aliases;
  }

  bool isNull() const
  {
    return m_Name.isEmpty();
  }
  int modifier(int m) const
  {
    return m_Modifiers[m] == static_cast<int>(ScreenConfig::Modifier::DefaultMod) ? m : m_Modifiers[m];
  }
  const QList<int> &modifiers() const
  {
    return m_Modifiers;
  }
  bool switchCorner(int c) const
  {
    return m_SwitchCorners[c];
  }
  const QList<bool> &switchCorners() const
  {
    return m_SwitchCorners;
  }
  int switchCornerSize() const
  {
    return m_SwitchCornerSize;
  }
  bool fix(Fix f) const
  {
    return m_Fixes[static_cast<int>(f)];
  }
  const QList<bool> &fixes() const
  {
    return m_Fixes;
  }

  void loadSettings(QSettingsProxy &settings);
  void saveSettings(QSettingsProxy &settings) const;
  QString screensSection() const;
  QString aliasesSection() const;

  bool swapped() const
  {
    return m_Swapped;
  }
  QString &name()
  {
    return m_Name;
  }
  void setName(const QString &name)
  {
    m_Name = name;
  }
  bool isServer() const
  {
    return m_isServer;
  }
  void markAsServer()
  {
    m_isServer = true;
  }

  bool operator==(const Screen &screen) const;

protected:
  void init();

  QStringList &aliases()
  {
    return m_Aliases;
  }
  void setModifier(int m, int n)
  {
    m_Modifiers[m] = n;
  }
  QList<int> &modifiers()
  {
    return m_Modifiers;
  }
  void addAlias(const QString &alias)
  {
    m_Aliases.append(alias);
  }
  void setSwitchCorner(int c, bool on)
  {
    m_SwitchCorners[c] = on;
  }
  QList<bool> &switchCorners()
  {
    return m_SwitchCorners;
  }
  void setSwitchCornerSize(int val)
  {
    m_SwitchCornerSize = val;
  }
  void setFix(int f, bool on)
  {
    m_Fixes[f] = on;
  }
  QList<bool> &fixes()
  {
    return m_Fixes;
  }
  void setSwapped(bool on)
  {
    m_Swapped = on;
  }

private:
  QPixmap m_Pixmap = QIcon::fromTheme("video-display").pixmap(QSize(96, 96));
  QString m_Name;
  QStringList m_Aliases;
  QList<int> m_Modifiers;
  QList<bool> m_SwitchCorners;
  int m_SwitchCornerSize;
  QList<bool> m_Fixes;
  bool m_Swapped = false;
  bool m_isServer = false;
};
