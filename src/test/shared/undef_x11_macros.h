/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

/**
 * This header fixes conflicts between X11 and Google Test.
 *
 * It should be included after headers of code under test (that use X11)
 * and before including Google Test headers.
 */

#undef None // NOSONAR
#undef Bool // NOSONAR
