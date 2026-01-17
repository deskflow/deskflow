/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Clipboard - Qt DBus implementation for XDG Desktop Portal Clipboard
 */

#include "PortalClipboardProxy.h"
#include "base/Log.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QFile>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

namespace deskflow {

PortalClipboardProxy::PortalClipboardProxy(QObject *parent) : QObject(parent), m_bus(QDBusConnection::sessionBus())
{
}

PortalClipboardProxy::~PortalClipboardProxy()
{
  disconnectSignals();
}

bool PortalClipboardProxy::init(const QDBusObjectPath &sessionHandle)
{
  if (!m_bus.isConnected()) {
    LOG_ERR("PortalClipboardProxy: Session bus not connected");
    return false;
  }

  m_sessionHandle = sessionHandle;
  connectSignals();
  return true;
}

void PortalClipboardProxy::requestClipboard(const QVariantMap &options)
{
  QDBusMessage msg =
      QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "RequestClipboard");

  QVariantMap opts = options;
  if (m_activationId != 0 && !opts.contains("activation_id")) {
    opts["activation_id"] = m_activationId;
  }

  msg << QVariant::fromValue(m_sessionHandle) << opts;

  QDBusPendingReply<> reply = m_bus.asyncCall(msg);
  reply.waitForFinished();

  if (reply.isError()) {
    LOG_WARN("PortalClipboardProxy: RequestClipboard failed: %s", reply.error().message().toUtf8().constData());
    Q_EMIT clipboardError(reply.error().message());
  } else {
    m_enabled = true;
    Q_EMIT clipboardEnabled();
  }
}

void PortalClipboardProxy::setSelection(const QStringList &mimeTypes, SelectionType type)
{
  if (!m_enabled) {
    LOG_WARN("PortalClipboardProxy: setSelection called but clipboard not enabled");
    return;
  }

  QVariantMap options;
  options["mime_types"] = mimeTypes;
  options["selection_type"] = (uint)type;

  QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SetSelection");

  msg << QVariant::fromValue(m_sessionHandle) << options;

  QDBusPendingReply<> reply = m_bus.asyncCall(msg);
  reply.waitForFinished();

  if (reply.isError()) {
    LOG_WARN("PortalClipboardProxy: SetSelection failed: %s", reply.error().message().toUtf8().constData());
    Q_EMIT clipboardError(reply.error().message());
  } else {
    // Note: m_isOwner and m_mimeTypes are now handled per selection type in signals
  }
}

QDBusUnixFileDescriptor PortalClipboardProxy::selectionRead(const QString &mimeType, SelectionType type)
{
  if (!m_enabled) {
    LOG_WARN("PortalClipboardProxy: selectionRead called but clipboard not enabled");
    return {};
  }

  QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionRead");

  QVariantMap options;
  options["selection_type"] = (uint)type;

  msg << QVariant::fromValue(m_sessionHandle) << mimeType << options;

  QDBusReply<QDBusUnixFileDescriptor> reply = m_bus.call(msg);

  if (!reply.isValid()) {
    LOG_WARN("PortalClipboardProxy: SelectionRead failed: %s", reply.error().message().toUtf8().constData());
    Q_EMIT clipboardError(reply.error().message());
    return {};
  }

  return reply.value();
}

QDBusUnixFileDescriptor PortalClipboardProxy::selectionWrite(quint32 serial, SelectionType type)
{
  if (!m_enabled) {
    LOG_WARN("PortalClipboardProxy: selectionWrite called but clipboard not enabled");
    return {};
  }

  QDBusMessage msg = QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionWrite");

  QVariantMap options;
  options["selection_type"] = (uint)type;

  msg << QVariant::fromValue(m_sessionHandle) << serial << options;

  QDBusReply<QDBusUnixFileDescriptor> reply = m_bus.call(msg);

  if (!reply.isValid()) {
    LOG_WARN("PortalClipboardProxy: SelectionWrite failed: %s", reply.error().message().toUtf8().constData());
    Q_EMIT clipboardError(reply.error().message());
    return {};
  }

  return reply.value();
}

void PortalClipboardProxy::selectionWriteDone(quint32 serial, bool success, SelectionType type)
{
  if (!m_enabled) {
    LOG_WARN("PortalClipboardProxy: selectionWriteDone called but clipboard not enabled");
    return;
  }

  QDBusMessage msg =
      QDBusMessage::createMethodCall(PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionWriteDone");

  QVariantMap options;
  options["selection_type"] = (uint)type;

  msg << QVariant::fromValue(m_sessionHandle) << serial << success << options;

  QDBusPendingReply<> reply = m_bus.asyncCall(msg);
  reply.waitForFinished();

  if (reply.isError()) {
    LOG_WARN("PortalClipboardProxy: SelectionWriteDone failed: %s", reply.error().message().toUtf8().constData());
  }
}

void PortalClipboardProxy::connectSignals()
{
  // Connect to SelectionOwnerChanged signal
  m_bus.connect(
      PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionOwnerChanged", this,
      SLOT(onSelectionOwnerChanged(QDBusObjectPath, QVariantMap))
  );

  // Connect to SelectionTransfer signal
  m_bus.connect(
      PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionTransfer", this,
      SLOT(onSelectionTransfer(QDBusObjectPath, QString, quint32, QVariantMap))
  );
}

void PortalClipboardProxy::disconnectSignals()
{
  m_bus.disconnect(
      PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionOwnerChanged", this,
      SLOT(onSelectionOwnerChanged(QDBusObjectPath, QVariantMap))
  );

  m_bus.disconnect(
      PORTAL_SERVICE, PORTAL_PATH, CLIPBOARD_INTERFACE, "SelectionTransfer", this,
      SLOT(onSelectionTransfer(QDBusObjectPath, QString, quint32, QVariantMap))
  );
}

void PortalClipboardProxy::onSelectionOwnerChanged(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
{
  // Only handle signals for our session
  if (sessionHandle != m_sessionHandle) {
    return;
  }

  QStringList mimeTypes;
  bool sessionIsOwner = false;

  if (options.contains("mime_types")) {
    mimeTypes = options["mime_types"].toStringList();
  }

  if (options.contains("session_is_owner")) {
    sessionIsOwner = options["session_is_owner"].toBool();
  }

  SelectionType type = Standard;
  if (options.contains("selection_type")) {
    type = (SelectionType)options["selection_type"].toUInt();
  }

  if (type == Standard) {
    m_mimeTypes = mimeTypes;
    m_isOwner = sessionIsOwner;
  }

  Q_EMIT selectionOwnerChanged(mimeTypes, sessionIsOwner, type);
}

void PortalClipboardProxy::onSelectionTransfer(
    const QDBusObjectPath &sessionHandle, const QString &mimeType, quint32 serial, const QVariantMap &options
)
{
  // Only handle signals for our session
  if (sessionHandle != m_sessionHandle) {
    return;
  }

  SelectionType type = Standard;
  if (options.contains("selection_type")) {
    type = (SelectionType)options["selection_type"].toUInt();
  }

  Q_EMIT selectionTransferRequested(mimeType, serial, type);
}

QByteArray PortalClipboardProxy::readSelectionData(const QString &mimeType, SelectionType type)
{
  QDBusUnixFileDescriptor fdWrapper = selectionRead(mimeType, type);
  if (!fdWrapper.isValid()) {
    return {};
  }

  int fd = fdWrapper.fileDescriptor();
  QByteArray data = readAllFromFd(fd);

  // QDBusUnixFileDescriptor doesn't always close the FD on destruction
  // depending on ownership. We should close it if we're done.
  ::close(fd);

  return data;
}

bool PortalClipboardProxy::writeSelectionData(quint32 serial, const QByteArray &data, SelectionType type)
{
  QDBusUnixFileDescriptor fdWrapper = selectionWrite(serial, type);
  if (!fdWrapper.isValid()) {
    return false;
  }

  int fd = fdWrapper.fileDescriptor();
  bool success = writeAllToFd(fd, data);

  ::close(fd);

  selectionWriteDone(serial, success, type);
  return success;
}

QByteArray PortalClipboardProxy::readAllFromFd(int fd)
{
  QByteArray data;
  char buffer[4096];
  ssize_t n;

  while (true) {
    n = ::read(fd, buffer, sizeof(buffer));
    if (n > 0) {
      data.append(buffer, n);
    } else if (n == 0) {
      break; // EOF
    } else if (n < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      LOG_WARN("PortalClipboardProxy: read error from FD %d: %s", fd, strerror(errno));
      break;
    }
  }

  return data;
}

bool PortalClipboardProxy::writeAllToFd(int fd, const QByteArray &data)
{
  const char *ptr = data.data();
  size_t remaining = data.size();
  ssize_t n;

  while (remaining > 0) {
    n = ::write(fd, ptr, remaining);
    if (n > 0) {
      ptr += n;
      remaining -= n;
    } else if (n < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      LOG_WARN("PortalClipboardProxy: write error to FD %d: %s", fd, strerror(errno));
      return false;
    } else {
      break; // Should not happen with write
    }
  }

  return true;
}

{
  const char *ptr = data.constData();
  size_t remaining = (size_t)data.size();
  ssize_t n;

  while (remaining > 0) {
    n = ::write(fd, ptr, remaining);
    if (n > 0) {
      ptr += n;
      remaining -= (size_t)n;
    } else if (n < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
      LOG_WARN("PortalClipboardProxy: write error to FD %d: %s", fd, strerror(errno));
      return false;
    }
  }

  return true;
}

bool PortalClipboardProxy::isEnabled() const
{
  return m_enabled;
}

bool PortalClipboardProxy::isOwner() const
{
  return m_isOwner;
}

QStringList PortalClipboardProxy::mimeTypes() const
{
  return m_mimeTypes;
}

} // namespace deskflow
