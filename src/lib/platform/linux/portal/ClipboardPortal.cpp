#include "platform/linux/portal/ClipboardPortal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QVariant>

ClipboardPortal::ClipboardPortal(const QDBusObjectPath &session, QObject *parent)
    : QObject(parent),
      m_clip(
          QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
          QDBusConnection::sessionBus()
      ),
      m_session(session)
{
  // Subscribe to portal signals
  QDBusConnection::sessionBus().connect(
      QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
      QStringLiteral("SelectionOwnerChanged"), this, SLOT(onSelectionOwnerChanged(QVariantMap))
  );

  QDBusConnection::sessionBus().connect(
      QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
      QStringLiteral("SelectionTransfer"), this, SLOT(onSelectionTransfer(QString, uint))
  );
}

bool ClipboardPortal::requestClipboard(const QVariantMap &options)
{
  QDBusMessage reply = m_clip.call(QStringLiteral("RequestClipboard"), QVariant::fromValue(m_session), options);
  return reply.type() != QDBusMessage::ErrorMessage;
}

bool ClipboardPortal::setSelection(const QStringList &mimeTypes)
{
  QVariantMap props;
  props.insert(QStringLiteral("mime_types"), mimeTypes);
  QDBusMessage reply = m_clip.call(QStringLiteral("SetSelection"), props);
  return reply.type() != QDBusMessage::ErrorMessage;
}

QDBusUnixFileDescriptor ClipboardPortal::selectionRead(const QString &mimeType)
{
  QVariantMap props;
  props.insert(QStringLiteral("mime_type"), mimeType);
  QDBusReply<QDBusUnixFileDescriptor> fd = m_clip.call(QStringLiteral("SelectionRead"), props);
  if (!fd.isValid())
    return QDBusUnixFileDescriptor();
  return fd.value();
}

QDBusUnixFileDescriptor ClipboardPortal::selectionWrite(uint32_t serial)
{
  QDBusReply<QDBusUnixFileDescriptor> fd = m_clip.call(QStringLiteral("SelectionWrite"), QVariant::fromValue(serial));
  if (!fd.isValid())
    return QDBusUnixFileDescriptor();
  return fd.value();
}

void ClipboardPortal::selectionWriteDone(uint32_t serial, bool success)
{
  m_clip.call(QStringLiteral("SelectionWriteDone"), QVariant::fromValue(serial), QVariant::fromValue(success));
}

void ClipboardPortal::onSelectionOwnerChanged(const QVariantMap &m)
{
  const QStringList list = m.value(QStringLiteral("mime_types")).toStringList();
  const bool owner = m.value(QStringLiteral("session_is_owner")).toBool();
  Q_EMIT selectionOwnerChanged(list, owner);
}

void ClipboardPortal::onSelectionTransfer(const QString &mime, uint32_t serial)
{
  Q_EMIT selectionTransfer(mime, serial);
}
