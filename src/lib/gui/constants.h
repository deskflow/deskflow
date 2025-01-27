/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui {

// important: this is used for settings paths on some platforms,
// and must not be a url. qt automatically converts this to reverse domain
// notation (rdn), e.g. org.deskflow
const auto kOrgDomain = QStringLiteral("deskflow.org");

const auto kLinkDownload = R"(<a href="%1" style="color: %2">Download now</a>)";

const auto kUrlSourceQuery = "source=gui";
const auto kUrlApp = QStringLiteral("https://deskflow.org");
const auto kUrlHelp = QString("%1/help?%2").arg(kUrlApp, kUrlSourceQuery);
const auto kUrlDownload = QString("%1/download?%2").arg(kUrlApp, kUrlSourceQuery);

#if defined(Q_OS_LINUX)
const auto kUrlGnomeTrayFix = "https://extensions.gnome.org/extension/615/appindicator-support/";
#endif

} // namespace deskflow::gui
