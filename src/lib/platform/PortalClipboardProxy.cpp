/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Clipboard - Qt DBus implementation for XDG Desktop Portal Clipboard
 */

#include "PortalClipboardProxy.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <QFile>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

namespace deskflow {

PortalClipboardProxy::PortalClipboardProxy(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::sessionBus())
{
}

PortalClipboardProxy::~PortalClipboardProxy()
{
    disconnectSignals();
}

bool PortalClipboardProxy::init(const QDBusObjectPath &sessionHandle)
{
    if (!m_bus.isConnected()) {
        qWarning() << "PortalClipboardProxy: Session bus not connected";
        return false;
    }

    m_sessionHandle = sessionHandle;
    connectSignals();
    return true;
}

void PortalClipboardProxy::requestClipboard(const QVariantMap &options)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "RequestClipboard"
    );

    msg << QVariant::fromValue(m_sessionHandle) << options;

    QDBusPendingReply<> reply = m_bus.asyncCall(msg);
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << "PortalClipboardProxy: RequestClipboard failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
    } else {
        m_enabled = true;
        Q_EMIT clipboardEnabled();
    }
}

void PortalClipboardProxy::setSelection(const QStringList &mimeTypes)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboardProxy: setSelection called but clipboard not enabled";
        return;
    }

    QVariantMap options;
    options["mime_types"] = mimeTypes;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SetSelection"
    );

    msg << QVariant::fromValue(m_sessionHandle) << options;

    QDBusPendingReply<> reply = m_bus.asyncCall(msg);
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << "PortalClipboardProxy: SetSelection failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
    } else {
        m_isOwner = true;
        m_mimeTypes = mimeTypes;
    }
}

QDBusUnixFileDescriptor PortalClipboardProxy::selectionRead(const QString &mimeType)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboardProxy: selectionRead called but clipboard not enabled";
        return {};
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionRead"
    );

    msg << QVariant::fromValue(m_sessionHandle) << mimeType;

    QDBusReply<QDBusUnixFileDescriptor> reply = m_bus.call(msg);

    if (!reply.isValid()) {
        qWarning() << "PortalClipboardProxy: SelectionRead failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
        return {};
    }

    return reply.value();
}

QDBusUnixFileDescriptor PortalClipboardProxy::selectionWrite(quint32 serial)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboardProxy: selectionWrite called but clipboard not enabled";
        return {};
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionWrite"
    );

    msg << QVariant::fromValue(m_sessionHandle) << serial;

    QDBusReply<QDBusUnixFileDescriptor> reply = m_bus.call(msg);

    if (!reply.isValid()) {
        qWarning() << "PortalClipboardProxy: SelectionWrite failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
        return {};
    }

    return reply.value();
}

void PortalClipboardProxy::selectionWriteDone(quint32 serial, bool success)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboardProxy: selectionWriteDone called but clipboard not enabled";
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionWriteDone"
    );

    msg << QVariant::fromValue(m_sessionHandle) << serial << success;

    QDBusPendingReply<> reply = m_bus.asyncCall(msg);
    reply.waitForFinished();

    if (reply.isError()) {
        qWarning() << "PortalClipboardProxy: SelectionWriteDone failed:" << reply.error().message();
    }
}

void PortalClipboardProxy::connectSignals()
{
    // Connect to SelectionOwnerChanged signal
    m_bus.connect(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionOwnerChanged",
        this,
        SLOT(onSelectionOwnerChanged(QDBusObjectPath, QVariantMap))
    );

    // Connect to SelectionTransfer signal
    m_bus.connect(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionTransfer",
        this,
        SLOT(onSelectionTransfer(QDBusObjectPath, QString, quint32))
    );
}

void PortalClipboardProxy::disconnectSignals()
{
    m_bus.disconnect(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionOwnerChanged",
        this,
        SLOT(onSelectionOwnerChanged(QDBusObjectPath, QVariantMap))
    );

    m_bus.disconnect(
        PORTAL_SERVICE,
        PORTAL_PATH,
        CLIPBOARD_INTERFACE,
        "SelectionTransfer",
        this,
        SLOT(onSelectionTransfer(QDBusObjectPath, QString, quint32))
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

    m_mimeTypes = mimeTypes;
    m_isOwner = sessionIsOwner;

    Q_EMIT selectionOwnerChanged(mimeTypes, sessionIsOwner);
}

void PortalClipboardProxy::onSelectionTransfer(const QDBusObjectPath &sessionHandle, const QString &mimeType, quint32 serial)
{
    // Only handle signals for our session
    if (sessionHandle != m_sessionHandle) {
        return;
    }

    Q_EMIT selectionTransferRequested(mimeType, serial);
}

QByteArray PortalClipboardProxy::readSelectionData(const QString &mimeType)
{
    QDBusUnixFileDescriptor fdWrapper = selectionRead(mimeType);
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

bool PortalClipboardProxy::writeSelectionData(quint32 serial, const QByteArray &data)
{
    QDBusUnixFileDescriptor fdWrapper = selectionWrite(serial);
    if (!fdWrapper.isValid()) {
        return false;
    }

    int fd = fdWrapper.fileDescriptor();
    bool success = writeAllToFd(fd, data);
    
    ::close(fd);
    
    selectionWriteDone(serial, success);
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
            qWarning() << "PortalClipboardProxy: read error from FD" << fd << ":" << strerror(errno);
            break;
        }
    }

    return data;
}

bool PortalClipboardProxy::writeAllToFd(int fd, const QByteArray &data)
{
    const char *ptr = data.constData();
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
            qWarning() << "PortalClipboardProxy: write error to FD" << fd << ":" << strerror(errno);
            return false;
        }
    }

    return true;
}
