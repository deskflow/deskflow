// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "CandidatePolicy.h"
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamWriter>
namespace deskflow::candidate {
bool plainPath(const QString &p)
{
  if (p.isEmpty() || !QDir::isAbsolutePath(p) || p != QDir::cleanPath(p) || p.startsWith("//") || p.contains('\\') ||
      p.contains(QChar(0)))
    return false;
  for (const auto &part : p.split('/'))
    if (part.compare("Deskflow", Qt::CaseInsensitive) == 0 || part.compare("Deskflow.app", Qt::CaseInsensitive) == 0)
      return false;
  for (auto part = p; !part.isEmpty();) {
    const QFileInfo f(part);
    if (f.isSymLink())
      return false;
    const auto next = f.absolutePath();
    if (next == part)
      break;
    part = next;
  }
  return true;
}
bool settingsPath(const QString &p)
{
  return plainPath(p) && QFileInfo(p).fileName() == "DeskflowCandidate.conf" &&
         QFileInfo(QFileInfo(p).absolutePath()).fileName() == "DeskflowCandidate";
}
bool childPath(const QString &root, const QString &p)
{
  return plainPath(root) && plainPath(p) && p.startsWith(root + '/');
}
bool securityProfile(const QString &settings, const QString &cert, bool tls, bool peers)
{
  return settingsPath(settings) && tls && peers &&
         cert == QFileInfo(settings).absolutePath() + "/tls/deskflow-candidate.pem" && plainPath(cert) &&
         plainPath(QFileInfo(settings).absolutePath() + "/tls/trusted-servers") &&
         plainPath(QFileInfo(settings).absolutePath() + "/tls/trusted-clients");
}
bool mayResume(bool autoStart, bool paused)
{
  return autoStart && !paused;
}
QByteArray launchAgent(const QString &executable, bool recoverAfterCrash)
{
  if (!plainPath(executable) || !executable.endsWith("/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate"))
    return {};
  QByteArray bytes;
  QXmlStreamWriter w(&bytes);
  w.setAutoFormatting(true);
  w.writeStartDocument();
  w.writeDTD(
      "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
  );
  w.writeStartElement("plist");
  w.writeAttribute("version", "1.0");
  w.writeStartElement("dict");
  w.writeTextElement("key", "Label");
  w.writeTextElement("string", "org.deskflow.candidate.gui");
  w.writeTextElement("key", "ProgramArguments");
  w.writeStartElement("array");
  w.writeTextElement("string", executable);
  w.writeEndElement();
  w.writeTextElement("key", "RunAtLoad");
  w.writeEmptyElement("true");
  if (recoverAfterCrash) {
    w.writeTextElement("key", "KeepAlive");
    w.writeStartElement("dict");
    w.writeTextElement("key", "SuccessfulExit");
    w.writeEmptyElement("false");
    w.writeEndElement();
  }
  w.writeTextElement("key", "ThrottleInterval");
  w.writeTextElement("integer", "15");
  w.writeEndElement();
  w.writeEndElement();
  w.writeEndDocument();
  return bytes;
}
} // namespace deskflow::candidate
