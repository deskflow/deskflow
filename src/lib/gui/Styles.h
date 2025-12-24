/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

namespace deskflow::gui {

const auto kColorError = "#ec4c47";

const auto kStyleErrorActiveLabel = //
    QStringLiteral(
        "padding: 3px 5px; border-radius: 3px; "
        "background-color: %1; color: white"
    )
        .arg(kColorError);

const auto kStyleErrorInactiveLabel = //
    QStringLiteral(
        "padding: 3px 5px; border-radius: 3px;"
        "background-color: none"
    );

} // namespace deskflow::gui
