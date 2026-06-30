/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ntddk.h>

extern UCHAR const g_DeskflowVhidMouseReportDescriptor[];
extern ULONG const g_DeskflowVhidMouseReportDescriptorLength;

extern UCHAR const g_DeskflowVhidKeyboardReportDescriptor[];
extern ULONG const g_DeskflowVhidKeyboardReportDescriptorLength;
