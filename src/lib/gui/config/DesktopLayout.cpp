// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "DesktopLayout.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <cmath>

namespace deskflow::gui {
QString DesktopLayout::displayNameError(const QString &name)
{
  if (name.isEmpty())
    return {};
  if (name.trimmed().isEmpty() || name.toUcs4().size() > 80)
    return QObject::tr("Use a display name of 1–80 characters, or leave it empty to show the protocol name.");
  for (const auto code : name.toUcs4()) {
    const auto type = QChar::category(code);
    if (type == QChar::Other_Control || type == QChar::Other_Format || type == QChar::Other_Surrogate ||
        type == QChar::Separator_Line || type == QChar::Separator_Paragraph || code == 0xfffd)
      return QObject::tr("Display names cannot contain control characters, line breaks or hidden direction marks.");
  }
  return {};
}
QString DesktopLayout::draftError(const std::vector<experience::Desktop> &desktops)
{
  for (const auto &d : desktops) {
    const auto error = displayNameError(QString::fromStdString(d.displayName));
    if (!error.isEmpty())
      return error;
  }
  const auto validation = experience::validateLayout(desktops);
  return validation.valid() ? QString() : errorText(validation.errors.front());
}
QString DesktopLayout::errorText(const std::string &error)
{
  const auto text = QString::fromStdString(error);
  if (text.startsWith("desktops overlap: "))
    return QObject::tr("desktops overlap: %1").arg(text.mid(18));
  if (text == "layout requires 1..16 computers")
    return QObject::tr("layout requires 1..16 computers");
  if (text == "invalid geometry or duplicate/unsupported computer name")
    return QObject::tr("invalid geometry or duplicate/unsupported computer name");
  if (text == "shared edge is too small for the core to represent")
    return QObject::tr("shared edge is too small for the core to represent");
  if (text == "every computer must share a usable edge with the connected layout")
    return QObject::tr("every computer must share a usable edge with the connected layout");
  return text;
}
DesktopLayout DesktopLayout::decode(const QString &json)
{
  DesktopLayout result;
  const auto invalid = [&result] {
    result.desktops.clear();
    result.error = QObject::tr("The saved layout cannot be read. Restore the grid to replace it explicitly.");
    return result;
  };
  if (json.size() > 32768)
    return invalid();
  QJsonParseError parseError;
  const auto document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject())
    return invalid();
  const auto root = document.object();
  if (root.value("version").toInt(-1) != 1 || !root.value("desktops").isArray())
    return invalid();
  const auto desktops = root.value("desktops").toArray();
  if (desktops.isEmpty() || desktops.size() > 16)
    return invalid();
  for (const auto &value : desktops) {
    if (!value.isObject())
      return invalid();
    const auto d = value.toObject();
    if (!d.value("name").isString() || (d.contains("displayName") && !d.value("displayName").isString()))
      return invalid();
    for (const auto *key : {"x", "y", "width", "height"})
      if (!d.value(key).isDouble() || !std::isfinite(d.value(key).toDouble()))
        return invalid();
    if (std::abs(d.value("x").toDouble()) > 10000000 || std::abs(d.value("y").toDouble()) > 10000000 ||
        d.value("width").toDouble() < 1 || d.value("height").toDouble() < 1 || d.value("width").toDouble() > 1000000 ||
        d.value("height").toDouble() > 1000000)
      return invalid();
    result.desktops.push_back(
        {d.value("name").toString().toStdString(), d.value("x").toDouble(), d.value("y").toDouble(),
         d.value("width").toDouble(), d.value("height").toDouble(), d.value("displayName").toString().toStdString()}
    );
  }
  return result;
}

QString DesktopLayout::encode(const std::vector<experience::Desktop> &desktops)
{
  QJsonArray array;
  for (const auto &d : desktops) {
    QJsonObject item{
        {"name", QString::fromStdString(d.name)}, {"x", d.x}, {"y", d.y}, {"width", d.width}, {"height", d.height}
    };
    if (!d.displayName.empty())
      item.insert("displayName", QString::fromStdString(d.displayName));
    array.append(item);
  }
  return QString::fromUtf8(
      QJsonDocument(QJsonObject{{"version", 1}, {"desktops", array}}).toJson(QJsonDocument::Compact)
  );
}

QString DesktopLayout::validate(const std::vector<experience::Desktop> &desktops, const QStringList &names)
{
  const auto error = draftError(desktops);
  if (!error.isEmpty())
    return error;
  QSet<QString> actual;
  for (const auto &d : desktops)
    actual.insert(QString::fromStdString(d.name));
  if (actual != QSet<QString>(names.begin(), names.end()) || actual.size() != names.size())
    return QObject::tr("The layout and computer list differ. Restore the grid or correct the computer list first.");
  return {};
}
} // namespace deskflow::gui
