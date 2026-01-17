/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Portal Clipboard - Qt DBus interface for XDG Desktop Portal Clipboard
 * This replaces libportal for clipboard access to support InputCapture sessions.
 */

#pragma once

#include <QObject>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QStringList>
#include <QVariantMap>

namespace deskflow {

/**
 * @brief Qt DBus Proxy for org.freedesktop.portal.Clipboard
 *
 * This class handles the low-level D-Bus communication with the XDG Desktop Portal.
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
   * @param sessionHandle The session object path
   * @return true if initialization succeeded
   */
  bool init(const QDBusObjectPath &sessionHandle);

  /**
   * @brief Request clipboard access for the session
   * @param options Additional options (e.g. for selection monitor)
   */
  void requestClipboard(const QVariantMap &options = QVariantMap());

  /**
   * @brief Set the clipboard selection
   * @param mimeTypes List of available MIME types
   */
  void setSelection(const QStringList &mimeTypes);

  /**
   * @brief Request to read clipboard data
   * @param mimeType The requested MIME type
   * @return A file descriptor to read from
   */
  QDBusUnixFileDescriptor selectionRead(const QString &mimeType);

  /**
   * @brief Request to write clipboard data (responding to transfer request)
   * @param serial The transfer serial
   * @return A file descriptor to write to
   */
  QDBusUnixFileDescriptor selectionWrite(quint32 serial);

  /**
   * @brief Signal that writing is finished
   * @param serial The transfer serial
   * @param success Whether the write was successful
   */
  void selectionWriteDone(quint32 serial, bool success);

  /**
   * @brief Helper to read all data from a selection FD
   */
  QByteArray readSelectionData(const QString &mimeType);

  /**
   * @brief Helper to write all data to a selection FD
   */
  bool writeSelectionData(quint32 serial, const QByteArray &data);

  // Getters
  bool isEnabled() const;
  bool isOwner() const;
  QStringList mimeTypes() const;

Q_SIGNALS:
  /**
   * @brief Emitted when clipboard ownership changes
   * @param mimeTypes Available MIME types
   * @param sessionIsOwner Whether our session owns the clipboard
   * @param type The type of selection (Standard or Primary)
   */
  void selectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner, SelectionType type);

  /**
   * @brief Emitted when clipboard content is requested from us
   * @param mimeType The requested MIME type
   * @param serial Serial to use when responding
   * @param type The type of selection (Standard or Primary)
   */
  void selectionTransferRequested(const QString &mimeType, quint32 serial, SelectionType type);

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
