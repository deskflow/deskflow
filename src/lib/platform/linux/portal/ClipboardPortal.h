#pragma once
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class ClipboardPortal : public QObject
{
  Q_OBJECT
public:
  explicit ClipboardPortal(const QDBusObjectPath &session, QObject *parent = nullptr);

  // Must be called BEFORE RemoteDesktop.Start (per the portal spec).
  bool requestClipboard(const QVariantMap &options = QVariantMap());

  // Advertise offered formats: { "mime_types": as }
  bool setSelection(const QStringList &mimeTypes);

  // Read current clipboard as the given MIME type (returns FD to read)
  QDBusUnixFileDescriptor selectionRead(const QString &mimeType);

  // When we own the selection, the portal requests data via SelectionTransfer(...)
  // We open an FD with selectionWrite(serial), write the payload, then call selectionWriteDone(...)
  QDBusUnixFileDescriptor selectionWrite(uint32_t serial);
  void selectionWriteDone(uint32_t serial, bool success);

Q_SIGNALS:
  void selectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner);
  void selectionTransfer(const QString &mimeType, uint32_t serial);

private Q_SLOTS:
  void onSelectionOwnerChanged(const QVariantMap &m);
  void onSelectionTransfer(const QString &mime, uint32_t serial);

private:
  static constexpr const char *kService = "org.freedesktop.portal.Desktop";
  static constexpr const char *kPath = "/org/freedesktop/portal/desktop";
  static constexpr const char *kIface = "org.freedesktop.portal.Clipboard";

  QDBusInterface m_clip;
  QDBusObjectPath m_session;
};
