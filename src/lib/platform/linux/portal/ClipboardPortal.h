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
  bool requestClipboard(const QVariantMap &options = QVariantMap()); // call BEFORE RemoteDesktop.Start
  bool setSelection(const QStringList &mimeTypes);
  QDBusUnixFileDescriptor selectionRead(const QString &mimeType);
  QDBusUnixFileDescriptor selectionWrite(uint32_t serial);
  void selectionWriteDone(uint32_t serial, bool success);

Q_SIGNALS:
  void selectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner);
  void selectionTransfer(const QString &mimeType, uint32_t serial);

private Q_SLOTS:
  // Match DBus signal signatures:
  // SelectionOwnerChanged(o a{sv})
  void onSelectionOwnerChanged(const QDBusObjectPath &session, const QVariantMap &options);
  // SelectionTransfer(o s u)
  void onSelectionTransfer(const QDBusObjectPath &session, const QString &mimeType, uint serial);

private:
  QDBusInterface m_clip;
  QDBusObjectPath m_session;
};
