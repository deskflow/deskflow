#include "ClipboardPortal.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QVariantMap>

static const char* kBus   = "org.freedesktop.portal.Desktop";
static const char* kPath  = "/org/freedesktop/portal/desktop";
static const char* kIface = "org.freedesktop.portal.Clipboard";

ClipboardPortal::ClipboardPortal(const QDBusObjectPath& session, QObject* parent)
  : QObject(parent)
  , m_clip(kBus, kPath, kIface, QDBusConnection::sessionBus())
  , m_session(session)
{
  QDBusConnection::sessionBus().connect(
    kBus, kPath, kIface, "SelectionOwnerChanged",
    this,
    [this](const QVariantMap &m){
      const QStringList list = m.value("mime_types").toStringList();
      const bool owner = m.value("session_is_owner").toBool();
      emit selectionOwnerChanged(list, owner);
    });

  QDBusConnection::sessionBus().connect(
    kBus, kPath, kIface, "SelectionTransfer",
    this,
    [this](const QString &mime, uint32_t serial){
      emit selectionTransfer(mime, serial);
    });
}

bool ClipboardPortal::requestClipboard() {
  QVariantMap opts;
  auto r = m_clip.call("RequestClipboard", QVariant::fromValue(m_session), opts);
  return r.type() != QDBusMessage::ErrorMessage;
}

bool ClipboardPortal::setSelection(const QStringList& types) {
  QVariantMap opts; opts.insert("mime_types", types);
  auto r = m_clip.call("SetSelection", QVariant::fromValue(m_session), opts);
  return r.type() != QDBusMessage::ErrorMessage;
}

QDBusUnixFileDescriptor ClipboardPortal::selectionRead(const QString& mt) {
  QDBusPendingReply<QDBusUnixFileDescriptor> r =
    m_clip.asyncCall("SelectionRead", QVariant::fromValue(m_session), mt);
  r.waitForFinished();
  return r.isError() ? QDBusUnixFileDescriptor() : r.value();
}

QDBusUnixFileDescriptor ClipboardPortal::selectionWrite(uint32_t serial) {
  QDBusPendingReply<QDBusUnixFileDescriptor> r =
    m_clip.asyncCall("SelectionWrite", QVariant::fromValue(m_session), serial);
  r.waitForFinished();
  return r.isError() ? QDBusUnixFileDescriptor() : r.value();
}

void ClipboardPortal::selectionWriteDone(uint32_t serial, bool ok) {
  m_clip.call("SelectionWriteDone", QVariant::fromValue(m_session), serial, ok);
}
