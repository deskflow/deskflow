/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "coordination/CoordinationProtocol.h"
#include "deskflow/KeyTypes.h"

namespace deskflow::coordination {

#if defined(__APPLE__)
bool mapRelayKeyFromCgEvent(
    void *cgEvent, Message::KeyPhase &phase, KeyID &id, KeyModifierMask &mask, KeyButton &button
);
#elif defined(_WIN32)
bool mapRelayKeyFromHook(
    int vkCode, int scanCode, bool isExtended, bool keyUp, bool isRepeat, KeyID &id, KeyModifierMask &mask,
    KeyButton &button, Message::KeyPhase &phase
);
#endif

} // namespace deskflow::coordination
