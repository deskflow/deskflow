/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/ClipboardTypes.h"
#include "deskflow/IClipboard.h"
#include "platform/EiClipboard.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct zwlr_data_control_manager_v1;
struct zwlr_data_control_device_v1;
struct zwlr_data_control_source_v1;
struct zwlr_data_control_offer_v1;

namespace deskflow {

class WlrClipboard
{
public:
  using GrabbedCallback = std::function<void()>;

  explicit WlrClipboard(GrabbedCallback onGrabbed, size_t maxClipboardBytes = 4 * 1024 * 1024);
  ~WlrClipboard();

  WlrClipboard(const WlrClipboard &) = delete;
  WlrClipboard &operator=(const WlrClipboard &) = delete;

  //! True if Wayland data control protocol was successfully connected and bound
  bool isAvailable() const
  {
    return m_available.load();
  }

  //! Set selection to Wayland compositor from provided IClipboard
  bool setSelection(const IClipboard *clipboard);

  //! Retrieve current selection from internal cache
  bool getClipboard(IClipboard *target) const;

  //! Direct pointer to internal cache
  EiClipboard *getCache() const
  {
    return m_cache;
  }

private:
  void threadFunc();
  void initWayland();
  void cleanupWayland();
  void processOutgoingSelection();
  void handleSelectionOffer(struct zwlr_data_control_offer_v1 *offer, const std::vector<std::string> &mimes);

  static void registryGlobal(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
  static void registryGlobalRemove(void *data, struct wl_registry *registry, uint32_t id);

  static void deviceDataOffer(void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);
  static void deviceSelection(void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);
  static void deviceFinished(void *data, struct zwlr_data_control_device_v1 *device);
  static void devicePrimarySelection(void *data, struct zwlr_data_control_device_v1 *device, struct zwlr_data_control_offer_v1 *offer);

  static void offerOffer(void *data, struct zwlr_data_control_offer_v1 *offer, const char *mime_type);

  static void sourceSend(void *data, struct zwlr_data_control_source_v1 *source, const char *mime_type, int32_t fd);
  static void sourceCancelled(void *data, struct zwlr_data_control_source_v1 *source);

private:
  GrabbedCallback m_onGrabbed;
  size_t m_maxClipboardBytes;
  mutable std::mutex m_mutex;

  EiClipboard *m_cache = nullptr;

  std::atomic<bool> m_available{false};
  std::atomic<bool> m_stopping{false};
  std::atomic<bool> m_initialized{false};
  std::condition_variable m_initCv;
  std::mutex m_initMutex;

  // Pipe for waking the background wayland poll loop
  int m_wakeupPipe[2]{-1, -1};
  std::thread m_thread;

  // Wayland state owned exclusively by the background thread
  struct wl_display *m_display = nullptr;
  struct wl_registry *m_registry = nullptr;
  struct wl_seat *m_seat = nullptr;
  struct zwlr_data_control_manager_v1 *m_manager = nullptr;
  struct zwlr_data_control_device_v1 *m_device = nullptr;
  struct zwlr_data_control_source_v1 *m_activeSource = nullptr;

  // Selection ownership tracking (loop prevention)
  bool m_isOwnSelection = false;
  int m_ownSelectionEchosRemaining = 0;

  // Staged data for outgoing selection transfers
  struct OutgoingData
  {
    bool valid = false;
    std::string text;
    std::string html;
    std::string bitmap;
  };

  OutgoingData m_pendingOutgoing;
  OutgoingData m_activeOutgoing;
  bool m_hasPendingOutgoing = false;

  // Current offer MIME types tracking
  struct ActiveOfferInfo
  {
    struct zwlr_data_control_offer_v1 *offer = nullptr;
    std::vector<std::string> mimes;
  };
  std::vector<ActiveOfferInfo> m_offers;
};

} // namespace deskflow
