/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Clipboard - Qt DBus implementation for XDG Desktop Portal Clipboard
 */

#include "PortalClipboard.h"

#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>

namespace deskflow {

PortalClipboard::PortalClipboard(QObject *parent)
    : QObject(parent)
    , m_bus(QDBusConnection::sessionBus())
{
}

PortalClipboard::~PortalClipboard()
{
    disconnectSignals();
}

bool PortalClipboard::init(const QDBusObjectPath &sessionHandle)
{
    if (!m_bus.isConnected()) {
        qWarning() << "PortalClipboard: Session bus not connected";
        return false;
    }

    m_sessionHandle = sessionHandle;
    connectSignals();
    return true;
}

void PortalClipboard::requestClipboard(const QVariantMap &options)
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
        qWarning() << "PortalClipboard: RequestClipboard failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
    } else {
        m_enabled = true;
        Q_EMIT clipboardEnabled();
    }
}

void PortalClipboard::setSelection(const QStringList &mimeTypes)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboard: setSelection called but clipboard not enabled";
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
        qWarning() << "PortalClipboard: SetSelection failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
    } else {
        m_isOwner = true;
        m_mimeTypes = mimeTypes;
    }
}

QDBusUnixFileDescriptor PortalClipboard::selectionRead(const QString &mimeType)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboard: selectionRead called but clipboard not enabled";
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
        qWarning() << "PortalClipboard: SelectionRead failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
        return {};
    }

    return reply.value();
}

QDBusUnixFileDescriptor PortalClipboard::selectionWrite(quint32 serial)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboard: selectionWrite called but clipboard not enabled";
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
        qWarning() << "PortalClipboard: SelectionWrite failed:" << reply.error().message();
        Q_EMIT clipboardError(reply.error().message());
        return {};
    }

    return reply.value();
}

void PortalClipboard::selectionWriteDone(quint32 serial, bool success)
{
    if (!m_enabled) {
        qWarning() << "PortalClipboard: selectionWriteDone called but clipboard not enabled";
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
        qWarning() << "PortalClipboard: SelectionWriteDone failed:" << reply.error().message();
    }
}

void PortalClipboard::connectSignals()
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

void PortalClipboard::disconnectSignals()
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

void PortalClipboard::onSelectionOwnerChanged(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
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

void PortalClipboard::onSelectionTransfer(const QDBusObjectPath &sessionHandle, const QString &mimeType, quint32 serial)
{
    // Only handle signals for our session
    if (sessionHandle != m_sessionHandle) {
        return;
    }

    Q_EMIT selectionTransferRequested(mimeType, serial);
}

} // namespace deskflow
