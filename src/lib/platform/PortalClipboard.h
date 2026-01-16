/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/IClipboard.h"
#include <QObject>
#include <QMap>
#include <QStringList>
#include <memory>
#include <mutex>

namespace deskflow {

class PortalClipboardProxy;

/**
 * @brief IClipboard implementation using XDG Desktop Portal
 * 
 * This class bridges the Deskflow IClipboard interface with the 
 * XDG Desktop Portal Clipboard via PortalClipboardProxy.
 */
class PortalClipboard : public QObject, public IClipboard
{
    Q_OBJECT

public:
    explicit PortalClipboard(PortalClipboardProxy *proxy, QObject *parent = nullptr);
    ~PortalClipboard() override;

    // IClipboard overrides
    bool empty() override;
    void add(Format format, const std::string &data) override;
    bool open(Time time) const override;
    void close() const override;
    Time getTime() const override;
    bool has(Format format) const override;
    std::string get(Format format) const override;

private Q_SLOTS:
    void onSelectionOwnerChanged(const QStringList &mimeTypes, bool sessionIsOwner);
    void onSelectionTransferRequested(const QString &mimeType, quint32 serial);

private:
    QString formatToMimeType(Format format) const;
    Format mimeTypeToFormat(const QString &mimeType) const;
    void updateCacheFromPortal() const;

    PortalClipboardProxy *m_proxy;
    mutable bool m_isOpen = false;
    mutable Time m_time = 0;
    
    // Cache for clipboard data
    mutable std::mutex m_cacheMutex;
    mutable QMap<Format, std::string> m_cache;
    mutable QStringList m_portalMimeTypes;
    mutable bool m_portalIsOwner = false;
};

} // namespace deskflow
