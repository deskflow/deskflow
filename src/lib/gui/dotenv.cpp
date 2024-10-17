/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "paths.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QTextStream>

namespace deskflow::gui {

QPair<QString, QString> getPair(const QString &line);

bool open(QFile &file, const QString &filePath)
{
  file.setFileName(filePath);
  return file.open(QIODevice::ReadOnly | QIODevice::Text);
}

/**
 * @brief A _very_ basic Qt .env file parser.
 *
 * This function re-invents the wheel to save adding a library dependency.
 * It does not support many things that a proper .env parser would, such as
 * trailing comments, escaping characters, multiline values, etc.
 *
 * If this function is not sufficient, replace it with a library such as:
 * https://github.com/adeharo9/cpp-dotenv
 */
void dotenv(const QString &filename)
{
  QString filePath = filename;
  QFile file;
  if (!open(file, filePath)) {
    QFileInfo fileInfo(filePath);
    qInfo("no %s file in dir: %s", qPrintable(filename), qPrintable(fileInfo.absolutePath()));

    // if nothing in current dir, then try the config dir.
    // this makes it a bit easier for engineers in the field to have an easily
    // predictable location for the .env file.
    const auto orgDir = paths::configDir();

    filePath = orgDir.filePath(filename);
    if (!open(file, filePath)) {
      qInfo("no %s file in app config dir: %s", qPrintable(filename), qPrintable(orgDir.absolutePath()));
      return;
    }
  }

  qInfo("loading env vars from: %s", qPrintable(filePath));

  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.startsWith('#'))
      continue;

    auto [key, value] = getPair(line);
    if (key.isEmpty() || value.isEmpty())
      continue;

    const auto keyBytes = key.toUtf8();
    const auto valueBytes = value.toUtf8();

    const auto &key_c = keyBytes.constData();
    const auto &value_c = valueBytes.constData();

    qDebug("%s=%s", key_c, value_c);
    qputenv(keyBytes, valueBytes);
  }
}

QString stripQuotes(const QString &value)
{
  QString result = value;
  if (result.startsWith('"') && result.endsWith('"')) {
    result = result.mid(1, result.length() - 2);
  }
  return result;
}

QPair<QString, QString> getPair(const QString &line)
{
  auto pos = line.indexOf('=');
  if (pos == -1) {
    return QPair<QString, QString>("", "");
  }

  QString key = line.left(pos);
  QString value = line.mid(pos + 1);

  return QPair<QString, QString>(key.trimmed(), stripQuotes(value.trimmed()));
}

} // namespace deskflow::gui
