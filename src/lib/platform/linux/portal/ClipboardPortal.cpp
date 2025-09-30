#include "ClipboardPortal.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QVariant>

static const char kBus[] = "org.freedesktop.portal.Desktop";
static const char kPath[] = "/org/freedesktop/portal/desktop";
static const char kIface[] = "org.freedesktop.portal.Clipboard";

ClipboardPortal::ClipboardPortal(const QDBusObjectPath &session, QObject *parent)
    : QObject(parent),
      m_clip(
          QString::fromLatin1(kBus), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
          QDBusConnection::sessionBus()
      ),
      m_session(session)
{
  // SelectionOwnerChanged(o a{sv})
  QDBusConnection::sessionBus().connect(
      QString::fromLatin1(kBus), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
      QStringLiteral("SelectionOwnerChanged"), this, SLOT(onSelectionOwnerChanged(QDBusObjectPath, QVariantMap))
  );

  // SelectionTransfer(o s u)
  QDBusConnection::sessionBus().connect(
      QString::fromLatin1(kBus), QString::fromLatin1(kPath), QString::fromLatin1(kIface),
      QStringLiteral("SelectionTransfer"), this, SLOT(onSelectionTransfer(QDBusObjectPath, QString, uint))
  );
}

bool ClipboardPortal::requestClipboard(const QVariantMap &options)
{
  const QDBusMessage r = m_clip.call(QStringLiteral("RequestClipboard"), QVariant::fromValue(m_session), options);
  return r.type() != QDBusMessage::ErrorMessage;
}

bool ClipboardPortal::setSelection(const QStringList &mimeTypes)
{
  QVariantMap opts;
  opts.insert(QStringLiteral("mime_types"), mimeTypes);
  const QDBusMessage r = m_clip.call(QStringLiteral("SetSelection"), QVariant::fromValue(m_session), opts);
  return r.type() != QDBusMessage::ErrorMessage;
}

QDBusUnixFileDescriptor ClipboardPortal::selectionRead(const QString &mimeType)
{
  QDBusPendingReply<QDBusUnixFileDescriptor> reply =
      m_clip.asyncCall(QStringLiteral("SelectionRead"), QVariant::fromValue(m_session), mimeType);
  reply.waitForFinished();
  if (reply.isError())
    return QDBusUnixFileDescriptor();
  return reply.value();
}

QDBusUnixFileDescriptor ClipboardPortal::selectionWrite(uint32_t serial)
{
  QDBusPendingReply<QDBusUnixFileDescriptor> reply =
      m_clip.asyncCall(QStringLiteral("SelectionWrite"), QVariant::fromValue(m_session), serial);
  reply.waitForFinished();
  if (reply.isError())
    return QDBusUnixFileDescriptor();
  return reply.value();
}

void ClipboardPortal::selectionWriteDone(uint32_t serial, bool success)
{
  m_clip.call(QStringLiteral("SelectionWriteDone"), QVariant::fromValue(m_session), serial, success);
}

void ClipboardPortal::onSelectionOwnerChanged(const QDBusObjectPath & /*session*/, const QVariantMap &options)
{
  const QStringList list = options.value(QStringLiteral("mime_types")).toStringList();
  const bool owner = options.value(QStringLiteral("session_is_owner")).toBool();
  Q_EMIT selectionOwnerChanged(list, owner);
}

void ClipboardPortal::onSelectionTransfer(const QDBusObjectPath & /*session*/, const QString &mimeType, uint serial)
{
  Q_EMIT selectionTransfer(mimeType, serial);
}
