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
const auto kUrlGnomeTrayFix = "https://extensions.gnome.org/extension/2890/tray-icons-reloaded/";
#endif

} // namespace deskflow::gui
