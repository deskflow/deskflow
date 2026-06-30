/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <vhf.h>

#include "public/deskflow_vhid_ioctl.h"

typedef struct _DEVICE_CONTEXT
{
  VHFHANDLE mouseVhf;
  VHFHANDLE keyboardVhf;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

EVT_WDF_DRIVER_DEVICE_ADD DeskflowVhidEvtDeviceAdd;
EVT_WDF_DEVICE_PREPARE_HARDWARE DeskflowVhidEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE DeskflowVhidEvtDeviceReleaseHardware;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DeskflowVhidEvtIoDeviceControl;
