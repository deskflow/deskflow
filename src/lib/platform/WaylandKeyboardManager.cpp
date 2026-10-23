/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Applone <app1oneme@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/WaylandKeyboardManager.h"
#include "common/PlatformInfo.h"
#include "deskflow/unix/XkbLayoutsParser.h"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <chrono>

namespace deskflow::platform {
namespace {
constexpr auto service = "org.kde.keyboard";
constexpr auto path = "/Layouts";
constexpr auto interface = "org.kde.KeyboardLayouts";
QDBusMessage request(const char *method)
{
  return QDBusMessage::createMethodCall(service, path, interface, QString::fromLatin1(method));
}
} // namespace

WaylandKeyboardManager &WaylandKeyboardManager::instance()
{
  static WaylandKeyboardManager manager;
  return manager;
}

WaylandKeyboardManager::WaylandKeyboardManager()
{
  if (!isWayland()) {
    m_probed = true;
    return;
  }
  moveToThread(&m_thread);
  connect(&m_thread, &QThread::started, this, &WaylandKeyboardManager::start);
  qAddPostRoutine([] { instance().shutdown(); });
  m_thread.start();
}

void WaylandKeyboardManager::shutdown()
{
  if (!m_thread.isRunning())
    return;
  QMetaObject::invokeMethod(
      this,
      [this] {
        const auto timers = findChildren<QTimer *>();
        for (auto *timer : timers)
          timer->stop();
        moveToThread(QCoreApplication::instance()->thread());
      },
      Qt::BlockingQueuedConnection
  );
  m_thread.quit();
  m_thread.wait();
}

WaylandKeyboardManager::Snapshot WaylandKeyboardManager::snapshot() const
{
  std::lock_guard lock(m_mutex);
  return m_snapshot;
}

void WaylandKeyboardManager::waitForInitialProbe()
{
  std::unique_lock lock(m_mutex);
  m_ready.wait_for(lock, std::chrono::milliseconds(700), [this] { return m_probed; });
}

void WaylandKeyboardManager::start()
{
  auto bus = QDBusConnection::sessionBus();
  bus.connect(service, path, interface, "layoutChanged", this, SLOT(layoutChanged(uint)));
  bus.connect(service, path, interface, "layoutListChanged", this, SLOT(refreshLayouts()));
  auto *watcher = new QDBusServiceWatcher(service, bus, QDBusServiceWatcher::WatchForOwnerChange, this);
  connect(
      watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
      [this](const QString &, const QString &, const QString &owner) {
        if (owner.isEmpty()) {
          std::lock_guard lock(m_mutex);
          m_snapshot.group = -1;
          ++m_groupEpoch;
          if (m_listPending)
            m_listDirty = true;
        } else {
          refreshLayouts();
        }
      }
  );
  auto *timer = new QTimer(this);
  timer->setInterval(30000);
  connect(timer, &QTimer::timeout, this, &WaylandKeyboardManager::refreshLayouts);
  timer->start();
  refreshLayouts();
}

WaylandKeyboardManager::Snapshot
WaylandKeyboardManager::readConfiguration(const QString &file, const QStringList &runtimeLayouts)
{
  Snapshot result;
  result.layouts = runtimeLayouts.join(',').toStdString();
  if (file.isEmpty())
    return result;
  QSettings settings(file, QSettings::IniFormat);
  const auto configured = settings.value("Layout/LayoutList").toStringList();
  if (configured != runtimeLayouts)
    return result;
  const auto variants = settings.value("Layout/VariantList").toStringList();
  if (variants.size() > runtimeLayouts.size())
    return result;
  result.configured = true;
  result.variants = variants.join(',').toStdString();
  result.model = settings.value("Layout/Model").toString().toStdString();
  result.options = settings.value("Layout/Options").toStringList().join(',').toStdString();
  return result;
}

void WaylandKeyboardManager::refreshLayouts()
{
  if (m_listPending) {
    m_listDirty = true;
    return;
  }
  m_listPending = true;
  auto *call =
      new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(request("getLayoutsList"), 300), this);
  connect(call, &QDBusPendingCallWatcher::finished, this, [this, call] {
    const QDBusMessage reply = call->reply();
    call->deleteLater();
    m_listPending = false;
    if (m_listDirty) {
      m_listDirty = false;
      refreshLayouts();
      return;
    }
    QStringList layouts;
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
      const auto argument = reply.arguments().first().value<QDBusArgument>();
      if (argument.currentType() == QDBusArgument::ArrayType) {
        argument.beginArray();
        while (!argument.atEnd()) {
          QString layout, display, name;
          argument.beginStructure();
          argument >> layout >> display >> name;
          argument.endStructure();
          layouts.append(layout);
        }
        argument.endArray();
      }
    }
    if (!layouts.isEmpty() && !layouts.contains(QString())) {
      auto next = readConfiguration(QStandardPaths::locate(QStandardPaths::ConfigLocation, "kxkbrc"), layouts);
      // Without matching configuration the runtime list has no variant IDs.
      // Report languages, but do not compile an auxiliary translation map.
      for (const auto &layout : layouts)
        next.languages.push_back(XkbLayoutsParser::convertLayoutToISO(layout.toStdString()));
      {
        std::lock_guard lock(m_mutex);
        const bool changed = next.layouts != m_snapshot.layouts || next.variants != m_snapshot.variants ||
                             next.model != m_snapshot.model || next.options != m_snapshot.options ||
                             next.configured != m_snapshot.configured;
        next.generation = m_snapshot.generation + (changed ? 1 : 0);
        next.group = changed ? -1 : m_snapshot.group;
        m_snapshot = std::move(next);
      }
      refreshGroup();
    }
    {
      std::lock_guard lock(m_mutex);
      m_probed = true;
    }
    m_ready.notify_all();
  });
}

void WaylandKeyboardManager::refreshGroup()
{
  const auto epoch = m_groupEpoch;
  auto *call = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(request("getLayout"), 300), this);
  connect(call, &QDBusPendingCallWatcher::finished, this, [this, call, epoch] {
    const QDBusMessage reply = call->reply();
    call->deleteLater();
    if (epoch != m_groupEpoch)
      return;
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
      bool ok = false;
      const auto group = reply.arguments().first().toUInt(&ok);
      if (ok)
        layoutChanged(group);
    }
  });
}

void WaylandKeyboardManager::layoutChanged(uint group)
{
  ++m_groupEpoch;
  std::lock_guard lock(m_mutex);
  m_snapshot.group = group < m_snapshot.languages.size() ? static_cast<int32_t>(group) : -1;
}

} // namespace deskflow::platform
