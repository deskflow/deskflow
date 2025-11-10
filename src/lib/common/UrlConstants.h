/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

using namespace Qt::StringLiterals;
// important: this is used for settings paths on some platforms,
// and must not be a url. qt automatically converts this to reverse domain
// notation (rdn), e.g. org.deskflow
const auto kOrgDomain = u"deskflow.org"_s;

const auto kUrlSourceQuery = u"source=gui"_s;
const auto kUrlApp = u"https://%1"_s.arg(kOrgDomain);
const auto kUrlHelp = u"%1/help?%2"_s.arg(kUrlApp, kUrlSourceQuery);
const auto kUrlDownload = u"%1/download?%2"_s.arg(kUrlApp, kUrlSourceQuery);

const auto kUrlUpdateCheck = u"https://api.%1/version"_s.arg(kOrgDomain);

#if defined(Q_OS_LINUX)
const auto kUrlGnomeTrayFix = u"https://extensions.gnome.org/extension/615/appindicator-support/"_s;
#endif
