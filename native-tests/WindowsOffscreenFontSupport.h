// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Test-only support: use the font observed by the read-only native Windows probe.
// Does not install fonts or change the user's application/system settings.
#pragma once
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRawFont>
#include <QStyle>
#include <QStyleFactory>

inline bool prepareWindowsOffscreenFont(QApplication &app)
{
#ifdef Q_OS_WIN
  if (QGuiApplication::platformName() != "offscreen")
    return true;
  const auto family = qEnvironmentVariable("DESKFLOW_TEST_FONT_FAMILY");
  if (family.isEmpty())
    return true; // Other hosts retain their existing test setup.
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (!QDir::isAbsolutePath(root) || !QDir(root).exists())
    return false;
  const auto styleName = qEnvironmentVariable("DESKFLOW_TEST_STYLE");
  if (!styleName.isEmpty()) {
    auto *style = QStyleFactory::create(styleName);
    if (!style)
      return false;
    app.setStyle(style);
  }
  QJsonArray loaded;
  if (family.startsWith("Microsoft YaHei")) {
    for (const auto *filename : {"msyh.ttc", "msyhbd.ttc"}) {
      const auto id = QFontDatabase::addApplicationFont(qEnvironmentVariable("WINDIR") + "/Fonts/" + filename);
      loaded.append(QJsonObject{{"file", QString(filename)}, {"id", id}});
      if (id < 0)
        return false;
    }
  }
  if (!QFontDatabase::families().contains(family))
    return false;
  bool ok = false;
  const auto points = qEnvironmentVariable("DESKFLOW_TEST_FONT_POINTS").toDouble(&ok);
  if (!ok || points <= 0 || points > 72)
    return false;
  QFont font(family);
  font.setPointSizeF(points);
  app.setFont(font);
  const QFontInfo info(app.font());
  const QFontMetricsF metrics(app.font());
  const auto raw = QRawFont::fromFont(app.font());
  const auto glyphs = raw.glyphIndexesForString(QStringLiteral("A") + QChar(0x4e2d));
  const bool valid = raw.isValid() && info.family() == family && glyphs.size() == 2 && glyphs[0] != 0 && glyphs[1] != 0;
  const QJsonObject evidence{{"requestedFamily", family},       {"resolvedFamily", info.family()},
                             {"requestedStyle", styleName},     {"resolvedStyle", app.style()->objectName()},
                             {"pointSize", info.pointSizeF()},  {"devicePixelRatio", app.devicePixelRatio()},
                             {"fontHeight", metrics.height()},  {"latinAndCjkGlyphsPresent", valid},
                             {"loadedApplicationFonts", loaded}};
  QFile output(root + "/font-environment.json");
  if (!output.open(QIODevice::WriteOnly))
    return false;
  const auto bytes = QJsonDocument(evidence).toJson(QJsonDocument::Indented);
  return valid && output.write(bytes) == bytes.size();
#else
  Q_UNUSED(app);
  return true;
#endif
}
