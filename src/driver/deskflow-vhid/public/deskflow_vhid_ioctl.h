/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 *
 * Shared IOCTL definitions for deskflow-vhid.sys and deskflow-vhid-bridge.exe.
 */

#pragma once

#ifdef _KERNEL_MODE
#include <guiddef.h>
#else
#include <windows.h>
#include <winioctl.h>
#include <initguid.h>
#endif

// {A7F3C2E1-8B4D-4F6A-9E2C-1D0B5A8F3E7C}
DEFINE_GUID(
    GUID_DEVINTERFACE_DESKFLOW_VHID,
    0xa7f3c2e1,
    0x8b4d,
    0x4f6a,
    0x9e,
    0x2c,
    0x1d,
    0x0b,
    0x5a,
    0x8f,
    0x3e,
    0x7c
);

#define DESKFLOW_VHID_DEVICE_TYPE 0x8000

#define IOCTL_DESKFLOW_VHID_MOUSE_REPORT \
  CTL_CODE(DESKFLOW_VHID_DEVICE_TYPE, 0x800, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_DESKFLOW_VHID_KEYBOARD_REPORT \
  CTL_CODE(DESKFLOW_VHID_DEVICE_TYPE, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define DESKFLOW_VHID_MOUSE_REPORT_SIZE 4
#define DESKFLOW_VHID_KEYBOARD_REPORT_SIZE 8

#pragma pack(push, 1)

typedef struct _DESKFLOW_VHID_MOUSE_REPORT
{
  unsigned char buttons;
  char dx;
  char dy;
  char wheel;
} DESKFLOW_VHID_MOUSE_REPORT;

typedef struct _DESKFLOW_VHID_KEYBOARD_REPORT
{
  unsigned char modifiers;
  unsigned char reserved;
  unsigned char keys[6];
} DESKFLOW_VHID_KEYBOARD_REPORT;

#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(DESKFLOW_VHID_MOUSE_REPORT) == DESKFLOW_VHID_MOUSE_REPORT_SIZE);
static_assert(sizeof(DESKFLOW_VHID_KEYBOARD_REPORT) == DESKFLOW_VHID_KEYBOARD_REPORT_SIZE);
#endif
