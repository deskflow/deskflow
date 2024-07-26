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

const auto kColorPrimary = "#ff7c00";
const auto kColorWhite = "#ffffff";
const auto kColorBlue = "#4285f4";

const auto kPrimaryLink = QString("color: %1").arg(kColorPrimary);
const auto kSecondaryLink = QString("color: %1").arg(kColorBlue);
const auto kWhiteLink = QString("color: %1").arg(kColorWhite);
const auto kBuyLink = R"(<a href="%1" style="%2">Buy now</a>)";
const auto kRenewLink = R"(<a href="%1" style="%2">Renew now</a>)";

const auto kUrlSourceQuery = "source=gui";
const auto kWebsiteUrl = "https://symless.com";
const auto kProductUrl = QString("%1/synergy").arg(kWebsiteUrl);
const auto kPurchaseUrl =
    QString("%1/purchase?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const auto kContactUrl =
    QString("%1/contact?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const auto kHelpUrl =
    QString("%1/help?%2").arg(kProductUrl).arg(kUrlSourceQuery);
const auto kDownloadUrl =
    QString("%1/download?%2").arg(kProductUrl).arg(kUrlSourceQuery);
