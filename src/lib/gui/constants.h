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

const auto kAppName = "Synergy";

// TODO: change to `com.symless`. we'll need to gracefully import old settings,
// since qt uses this on some platforms when saving settings.
const auto kAppDomain = "https://symless.com";

#ifdef SYNERGY_PRODUCT_NAME
const QString kProductName = SYNERGY_PRODUCT_NAME;
#else
const QString kProductName;
#endif

#ifdef SYNERGY_ENABLE_LICENSING
const bool kLicensingEnabled = true;
#else
const bool kLicensingEnabled = false;
#endif // SYNERGY_ENABLE_LICENSING

const auto kLinkBuy = R"(<a href="%1" style="color: %2">Buy now</a>)";
const auto kLinkRenew = R"(<a href="%1" style="color: %2">Renew now</a>)";
const auto kLinkDownload = R"(<a href="%1" style="color: %2">Download now</a>)";

const auto kUrlSourceQuery = "source=gui";
const auto kUrlWebsite = "https://symless.com";
const auto kUrlContribute = "https://github.com/symless/synergy-core";
const auto kUrlGnomeTrayFix =
    "https://extensions.gnome.org/extension/2890/tray-icons-reloaded/";
const auto kUrlProduct = QString("%1/synergy").arg(kUrlWebsite);
const auto kUrlPurchase =
    QString("%1/purchase?%2").arg(kUrlProduct, kUrlSourceQuery);
const auto kUrlContact =
    QString("%1/contact?%2").arg(kUrlProduct, kUrlSourceQuery);
const auto kUrlHelp = QString("%1/help?%2").arg(kUrlProduct, kUrlSourceQuery);
const auto kUrlDownload =
    QString("%1/download?%2").arg(kUrlProduct, kUrlSourceQuery);
