/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

using namespace Qt::StringLiterals;

namespace deskflow::gui {

const auto kColorWhite = "#ffffff";
const auto kColorSecondary = "#4285f4";
const auto kColorError = "#ec4c47";
const auto kColorLightGrey = "#666666";

const auto kStyleLink = u"color: %1"_s.arg(kColorSecondary);

const auto kStyleLineEditErrorBorder = u"border: 1px solid %1; border-radius: 2px; padding: 2px;"_s.arg(kColorError);

const auto kStyleErrorActiveLabel = //
    u"padding: 3px 5px; border-radius: 3px; "
    "background-color: %1; color: %2"_s.arg(kColorError, kColorWhite);

const auto kStyleErrorInactiveLabel = u"padding: 3px 5px; border-radius: 3px; background-color: none"_s;

} // namespace deskflow::gui
