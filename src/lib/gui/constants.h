/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Symless Ltd.
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

const char *const kPrimaryLink = "color: #ff7c00";
const char *const kSecondaryLink = "color: #4285F4";
const char *const kWhiteLink = "color: #ffffff";
const char *const kBuyLink = R"(<a href="%1" style="%2">Buy now</a>)";
const char *const kRenewLink = R"(<a href="%1" style="%2">Renew now</a>)";

const QString kUrlSourceQuery = "source=gui";
const QString kWebsiteUrl = "https://symless.com";
const QString kProductUrl = QString("%1/synergy").arg(kWebsiteUrl);
const QString kPurchaseUrl =
    QString("%1/purchase?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const QString kContactUrl =
    QString("%1/contact?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const QString kHelpUrl =
    QString("%1/help?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const QString kDownloadUrl =
    QString("%1/download?%2").arg(kProductUrl).arg(kUrlSourceQuery);
