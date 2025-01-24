/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "QUtility.h"

#include "ProcessorArch.h"

#if defined(Q_OS_LINUX)
#include <QFile>
#include <QProcess>
#endif

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

void setIndexFromItemData(QComboBox *comboBox, const QVariant &itemData)
{
  for (int i = 0; i < comboBox->count(); ++i) {
    if (comboBox->itemData(i) == itemData) {
      comboBox->setCurrentIndex(i);
      return;
    }
  }
}

QString hash(const QString &string)
{
  QByteArray data = string.toUtf8();
  QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
  return hash.toHex();
}

qProcessorArch getProcessorArch()
{
#if defined(Q_OS_WIN)
  SYSTEM_INFO systemInfo;
  GetNativeSystemInfo(&systemInfo);

  switch (systemInfo.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_INTEL:
    return kProcessorArchWin32;
  case PROCESSOR_ARCHITECTURE_IA64:
    return kProcessorArchWin64;
  case PROCESSOR_ARCHITECTURE_AMD64:
    return kProcessorArchWin64;
  default:
    return kProcessorArchUnknown;
  }
#endif

#if defined(Q_OS_LINUX)
#ifdef __i386__
  return kProcessorArchLinux32;
#else
  return kProcessorArchLinux64;
#endif
#endif

  return kProcessorArchUnknown;
}
