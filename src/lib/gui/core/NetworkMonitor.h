/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QHostAddress>
#include <QList>
#include <QObject>

class QTimer;
namespace deskflow::gui {

/**
 * @brief Monitor network activity changes and provide IP address updates
 *
 * The NetworkMonitor class monitors IP address changes
 * It periodically checks network status and emits signals when changes are detected.
 */
class NetworkMonitor : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief Construct a new NetworkMonitor object
   * @param parent Parent QObject
   */
  explicit NetworkMonitor(QObject *parent = nullptr);

  /**
   * @brief Destroy the NetworkMonitor object
   */
  ~NetworkMonitor() override = default;

  /**
   * @brief Start network monitoring
   * @param intervalMs Check interval in milliseconds, default 3000ms (3 seconds)
   */
  void startMonitoring(int intervalMs = 3000);

  /**
   * @brief Stop network monitoring
   */
  void stopMonitoring();

  /**
   * @brief Get list of all available IPv4 addresses (excluding local and link-local addresses)
   * @return IPv4 address list
   */
  QList<QHostAddress> getAvailableIPv4Addresses() const;

  /**
   * @brief Get recommended IP address (192.168.x.x preferred)
   * @return Recommended IP address, returns null if none available
   */
  QHostAddress getSuggestedIPv4Address() const;

Q_SIGNALS:
  /**
   * @brief Emitted when IP addresses change
   * @param addresses New IP address list
   */
  void ipAddressesChanged(const QList<QHostAddress> &addresses);

private:
  void setIpAddresses(const QList<QHostAddress> &newAddresses);

  /**
   * @brief Check if a network interface is virtual
   * @param interfaceName Network interface name
   * @return true if it's a virtual interface
   */
  bool isVirtualInterface(const QString &interfaceName) const;

  /**
   * @brief Update current network status
   */
  void updateNetworkState();

  QTimer *m_checkTimer;                ///< Timer for periodic network checks
  QList<QHostAddress> m_lastAddresses; ///< Last known IP addresses
  bool m_isMonitoring;                 ///< Flag indicating if monitoring is active
};

} // namespace deskflow::gui
