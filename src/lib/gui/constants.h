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

#ifdef SYNERGY_ENABLE_LICENSING
const bool kLicensingEnabled = true;
#else
const bool kLicensingEnabled = false;
#endif // SYNERGY_ENABLE_LICENSING

const auto kColorWhite = "#ffffff";
const auto kColorPrimary = "#ff7c00";
const auto kColorSecondary = "#4285f4";
const auto kColorTertiary = "#33b2cc";

const auto kLinkStyleWhite = QString("color: %1").arg(kColorWhite);
const auto kLinkStylePrimary = QString("color: %1").arg(kColorPrimary);
const auto kLinkStyleSecondary = QString("color: %1").arg(kColorSecondary);

const auto kLinkBuy = R"(<a href="%1" style="%2">Buy now</a>)";
const auto kLinkRenew = R"(<a href="%1" style="%2">Renew now</a>)";
const auto kLinkDownload = R"(<a href="%1" style="%2">Download now</a>)";

const auto kUrlSourceQuery = "source=gui";
const auto kUrlWebsite = "https://symless.com";
const auto kUrlProduct = QString("%1/synergy").arg(kUrlWebsite);
const auto kUrlPurchase =
    QString("%1/purchase?%2").arg(kUrlProduct).arg(kUrlSourceQuery);
const auto kUrlContact =
    QString("%1/contact?%2").arg(kUrlProduct).arg(kUrlSourceQuery);
const auto kUrlHelp =
    QString("%1/help?%2").arg(kUrlProduct).arg(kUrlSourceQuery);
const auto kUrlDownload =
    QString("%1/download?%2").arg(kUrlProduct).arg(kUrlSourceQuery);

const auto kRedBorder = "border: 1px solid #EC4C47";
