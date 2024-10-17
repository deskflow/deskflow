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

#include "ScreenConfig.h"

#include "gui/proxy/QSettingsProxy.h"

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
  QTextStream &writeScreensSection(QTextStream &outStream) const;
  QTextStream &writeAliasesSection(QTextStream &outStream) const;

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
  QPixmap m_Pixmap = QPixmap(":res/icons/64x64/video-display.png");
  QString m_Name;
  QStringList m_Aliases;
  QList<int> m_Modifiers;
  QList<bool> m_SwitchCorners;
  int m_SwitchCornerSize;
  QList<bool> m_Fixes;
  bool m_Swapped = false;
  bool m_isServer = false;
};
