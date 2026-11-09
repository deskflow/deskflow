// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DiscoveryCatalog.h"
#include <QRegularExpression>
#include <algorithm>
#include <limits>
namespace deskflow::gui {
QStringList DiscoveryCatalog::serviceNames(const QString &host, int port) const
{
  QStringList result;
  for (auto it = m_sources.cbegin(); it != m_sources.cend(); ++it)
    if (it->host == canonicalHost(host) && it->port == port && result.size() < 8)
      result << it.key();
  return result;
}
bool DiscoveryCatalog::validName(const QString &name)
{
  if (name.isEmpty() || name != name.trimmed() || name.toUtf8().size() > 63)
    return false;
  for (const auto ch : name) {
    if (ch.category() == QChar::Other_Control || ch.category() == QChar::Other_Format ||
        ch.category() == QChar::Other_Surrogate || ch == QChar::ReplacementCharacter ||
        ch.category() == QChar::Separator_Line || ch.category() == QChar::Separator_Paragraph)
      return false;
  }
  return true;
}
QString DiscoveryCatalog::canonicalHost(QString host)
{
  host = host.toLower();
  if (host.endsWith('.'))
    host.chop(1);
  return host;
}
bool DiscoveryCatalog::validEndpoint(const QString &host, int port)
{
  if (port < 1 || port > 65535 || host.size() > 254)
    return false;
  static const QRegularExpression pattern(
      "\\A(?:[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?\\.)+local\\.?\\z", QRegularExpression::CaseInsensitiveOption
  );
  return pattern.match(host).hasMatch();
}
bool DiscoveryCatalog::upsert(
    const QString &source, const QString &name, const QString &host, int port, qint64 nowMs, int leaseMs
)
{
  if (source.isEmpty() || source.size() > 300 || !validName(name) || !validEndpoint(host, port) || nowMs < 0 ||
      leaseMs < 0 || nowMs > std::numeric_limits<qint64>::max() - 120000)
    return false;
  if (leaseMs == 0) {
    remove(source);
    return true;
  }
  if (!m_sources.contains(source) && m_sources.size() >= 64)
    return false;
  m_sources[source] = {
      name.normalized(QString::NormalizationForm_C), canonicalHost(host), port, nowMs + std::min(leaseMs, 120000)
  };
  publish();
  return true;
}
void DiscoveryCatalog::remove(const QString &source)
{
  m_sources.remove(source);
  publish();
}
void DiscoveryCatalog::clear()
{
  m_sources.clear();
  publish();
}
void DiscoveryCatalog::expire(qint64 nowMs)
{
  for (auto it = m_sources.begin(); it != m_sources.end();) {
    if (it->expires <= nowMs)
      it = m_sources.erase(it);
    else
      ++it;
  }
  publish();
}
void DiscoveryCatalog::publish()
{
  QMap<QString, Entry> next;
  // Deduplicate network interfaces/aliases by endpoint, never by untrusted name.
  for (const auto &entry : std::as_const(m_sources)) {
    const auto key = entry.host + ':' + QString::number(entry.port);
    if (!next.contains(key))
      next.insert(key, entry);
  }
  const auto previous = m_visible;
  m_visible = next;
  for (auto it = previous.cbegin(); it != previous.cend(); ++it)
    if (!next.contains(it.key()))
      Q_EMIT removed(it.key());
  for (auto it = next.cbegin(); it != next.cend(); ++it) {
    if (!previous.contains(it.key()) || previous[it.key()].name != it->name)
      Q_EMIT found(it.key(), it->name, it->host, it->port);
  }
}
} // namespace deskflow::gui
