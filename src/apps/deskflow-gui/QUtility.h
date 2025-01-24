/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "ProcessorArch.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QVariant>

void setIndexFromItemData(QComboBox *comboBox, const QVariant &itemData);
QString hash(const QString &string);
qProcessorArch getProcessorArch();
