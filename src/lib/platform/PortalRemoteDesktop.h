/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <memory>

namespace deskflow {

class EiScreen;
class IClipboard;
class IEventQueue;
class PortalSessionProxy;
class PortalClipboard;

class PortalRemoteDesktop : public QObject
{
  Q_OBJECT

public:
  PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
  ~PortalRemoteDesktop() override;

  IClipboard *getClipboard() const;

signals:
  void clipboardChanged();

private:
  EiScreen *m_screen;
  IEventQueue *m_events;

  std::unique_ptr<PortalSessionProxy> m_sessionProxy;
  std::unique_ptr<PortalClipboard> m_clipboard;
};

} // namespace deskflow
