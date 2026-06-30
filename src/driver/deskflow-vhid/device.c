/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "device.h"

#include <initguid.h>
#include "public/deskflow_vhid_ioctl.h"
#include "hid_descriptors.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DeskflowVhidEvtDeviceAdd)
#pragma alloc_text(PAGE, DeskflowVhidEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, DeskflowVhidEvtDeviceReleaseHardware)
#endif

static NTSTATUS DeskflowVhidCreateVhfDevice(
    WDFDEVICE device, PDEVICE_CONTEXT context, VHFHANDLE *outHandle, UCHAR const *descriptor, ULONG descriptorLength
)
{
  PAGED_CODE();

  VHF_CONFIG vhfConfig;
  VHF_CONFIG_INIT(
      &vhfConfig, WdfDeviceWdmGetDeviceObject(device), static_cast<USHORT>(descriptorLength),
      const_cast<PUCHAR>(descriptor)
  );

  vhfConfig.VendorID = 0x046D;  // Logitech VID placeholder for labelling
  vhfConfig.ProductID = 0xFFFF; // Deskflow virtual product
  vhfConfig.VersionNumber = 0x0001;
  vhfConfig.VhfClientContext = context;

  NTSTATUS status = VhfCreate(&vhfConfig, outHandle);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  return VhfStart(*outHandle);
}

static NTSTATUS DeskflowVhidSubmitReport(VHFHANDLE handle, PVOID report, USHORT reportLength)
{
  HID_XFER_PACKET packet{};
  packet.reportBuffer = static_cast<PUCHAR>(report);
  packet.reportBufferLen = reportLength;
  packet.reportId = 0;
  return VhfReadReportSubmit(handle, &packet);
}

NTSTATUS DeskflowVhidEvtDeviceAdd(WDFDRIVER driver, PWDFDEVICE_INIT deviceInit)
{
  UNREFERENCED_PARAMETER(driver);
  PAGED_CODE();

  WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);
  WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);

  WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
  WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);
  pnpCallbacks.EvtDevicePrepareHardware = DeskflowVhidEvtDevicePrepareHardware;
  pnpCallbacks.EvtDeviceReleaseHardware = DeskflowVhidEvtDeviceReleaseHardware;
  WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &pnpCallbacks);

  WDF_OBJECT_ATTRIBUTES attributes;
  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

  WDFDEVICE device;
  NTSTATUS status = WdfDeviceCreate(&deviceInit, &attributes, &device);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_DESKFLOW_VHID, nullptr);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  WDF_IO_QUEUE_CONFIG queueConfig;
  WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);
  queueConfig.EvtIoDeviceControl = DeskflowVhidEvtIoDeviceControl;

  WDFQUEUE queue;
  status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
  if (!NT_SUCCESS(status)) {
    return status;
  }

  return STATUS_SUCCESS;
}

NTSTATUS DeskflowVhidEvtDevicePrepareHardware(WDFDEVICE device, WDFCMRESLIST, WDFCMRESLIST)
{
  PAGED_CODE();

  PDEVICE_CONTEXT context = DeviceGetContext(device);

  NTSTATUS status = DeskflowVhidCreateVhfDevice(
      device, context, &context->mouseVhf, g_DeskflowVhidMouseReportDescriptor,
      g_DeskflowVhidMouseReportDescriptorLength
  );
  if (!NT_SUCCESS(status)) {
    return status;
  }

  status = DeskflowVhidCreateVhfDevice(
      device, context, &context->keyboardVhf, g_DeskflowVhidKeyboardReportDescriptor,
      g_DeskflowVhidKeyboardReportDescriptorLength
  );
  if (!NT_SUCCESS(status)) {
    if (context->mouseVhf != nullptr) {
      VhfDelete(context->mouseVhf, TRUE);
      context->mouseVhf = nullptr;
    }
    return status;
  }

  return STATUS_SUCCESS;
}

NTSTATUS DeskflowVhidEvtDeviceReleaseHardware(WDFDEVICE device, WDFCMRESLIST)
{
  PAGED_CODE();

  PDEVICE_CONTEXT context = DeviceGetContext(device);
  if (context->keyboardVhf != nullptr) {
    VhfDelete(context->keyboardVhf, TRUE);
    context->keyboardVhf = nullptr;
  }
  if (context->mouseVhf != nullptr) {
    VhfDelete(context->mouseVhf, TRUE);
    context->mouseVhf = nullptr;
  }
  return STATUS_SUCCESS;
}

VOID DeskflowVhidEvtIoDeviceControl(
    WDFQUEUE queue, WDFREQUEST request, size_t outputBufferLength, size_t inputBufferLength, ULONG ioControlCode
)
{
  UNREFERENCED_PARAMETER(outputBufferLength);

  WDFDEVICE device = WdfIoQueueGetDevice(queue);
  PDEVICE_CONTEXT context = DeviceGetContext(device);

  NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
  size_t bytesReturned = 0;

  switch (ioControlCode) {
  case IOCTL_DESKFLOW_VHID_MOUSE_REPORT: {
    if (inputBufferLength < sizeof(DESKFLOW_VHID_MOUSE_REPORT) || context->mouseVhf == nullptr) {
      status = STATUS_INVALID_PARAMETER;
      break;
    }
    DESKFLOW_VHID_MOUSE_REPORT *report = nullptr;
    status = WdfRequestRetrieveInputBuffer(request, sizeof(*report), reinterpret_cast<PVOID *>(&report), nullptr);
    if (!NT_SUCCESS(status)) {
      break;
    }
    status = DeskflowVhidSubmitReport(context->mouseVhf, report, sizeof(*report));
    break;
  }
  case IOCTL_DESKFLOW_VHID_KEYBOARD_REPORT: {
    if (inputBufferLength < sizeof(DESKFLOW_VHID_KEYBOARD_REPORT) || context->keyboardVhf == nullptr) {
      status = STATUS_INVALID_PARAMETER;
      break;
    }
    DESKFLOW_VHID_KEYBOARD_REPORT *report = nullptr;
    status = WdfRequestRetrieveInputBuffer(request, sizeof(*report), reinterpret_cast<PVOID *>(&report), nullptr);
    if (!NT_SUCCESS(status)) {
      break;
    }
    status = DeskflowVhidSubmitReport(context->keyboardVhf, report, sizeof(*report));
    break;
  }
  default:
    status = STATUS_INVALID_DEVICE_REQUEST;
    break;
  }

  WdfRequestCompleteWithInformation(request, status, bytesReturned);
}
