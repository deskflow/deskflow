// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QByteArray>
#include <QVector>
namespace deskflow::ipc {
class LineBuffer
{
public:
  bool append(const QByteArray &chunk, QVector<QByteArray> &lines)
  {
    if (chunk.size() > 65536 || m_pending.size() + chunk.size() > 65536) {
      m_pending.clear();
      return false;
    }
    m_pending += chunk;
    qsizetype end;
    while ((end = m_pending.indexOf('\n')) >= 0) {
      lines.append(m_pending.left(end));
      m_pending.remove(0, end + 1);
    }
    return true;
  }

private:
  QByteArray m_pending;
};
} // namespace deskflow::ipc
