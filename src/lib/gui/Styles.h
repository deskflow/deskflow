/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui {

const auto kColorWhite = "#ffffff";
const auto kColorSecondary = "#4285f4";
const auto kColorError = "#ec4c47";
const auto kColorLightGrey = "#666666";

const auto kStyleLink = //
    QStringLiteral("color: %1").arg(kColorSecondary);

const auto kStyleLineEditErrorBorder =
    QStringLiteral("border: 1px solid %1; border-radius: 2px; padding: 2px;").arg(kColorError);

const auto kStyleErrorActiveLabel = //
    QStringLiteral(
        "padding: 3px 5px; border-radius: 3px; "
        "background-color: %1; color: %2"
    )
        .arg(kColorError, kColorWhite);

const auto kStyleErrorInactiveLabel = //
    QStringLiteral(
        "padding: 3px 5px; border-radius: 3px;"
        "background-color: none"
    );

const auto kStyleFlatButton = QStringLiteral("QAbstractButton{background-color: none; border: none;}");

const auto kStyleFlatButtonHoverable = QStringLiteral(
                                           "%1\n"
                                           "QAbstractButton:hover{border: 1px solid palette(highlight);"
                                           " border-radius: 6px}"
)
                                           .arg(kStyleFlatButton);

} // namespace deskflow::gui
