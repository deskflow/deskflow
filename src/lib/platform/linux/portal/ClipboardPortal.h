#pragma once
#include <QObject>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusUnixFileDescriptor>
#include <QStringList>

class ClipboardPortal : public QObject {
  Q_OBJECT
public:
  explicit ClipboardPortal(const QDBusObjectPath& session, QObject* parent=nullptr);
  bool requestClipboard(); // call BEFORE RemoteDesktop.Start
  bool setSelection(const QStringList& mimeTypes);
  QDBusUnixFileDescriptor selectionRead(const QString& mimeType);
  QDBusUnixFileDescriptor selectionWrite(uint32_t serial);
  void selectionWriteDone(uint32_t serial, bool success);

Q_SIGNALS:
  void selectionOwnerChanged(QStringList mimeTypes, bool sessionIsOwner);
  void selectionTransfer(QString mimeType, uint32_t serial);

private:
  QDBusInterface m_clip;
  QDBusObjectPath m_session;
};
