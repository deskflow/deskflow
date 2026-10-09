/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "Transfer.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QThread>

#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace deskflow::handoff {
namespace {
constexpr qint64 maxHeader = 1024 * 1024;
constexpr qint64 maxFileSize = qint64(1) << 40;
constexpr int maxFiles = 1024;

void require(bool condition, const QString &message)
{
  if (!condition)
    throw std::runtime_error(message.toStdString());
}

void writeAll(QIODevice &output, QByteArrayView bytes)
{
  while (!bytes.isEmpty()) {
    require(!QThread::currentThread()->isInterruptionRequested(), QStringLiteral("Transfer cancelled"));
    const auto count = output.write(bytes.data(), bytes.size());
    require(count > 0, QStringLiteral("Could not write transfer: %1").arg(output.errorString()));
    bytes = bytes.sliced(count);
    // Keep large copies out of QProcess's unbounded write buffer.
    QElapsedTimer deadline;
    deadline.start();
    while (output.bytesToWrite() > 65536) {
      require(!QThread::currentThread()->isInterruptionRequested(), QStringLiteral("Transfer cancelled"));
      require(deadline.elapsed() < 30000, QStringLiteral("Transfer stopped responding"));
      output.waitForBytesWritten(250);
    }
  }
}

QByteArray readExact(QIODevice &input, qint64 size)
{
  QByteArray result;
  while (result.size() < size) {
    const auto bytes = input.read(size - result.size());
    require(!bytes.isEmpty(), QStringLiteral("Transfer ended before all data arrived"));
    result += bytes;
  }
  return result;
}

void validate(const Request &request)
{
  static const QRegularExpression bundleId(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9.-]{0,254}$"));
  require(
      request.application.isEmpty() || bundleId.match(request.application).hasMatch(),
      QStringLiteral("Invalid application identifier")
  );
  require(
      request.url.isEmpty() ||
          (request.url.isValid() && !request.url.host().isEmpty() &&
           (request.url.scheme() == "https" || request.url.scheme() == "http") && request.url.userInfo().isEmpty()),
      QStringLiteral("Only HTTP and HTTPS URLs without embedded credentials can be sent")
  );
  require(request.url.isEmpty() || request.files.isEmpty(), QStringLiteral("Send files or a URL, not both"));
  require(
      !request.files.isEmpty() || !request.url.isEmpty() || !request.application.isEmpty(),
      QStringLiteral("Nothing to send")
  );
  require(request.files.size() <= maxFiles, QStringLiteral("Too many files (maximum 1024)"));
}

void validateName(const QString &name, QSet<QString> &names)
{
  require(
      !name.isEmpty() && name != "." && name != ".." && !name.contains('/') && !name.contains('\\') &&
          !name.contains(QChar(0)) && name.toUtf8().size() <= 255,
      QStringLiteral("Invalid filename")
  );
  const auto key = name.normalized(QString::NormalizationForm_C).toCaseFolded();
  require(!names.contains(key), QStringLiteral("Duplicate filename: %1. Send these files separately.").arg(name));
  names.insert(key);
}
} // namespace

bool validDestination(const QString &destination)
{
  static const QRegularExpression pattern(QStringLiteral("^(?:[A-Za-z0-9_][A-Za-z0-9_.-]*@)?[A-Za-z0-9][A-Za-z0-9.-]*$")
  );
  return destination.size() <= 255 && pattern.match(destination).hasMatch();
}

QStringList sshArguments(const QString &destination)
{
  require(validDestination(destination), QStringLiteral("Use an SSH host alias or user@hostname"));
  return {
      "-T",
      "-oBatchMode=yes",
      "-oStrictHostKeyChecking=yes",
      "-oConnectTimeout=10",
      "-oServerAliveInterval=15",
      "-oServerAliveCountMax=2",
      "--",
      destination,
      "'/Applications/Deskflow.app/Contents/MacOS/deskflow-handoff'"
  };
}

void send(QIODevice &output, const Request &request)
{
  validate(request);
  QJsonArray entries;
  QSet<QString> names;
  for (const auto &path : request.files) {
    const QFileInfo file(path);
    require(file.isFile() && !file.isSymLink(), QStringLiteral("Only regular files can be sent: %1").arg(path));
    require(file.size() >= 0 && file.size() <= maxFileSize, QStringLiteral("File is too large: %1").arg(path));
    validateName(file.fileName(), names);
    entries.append(QJsonObject{{"name", file.fileName()}, {"size", QString::number(file.size())}});
  }
  const auto header = QJsonDocument(
                          QJsonObject{
                              {"version", 1},
                              {"files", entries},
                              {"application", request.application},
                              {"url", request.url.toString(QUrl::FullyEncoded)}
                          }
  ).toJson(QJsonDocument::Compact);
  require(header.size() <= maxHeader, QStringLiteral("Transfer header is too large"));
  writeAll(output, QByteArray::number(header.size()) + '\n');
  writeAll(output, header);
  for (qsizetype i = 0; i < request.files.size(); ++i) {
    QFile file(request.files.at(i));
    const auto fd = ::open(QFile::encodeName(file.fileName()).constData(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    require(fd >= 0, QStringLiteral("Could not read %1").arg(file.fileName()));
    if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
      ::close(fd);
      throw std::runtime_error("Could not read file");
    }
    struct stat status{};
    require(::fstat(fd, &status) == 0 && S_ISREG(status.st_mode), QStringLiteral("Source is no longer a regular file"));
    const auto size = entries.at(i).toObject().value("size").toString().toLongLong();
    const auto modified = QFileInfo(file).lastModified();
    require(file.size() == size, QStringLiteral("File changed before transfer"));
    for (qint64 remaining = size; remaining > 0;) {
      const auto bytes = file.read(qMin<qint64>(remaining, 65536));
      require(!bytes.isEmpty(), QStringLiteral("File changed or could not be read"));
      writeAll(output, bytes);
      remaining -= bytes.size();
    }
    require(
        file.size() == size && QFileInfo(file).lastModified() == modified,
        QStringLiteral("File changed during transfer; try again after saving")
    );
  }
  // A failed sender never writes this commit marker, so the receiver discards partial copies.
  writeAll(output, QByteArrayView("DONE", 4));
}

Request receive(QIODevice &input, const QString &downloads)
{
  QByteArray prefix;
  for (;;) {
    const auto byte = readExact(input, 1);
    if (byte == "\n")
      break;
    require(
        byte.at(0) >= '0' && byte.at(0) <= '9' && prefix.size() < 7, QStringLiteral("Invalid transfer header length")
    );
    prefix += byte;
  }
  bool okay = false;
  const auto size = prefix.toLongLong(&okay);
  require(okay && size > 0 && size <= maxHeader, QStringLiteral("Transfer header is too large or empty"));
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(readExact(input, size), &error);
  require(error.error == QJsonParseError::NoError && document.isObject(), QStringLiteral("Invalid transfer header"));
  const auto header = document.object();
  require(
      header.value("version").toInt() == 1 && header.value("files").isArray() &&
          header.value("application").isString() && header.value("url").isString(),
      QStringLiteral("Unsupported transfer header")
  );
  const auto entries = header.value("files").toArray();
  require(entries.size() <= maxFiles, QStringLiteral("Too many files"));
  Request request{{}, header.value("application").toString(), QUrl(header.value("url").toString(), QUrl::StrictMode)};
  QList<qint64> sizes;
  QSet<QString> names;
  for (const auto &entry : entries) {
    const auto object = entry.toObject();
    require(object.value("name").isString() && object.value("size").isString(), QStringLiteral("Invalid file entry"));
    const auto name = object.value("name").toString();
    validateName(name, names);
    const auto fileSize = object.value("size").toString().toLongLong(&okay);
    require(okay && fileSize >= 0 && fileSize <= maxFileSize, QStringLiteral("Invalid file size"));
    request.files.append(name);
    sizes.append(fileSize);
  }
  validate(request);
  require(QDir().mkpath(downloads), QStringLiteral("Could not create Downloads directory"));
  QTemporaryDir staging(QDir(downloads).filePath(".deskflow-XXXXXX"));
  require(staging.isValid(), QStringLiteral("Could not create transfer directory"));
  for (qsizetype i = 0; i < request.files.size(); ++i) {
    QFile file(QDir(staging.path()).filePath(request.files.at(i)));
    require(
        file.open(QIODevice::WriteOnly | QIODevice::NewOnly),
        QStringLiteral("Could not create received file (filename may collide)")
    );
    require(
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner), QStringLiteral("Could not protect received file")
    );
    for (qint64 remaining = sizes.at(i); remaining > 0;) {
      const auto bytes = readExact(input, qMin<qint64>(remaining, 65536));
      require(file.write(bytes) == bytes.size(), QStringLiteral("Could not save file (check free disk space)"));
      remaining -= bytes.size();
    }
    require(file.flush(), QStringLiteral("Could not finish received file"));
    file.close();
  }
  require(readExact(input, 4) == "DONE", QStringLiteral("Transfer did not complete"));
  require(input.read(1).isEmpty(), QStringLiteral("Unexpected data after transfer"));
  if (!request.files.isEmpty()) {
    const auto finalPath = QDir(downloads).filePath(QFileInfo(staging.path()).fileName().mid(1));
    require(QDir().rename(staging.path(), finalPath), QStringLiteral("Could not finish transfer directory"));
    for (auto &name : request.files)
      name = QDir(finalPath).filePath(name);
  }
  return request;
}
} // namespace deskflow::handoff
