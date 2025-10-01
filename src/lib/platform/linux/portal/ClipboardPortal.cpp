#include "platform/linux/portal/ClipboardPortal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariant>

static const char kService[] = "org.freedesktop.portal.Desktop";
static const char kPath[] = "/org/freedesktop/portal/desktop";
static const char kIface[] = "org.freedesktop.portal.Clipboard";

ClipboardPortal::ClipboardPortal(const QDBusObjectPath &session, QObject *parent)
    : QObject(parent),
      m_clip(
          QString::fromLatin1(kService), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
          QDBusConnection::sessionBus(), this
      ),
      m_session(session)
{
  // Subscribe to Clipboard portal signals
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
  return reply.type() == QDBusMessage::ReplyMessage;
}

bool ClipboardPortal::setSelection(const QStringList &mimeTypes)
{
  QVariantMap props;
  props.insert(QStringLiteral("mime_types"), QVariant::fromValue(mimeTypes));
  QDBusMessage reply = m_clip.call(QStringLiteral("SetSelection"), props);
  return reply.type() == QDBusMessage::ReplyMessage;
}

QDBusUnixFileDescriptor ClipboardPortal::selectionRead(const QString &mimeType)
{
  QVariantMap props;
  props.insert(QStringLiteral("mime_type"), mimeType);
  QDBusMessage reply = m_clip.call(QStringLiteral("SelectionRead"), props);
  QDBusUnixFileDescriptor fd;
  if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty())
    fd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments().constFirst());
  return fd;
}

QDBusUnixFileDescriptor ClipboardPortal::selectionWrite(uint32_t serial)
{
  QDBusMessage reply = m_clip.call(QStringLiteral("SelectionWrite"), QVariant::fromValue(serial));
  QDBusUnixFileDescriptor fd;
  if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty())
    fd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments().constFirst());
  return fd;
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
