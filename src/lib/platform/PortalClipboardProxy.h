/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Clipboard - Qt DBus interface for XDG Desktop Portal Clipboard
 * This replaces libportal for clipboard access to support InputCapture sessions.
 */

#pragma once

#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace deskflow {

/**
 * @brief Qt DBus interface for org.freedesktop.portal.Clipboard
 * 
 * This class provides clipboard access for XDG Desktop Portal sessions.
 * It can be used with both RemoteDesktop and InputCapture sessions that
 * support the ClipboardProvider interface.
 */
class PortalClipboardProxy : public QObject
{
    Q_OBJECT

public:
    static constexpr const char* PORTAL_SERVICE = "org.freedesktop.portal.Desktop";
    static constexpr const char* PORTAL_PATH = "/org/freedesktop/portal/desktop";
    static constexpr const char* CLIPBOARD_INTERFACE = "org.freedesktop.portal.Clipboard";

    explicit PortalClipboardProxy(QObject *parent = nullptr);
    ~PortalClipboardProxy() override;

    /**
     * @brief Initialize the clipboard for a session
     * @param sessionHandle The DBus object path of the session
     * @return true if initialization succeeded
     */
    bool init(const QDBusObjectPath &sessionHandle);

    /**
     * @brief Request clipboard access for the session
     * Must be called before the session starts.
     * @param options Optional parameters
     */
    void requestClipboard(const QVariantMap &options = {});

    /**
     * @brief Advertise clipboard content ownership
     * @param mimeTypes List of MIME types we have content for
     */
    void setSelection(const QStringList &mimeTypes);

    /**
     * @brief Read clipboard content
     * @param mimeType The MIME type to read
     * @return File descriptor to read clipboard data from
     */
    QDBusUnixFileDescriptor selectionRead(const QString &mimeType);

    /**
     * @brief Write clipboard content in response to SelectionTransfer
     * @param serial The transfer request serial
     * @return File descriptor to write clipboard data to
     */
    QDBusUnixFileDescriptor selectionWrite(quint32 serial);

    /**
     * @brief Signal completion of a SelectionTransfer
     * @param serial The transfer request serial
     * @param success Whether the transfer completed successfully
     */
    void selectionWriteDone(quint32 serial, bool success);

    /**
     * @brief Read data from the portal for a specific MIME type
     * @param mimeType The MIME type to read
     * @return The read data as QByteArray
     */
    QByteArray readSelectionData(const QString &mimeType);

    /**
     * @brief Write data to the portal in response to a selection transfer
     * @param serial The transfer request serial
     * @param data The data to write
     * @return true if write succeeded
     */
    bool writeSelectionData(quint32 serial, const QByteArray &data);

    /**
     * @brief Check if clipboard is enabled for this session
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Check if we own the clipboard selection
     */
    bool isOwner() const { return m_isOwner; }

    /**
     * @brief Get current available MIME types
     */
    QStringList availableMimeTypes() const { return m_mimeTypes; }

Q_SIGNALS:
    /**
     * @brief Emitted when clipboard ownership changes
     * @param mimeTypes Available MIME types
     * @param sessionIsOwner Whether our session owns the clipboard
     */
    void selectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner);

    /**
     * @brief Emitted when clipboard content is requested from us
     * @param mimeType The requested MIME type
     * @param serial Serial to use when responding
     */
    void selectionTransferRequested(const QString &mimeType, quint32 serial);

    /**
     * @brief Emitted when clipboard becomes available
     */
    void clipboardEnabled();

    /**
     * @brief Emitted on clipboard error
     */
    void clipboardError(const QString &error);

private Q_SLOTS:
    void onSelectionOwnerChanged(const QDBusObjectPath &sessionHandle, const QVariantMap &options);
    void onSelectionTransfer(const QDBusObjectPath &sessionHandle, const QString &mimeType, quint32 serial);

private:
    void connectSignals();
    void disconnectSignals();

    QByteArray readAllFromFd(int fd);
    bool writeAllToFd(int fd, const QByteArray &data);

    QDBusConnection m_bus;
    QDBusObjectPath m_sessionHandle;
    bool m_enabled = false;
    bool m_isOwner = false;
    QStringList m_mimeTypes;
};

} // namespace deskflow
