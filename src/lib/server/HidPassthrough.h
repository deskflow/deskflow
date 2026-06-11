/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Event.h"
#include "base/EventTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class IEventQueue;

namespace deskflow::server {

//! A device the user wants passed through, by USB identity.
/*!
\c pid == 0 means "any product from this vendor".
*/
struct HidDeviceSelector
{
  uint16_t vid = 0;
  uint16_t pid = 0;
};

//! Parse the `server/hidPassthroughDevices` setting.
/*!
Comma-separated `VID:PID` pairs in hex (`046D:B042`); `*` as the PID
matches any product from that vendor. Malformed entries are skipped.
*/
std::vector<HidDeviceSelector> parseHidDeviceSelectors(const std::string &setting);

//! Lowercase hex encoding for raw report frames (consumer protocol).
std::string hidBytesToHex(const uint8_t *bytes, size_t len);

//! Identity of a grabbed device, as advertised to the focused client.
struct HidDeviceDescriptor
{
  uint16_t deviceId = 0;
  uint16_t vid = 0;
  uint16_t pid = 0;
  uint32_t usagePage = 0;
  uint32_t usage = 0;
  std::string name;
};

//! Platform half of HID pass-through: discover + seize + read raw reports.
/*!
Implementations seize ONLY vendor-defined HID collections (usage page
>= 0xFF00), never the standard pointer interface -- the pointer already
travels over Deskflow's normal path (docs/hid-passthrough.md). The seize
must be held by a mechanism the OS releases on process exit, so a crash
always returns the device to the host.
*/
class IHidGrabber
{
public:
  using AttachCallback = std::function<void(const HidDeviceDescriptor &)>;
  using FrameCallback = std::function<void(uint16_t deviceId, std::string bytes)>;
  using DetachCallback = std::function<void(uint16_t deviceId)>;

  virtual ~IHidGrabber() = default;

  //! Begin discovery; callbacks fire on the grabber's own thread.
  virtual bool start(
      std::vector<HidDeviceSelector> selectors, AttachCallback onAttach, FrameCallback onFrame, DetachCallback onDetach
  ) = 0;

  //! Seize (true) or release (false) the matched vendor interfaces.
  virtual void setSeized(bool seized) = 0;

  virtual void stop() = 0;
};

//! Platform factory (macOS implementation; stub elsewhere for now).
std::unique_ptr<IHidGrabber> createHidGrabber();

//! Event data for EventTypes::ServerHidPassthroughEvent.
class HidPassthroughEventData : public EventData
{
public:
  enum class Kind
  {
    Attach, //!< payload is a consumer-protocol "connect" JSON line
    Frame,  //!< payload is raw report bytes (binary)
    Detach  //!< payload is a consumer-protocol "disconnect" JSON line
  };

  HidPassthroughEventData(Kind kind, std::string payload, uint16_t deviceId = 0)
      : m_kind(kind),
        m_payload(std::move(payload)),
        m_deviceId(deviceId)
  {
    // do nothing
  }
  ~HidPassthroughEventData() override = default;

  Kind kind() const
  {
    return m_kind;
  }
  const std::string &payload() const
  {
    return m_payload;
  }
  uint16_t deviceId() const
  {
    return m_deviceId;
  }

private:
  Kind m_kind;
  std::string m_payload;
  uint16_t m_deviceId;
};

//! Server-side HID pass-through orchestrator.
/*!
Owns the platform grabber and translates its callbacks into events on the
server's queue (\c EventTypes::ServerHidPassthroughEvent), where \c Server
relays them to whichever client currently hosts the virtual device. Focus
drives the seize: remote focus seizes the vendor interface (the host's own
consumer sees the device vanish), local focus releases it.
*/
class HidPassthrough
{
public:
  HidPassthrough(IEventQueue *events, void *eventTarget);
  HidPassthrough(const HidPassthrough &) = delete;
  HidPassthrough &operator=(const HidPassthrough &) = delete;
  ~HidPassthrough();

  //! Read settings and begin discovery; false when disabled/unconfigured.
  bool start();
  void stop();

  //! Focus moved: seize while a remote screen is focused, else release.
  void setFocusRemote(bool remote);

private:
  std::string connectLineFor(const HidDeviceDescriptor &descriptor) const;

  IEventQueue *m_events;
  void *m_eventTarget;
  std::unique_ptr<IHidGrabber> m_grabber;
  bool m_started = false;
};

} // namespace deskflow::server
