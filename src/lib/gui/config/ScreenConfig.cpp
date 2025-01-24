/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ScreenConfig.h"

const char *ScreenConfig::m_ModifierNames[] = {"shift", "ctrl", "alt", "meta", "super", "none"};

const char *ScreenConfig::m_FixNames[] = {
    "halfDuplexCapsLock", "halfDuplexNumLock", "halfDuplexScrollLock", "xtestIsXineramaUnaware"
};

const char *ScreenConfig::m_SwitchCornerNames[] = {"top-left", "top-right", "bottom-left", "bottom-right"};
