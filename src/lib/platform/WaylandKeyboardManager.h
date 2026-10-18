/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include <QObject>
#include <QStringList>
#include <QThread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

namespace deskflow::platform {

class WaylandKeyboardManager : public QObject
{
  Q_OBJECT
public:
  struct Snapshot
  {
    std::string layouts, variants, model, options;
    std::vector<std::string> languages;
    int32_t group = -1;
    uint64_t generation = 0;
    bool configured = false;
  };
  static WaylandKeyboardManager &instance();
  Snapshot snapshot() const;
  // Only used when enumerating languages at startup / connection time.
  void waitForInitialProbe();
  void shutdown();
  static Snapshot readConfiguration(const QString &path, const QStringList &runtimeLayouts);

private Q_SLOTS:
  void refreshLayouts();
  void refreshGroup();
  void layoutChanged(uint group);

private:
  WaylandKeyboardManager();
  void start();
  mutable std::mutex m_mutex;
  std::condition_variable m_ready;
  bool m_probed = false;
  bool m_listPending = false;
  bool m_listDirty = false;
  uint64_t m_groupEpoch = 0;
  Snapshot m_snapshot;
  QThread m_thread;
};

} // namespace deskflow::platform
