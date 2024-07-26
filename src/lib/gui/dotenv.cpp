/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dotenv.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QTextStream>
#include <qglobal.h>
#include <qprocess.h>

namespace synergy::gui {

QPair<QString, QString> getPair(const QString &line);

/**
 * @brief A _very_ basic Qt .env file parser.
 *
 * This function re-invents the wheel to save adding a library dependency.
 * It does not support many things that a proper .env parser would, such as
 * trailing comments, escaping characters, multiline values, etc.
 *
 * If this function is not sufficient, replace it with a library such as:
 * https://github.com/adeharo9/cpp-dotenv
 *
 * Or there's boost::trim ;-)
 */
void dotenv(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug("no .env file in current dir");
    return;
  }

  qDebug("loading env vars from: %s", filePath.toUtf8().constData());

  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.startsWith('#'))
      continue;

    auto [key, value] = getPair(line);
    if (key.isEmpty() || value.isEmpty())
      continue;

    auto saved = QByteArray(value.toUtf8());
    const auto &key_c = key.toUtf8().constData();
    const auto &saved_c = saved.constData();

    qDebug("%s=%s", key_c, saved_c);
    qputenv(key.toUtf8(), value.toUtf8());
  }
}

QString stripQuotes(const QString &value) {
  QString result = value;
  if (result.startsWith('"') && result.endsWith('"')) {
    result = result.mid(1, result.length() - 2);
  }
  return result;
}

QPair<QString, QString> getPair(const QString &line) {
  auto pos = line.indexOf('=');
  if (pos == -1) {
    return QPair<QString, QString>("", "");
  }

  QString key = line.left(pos);
  QString value = line.mid(pos + 1);

  return QPair<QString, QString>(key.trimmed(), stripQuotes(value.trimmed()));
}

} // namespace synergy::gui
