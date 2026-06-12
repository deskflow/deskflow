// deskflow-vhid-bridge — replays a Deskflow host's mouse & keyboard stream
// onto this Mac through a Karabiner DriverKit virtual HID device, so the
// host can drive the machine at the login window where CGEventPost is blocked.
//
// Usage: deskflow-vhid-bridge <server_hosts> <client_screen_name>
//          [port [width height [scale_factor]]]
//          [--size=WxH] [--scale=S]
//
// <server_hosts> is a comma-separated candidate list. In auto-switch mode any
// peer can be the elected server, and only the elected server listens on the
// Deskflow port -- the bridge cycles the list until one accepts, and returns
// to cycling when that connection drops (role flip).
//
// Scope: TLS-disabled protocol only (the KVM runs inside Tailscale). It handles
// the handshake, keep-alives, screen-info query, and mouse/key data messages;
// non-input messages (clipboard, options, file transfer) are acknowledged or
// ignored. Mouse position is relayed as relative motion (the operator watches
// the screen and self-corrects), so OS pointer acceleration is irrelevant.

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <CoreGraphics/CoreGraphics.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>

#include <pqrs/karabiner/driverkit/virtual_hid_device_driver.hpp>
#include <pqrs/karabiner/driverkit/virtual_hid_device_service.hpp>

namespace {

namespace hr = pqrs::karabiner::driverkit::virtual_hid_device_driver::hid_report;
using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

void log_line(const std::string &message) {
  // stderr is captured by the LaunchDaemon log; stdout is reserved for none.
  std::string line = "[bridge] " + message + "\n";
  ::write(STDERR_FILENO, line.data(), line.size());
}

// We inject RELATIVE pointer motion; under macOS pointer acceleration a delta does
// not map 1:1 to a screen point, so the cursor drifts from the host's absolute
// position and can't reach the last strip to the far edge. Force linear (no-accel)
// so 1 delta == 1 point. The bridge runs as root at the login window, where the
// per-user mouse pref doesn't apply, so we set it directly on IOHIDSystem.
//
// The legacy IOHID acceleration API is deprecated and can BLOCK in some contexts,
// so the caller runs this on a detached thread — if it ever hangs, the bridge still
// works (just without the accel tweak), never a dead cursor.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
void disable_pointer_acceleration() {
  io_service_t service =
      IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOHIDSystem"));
  if (!service) {
    log_line("accel: IOHIDSystem not found");
    return;
  }
  io_connect_t connect = 0;
  kern_return_t kr = IOServiceOpen(service, mach_task_self(), kIOHIDParamConnectType, &connect);
  IOObjectRelease(service);
  if (kr != KERN_SUCCESS) {
    log_line("accel: IOServiceOpen failed kr=" + std::to_string(kr));
    return;
  }
  IOReturn rm = IOHIDSetAccelerationWithKey(connect, CFSTR(kIOHIDMouseAccelerationType), -1.0);
  IOReturn rp = IOHIDSetAccelerationWithKey(connect, CFSTR(kIOHIDPointerAccelerationKey), -1.0);
  IOServiceClose(connect);
  log_line("accel: disabled mouse_r=" + std::to_string(rm) + " pointer_r=" + std::to_string(rp) +
           " (0=ok)");
}
#pragma clang diagnostic pop

// ---------------------------------------------------------------------------
// Deskflow/Barrier protocol constants (verified against deskflow ProtocolTypes).
// ---------------------------------------------------------------------------
namespace proto {
constexpr char kGreeting[7] = {'B', 'a', 'r', 'r', 'i', 'e', 'r'};
constexpr int16_t kMajorVersion = 1;
constexpr int16_t kMinorVersion = 8;
constexpr uint32_t kMaxMessageBytes = 1u << 20; // reject absurd framed lengths
constexpr uint16_t kDefaultPort = 24800;

// Codes the bridge acts on. Other messages (CIAK/CROP/DSOP/CSEC/CCLP/…) carry
// no input and fall through to a no-op in dispatch(), so they need no constant.
constexpr char kQueryInfo[4] = {'Q', 'I', 'N', 'F'};
constexpr char kKeepAlive[4] = {'C', 'A', 'L', 'V'};
constexpr char kNoop[4] = {'C', 'N', 'O', 'P'};
constexpr char kClose[4] = {'C', 'B', 'Y', 'E'};
constexpr char kEnter[4] = {'C', 'I', 'N', 'N'};
constexpr char kLeave[4] = {'C', 'O', 'U', 'T'};
constexpr char kInfo[4] = {'D', 'I', 'N', 'F'};
constexpr char kMouseMove[4] = {'D', 'M', 'M', 'V'};
constexpr char kMouseRelMove[4] = {'D', 'M', 'R', 'M'};
constexpr char kMouseDown[4] = {'D', 'M', 'D', 'N'};
constexpr char kMouseUp[4] = {'D', 'M', 'U', 'P'};
constexpr char kMouseWheel[4] = {'D', 'M', 'W', 'M'};
constexpr char kKeyDown[4] = {'D', 'K', 'D', 'N'};
constexpr char kKeyDownLang[4] = {'D', 'K', 'D', 'L'}; // v1.8 key-down w/ language
constexpr char kKeyUp[4] = {'D', 'K', 'U', 'P'};
constexpr char kKeyRepeat[4] = {'D', 'K', 'R', 'P'};

// Deskflow key-modifier mask bits (KeyTypes.h).
constexpr uint32_t kMaskShift = 0x0001;
constexpr uint32_t kMaskControl = 0x0002;
constexpr uint32_t kMaskAlt = 0x0004;
constexpr uint32_t kMaskMeta = 0x0008;
constexpr uint32_t kMaskSuper = 0x0010;
} // namespace proto

// ---------------------------------------------------------------------------
// Byte (de)serialization — bounds-checked, big-endian (network order).
// ---------------------------------------------------------------------------
class ByteReader {
public:
  explicit ByteReader(const std::vector<uint8_t> &data) : data_(data) {}

  bool ok() const { return ok_; }
  size_t remaining() const { return data_.size() - pos_; }

  bool skip(size_t n) {
    if (remaining() < n) {
      ok_ = false;
      return false;
    }
    pos_ += n;
    return true;
  }
  std::optional<uint8_t> u8() {
    if (remaining() < 1) {
      ok_ = false;
      return std::nullopt;
    }
    return data_[pos_++];
  }
  std::optional<int16_t> i16() {
    if (remaining() < 2) {
      ok_ = false;
      return std::nullopt;
    }
    int16_t v = static_cast<int16_t>((data_[pos_] << 8) | data_[pos_ + 1]);
    pos_ += 2;
    return v;
  }

private:
  const std::vector<uint8_t> &data_;
  size_t pos_ = 0;
  bool ok_ = true;
};

void append_be16(std::vector<uint8_t> &out, int16_t value) {
  auto u = static_cast<uint16_t>(value);
  out.push_back(static_cast<uint8_t>(u >> 8));
  out.push_back(static_cast<uint8_t>(u & 0xff));
}
void append_be32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value >> 24));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value & 0xff));
}
void append_bytes(std::vector<uint8_t> &out, const char *bytes, size_t n) {
  out.insert(out.end(), bytes, bytes + n);
}

bool body_has_code(const std::vector<uint8_t> &body, const char (&code)[4]) {
  return body.size() >= 4 && std::memcmp(body.data(), code, 4) == 0;
}

// ---------------------------------------------------------------------------
// Framed socket I/O. Every message is a 4-byte big-endian length + payload.
// ---------------------------------------------------------------------------
class FramedSocket {
public:
  explicit FramedSocket(int fd) : fd_(fd) {}
  FramedSocket(const FramedSocket &) = delete;
  FramedSocket &operator=(const FramedSocket &) = delete;
  ~FramedSocket() { close(); }

  void close() {
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }
  int fd() const { return fd_; }

  // Reads a complete framed message. Returns std::nullopt on EOF or any error.
  std::optional<std::vector<uint8_t>> read_message() {
    std::array<uint8_t, 4> header{};
    if (!read_exact(header.data(), header.size())) return std::nullopt;
    uint32_t length = (static_cast<uint32_t>(header[0]) << 24) |
                      (static_cast<uint32_t>(header[1]) << 16) |
                      (static_cast<uint32_t>(header[2]) << 8) |
                      static_cast<uint32_t>(header[3]);
    if (length == 0 || length > proto::kMaxMessageBytes) {
      log_line("rejecting framed length " + std::to_string(length));
      return std::nullopt;
    }
    std::vector<uint8_t> body(length);
    if (!read_exact(body.data(), body.size())) return std::nullopt;
    return body;
  }

  bool write_message(const std::vector<uint8_t> &body) {
    std::vector<uint8_t> frame;
    frame.reserve(4 + body.size());
    append_be32(frame, static_cast<uint32_t>(body.size()));
    frame.insert(frame.end(), body.begin(), body.end());
    return write_all(frame.data(), frame.size());
  }

private:
  bool read_exact(uint8_t *buffer, size_t n) {
    size_t got = 0;
    while (got < n) {
      ssize_t r = ::recv(fd_, buffer + got, n - got, 0);
      if (r == 0) return false;              // peer closed
      if (r < 0) {
        if (errno == EINTR) continue;
        return false;
      }
      got += static_cast<size_t>(r);
    }
    return true;
  }
  bool write_all(const uint8_t *buffer, size_t n) {
    size_t sent = 0;
    while (sent < n) {
      ssize_t w = ::send(fd_, buffer + sent, n - sent, 0);
      if (w <= 0) {
        if (w < 0 && errno == EINTR) continue;
        return false;
      }
      sent += static_cast<size_t>(w);
    }
    return true;
  }
  int fd_ = -1;
};

// A blocking connect() to a black-holed host (firewall drop, sleeping machine)
// stalls ~75s, defeating the reconnect backoff; a healthy link must also notice a
// silently-dead host (no RST) rather than blocking in recv() forever. Bound both.
constexpr int kConnectTimeoutMs = 4000;
constexpr int kIoTimeoutSeconds = 10; // > deskflow's 5s CALV keep-alive interval

void set_io_timeouts(int fd) {
  timeval tv{};
  tv.tv_sec = kIoTimeoutSeconds;
  tv.tv_usec = 0;
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

bool connect_with_timeout(int fd, const sockaddr *addr, socklen_t len, int timeout_ms) {
  int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return false;
  bool connected = false;
  if (::connect(fd, addr, len) == 0) {
    connected = true;
  } else if (errno == EINPROGRESS) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLOUT;
    if (::poll(&pfd, 1, timeout_ms) > 0 && (pfd.revents & POLLOUT)) {
      int err = 0;
      socklen_t err_len = sizeof(err);
      if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len) == 0 && err == 0)
        connected = true;
    }
  }
  ::fcntl(fd, F_SETFL, flags); // restore blocking for read_exact/write_all
  return connected;
}

int connect_tcp(const std::string &host, uint16_t port) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *result = nullptr;
  std::string port_str = std::to_string(port);
  int rc = ::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
  if (rc != 0) {
    log_line(std::string("getaddrinfo: ") + gai_strerror(rc));
    return -1;
  }
  int fd = -1;
  for (addrinfo *ai = result; ai != nullptr; ai = ai->ai_next) {
    fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) continue;
    if (connect_with_timeout(fd, ai->ai_addr, ai->ai_addrlen, kConnectTimeoutMs)) {
      set_io_timeouts(fd);
      break;
    }
    ::close(fd);
    fd = -1;
  }
  ::freeaddrinfo(result);
  return fd;
}

// ---------------------------------------------------------------------------
// Virtual HID sink — owns the pqrs client and emits HID reports.
// ---------------------------------------------------------------------------
constexpr std::array<hr::modifier, 8> kAllModifiers = {
    hr::modifier::left_control,  hr::modifier::left_shift,
    hr::modifier::left_option,   hr::modifier::left_command,
    hr::modifier::right_control, hr::modifier::right_shift,
    hr::modifier::right_option,  hr::modifier::right_command};

class VirtualHidSink {
public:
  VirtualHidSink() {
    pqrs::dispatcher::extra::initialize_shared_dispatcher();
    client_ = std::make_unique<pqrs::karabiner::driverkit::virtual_hid_device_service::client>();
    client_->connected.connect([this] {
      pqrs::karabiner::driverkit::virtual_hid_device_service::virtual_hid_keyboard_parameters p;
      p.set_country_code(pqrs::hid::country_code::us);
      client_->async_virtual_hid_keyboard_initialize(p);
      client_->async_virtual_hid_pointing_initialize();
    });
    client_->connect_failed.connect(
        [](auto &&ec) { log_line("vhid connect_failed: " + std::to_string(ec.value())); });
    client_->virtual_hid_keyboard_ready.connect([this](bool r) { keyboard_ready_ = r; });
    client_->virtual_hid_pointing_ready.connect([this](bool r) { pointing_ready_ = r; });
  }
  VirtualHidSink(const VirtualHidSink &) = delete;
  VirtualHidSink &operator=(const VirtualHidSink &) = delete;
  ~VirtualHidSink() {
    if (client_) client_->async_stop();
    pqrs::dispatcher::extra::terminate_shared_dispatcher();
  }

  void start() { client_->async_start(); }

  bool wait_ready(milliseconds timeout) {
    Clock::time_point deadline = Clock::now() + timeout;
    while (Clock::now() < deadline) {
      if (keyboard_ready_ && pointing_ready_) return true;
      std::this_thread::sleep_for(milliseconds(20));
    }
    return keyboard_ready_ && pointing_ready_;
  }

  void post_keyboard(uint8_t modifier_bits, const std::set<uint16_t> &keys) {
    hr::keyboard_input report;
    for (hr::modifier m : kAllModifiers) {
      if (modifier_bits & static_cast<uint8_t>(m)) report.modifiers.insert(m);
    }
    for (uint16_t usage : keys) report.keys.insert(usage);
    client_->async_post_report(report);
  }

  void post_pointing(const std::set<uint8_t> &buttons, int8_t dx, int8_t dy,
                     int8_t vertical_wheel, int8_t horizontal_wheel) {
    hr::pointing_input report;
    for (uint8_t b : buttons) report.buttons.insert(b);
    report.x = static_cast<uint8_t>(dx);
    report.y = static_cast<uint8_t>(dy);
    report.vertical_wheel = static_cast<uint8_t>(vertical_wheel);
    report.horizontal_wheel = static_cast<uint8_t>(horizontal_wheel);
    client_->async_post_report(report);
  }

private:
  std::unique_ptr<pqrs::karabiner::driverkit::virtual_hid_device_service::client> client_;
  std::atomic<bool> keyboard_ready_{false};
  std::atomic<bool> pointing_ready_{false};
};

// ---------------------------------------------------------------------------
// Key translation: Deskflow KeyID + modifier mask -> HID usage / modifier bits.
// ---------------------------------------------------------------------------
// Physical US-keyboard HID usage for an ASCII character, ignoring shift (shift
// is taken from the protocol mask). Both members of a shifted pair map here.
std::optional<uint16_t> ascii_to_physical_usage(char c) {
  if (c >= 'a' && c <= 'z') return static_cast<uint16_t>(0x04 + (c - 'a'));
  if (c >= 'A' && c <= 'Z') return static_cast<uint16_t>(0x04 + (c - 'A'));
  switch (c) {
  case '1': case '!': return 0x1e;
  case '2': case '@': return 0x1f;
  case '3': case '#': return 0x20;
  case '4': case '$': return 0x21;
  case '5': case '%': return 0x22;
  case '6': case '^': return 0x23;
  case '7': case '&': return 0x24;
  case '8': case '*': return 0x25;
  case '9': case '(': return 0x26;
  case '0': case ')': return 0x27;
  case '-': case '_': return 0x2d;
  case '=': case '+': return 0x2e;
  case '[': case '{': return 0x2f;
  case ']': case '}': return 0x30;
  case '\\': case '|': return 0x31;
  case ';': case ':': return 0x33;
  case '\'': case '"': return 0x34;
  case '`': case '~': return 0x35;
  case ',': case '<': return 0x36;
  case '.': case '>': return 0x37;
  case '/': case '?': return 0x38;
  case ' ': return 0x2c;
  default: return std::nullopt;
  }
}

std::optional<uint16_t> special_keyid_to_usage(uint16_t key_id) {
  switch (key_id) {
  case 0xEF08: return 0x2a; // BackSpace
  case 0xEF09: return 0x2b; // Tab
  case 0xEF0D: return 0x28; // Return
  case 0xEF1B: return 0x29; // Escape
  case 0xEFFF: return 0x4c; // Delete (forward)
  case 0xEF50: return 0x4a; // Home
  case 0xEF51: return 0x50; // Left
  case 0xEF52: return 0x52; // Up
  case 0xEF53: return 0x4f; // Right
  case 0xEF54: return 0x51; // Down
  case 0xEF55: return 0x4b; // PageUp
  case 0xEF56: return 0x4e; // PageDown
  case 0xEF57: return 0x4d; // End
  default: return std::nullopt;
  }
}

// Modifier KeyIDs map to a single HID modifier bit; non-modifier keys return 0.
uint8_t modifier_keyid_to_bit(uint16_t key_id) {
  switch (key_id) {
  case 0xEFE1: return static_cast<uint8_t>(hr::modifier::left_shift);
  case 0xEFE2: return static_cast<uint8_t>(hr::modifier::right_shift);
  case 0xEFE3: return static_cast<uint8_t>(hr::modifier::left_control);
  case 0xEFE4: return static_cast<uint8_t>(hr::modifier::right_control);
  case 0xEFE9: return static_cast<uint8_t>(hr::modifier::left_option);
  case 0xEFEA: return static_cast<uint8_t>(hr::modifier::right_option);
  case 0xEFE7: case 0xEFEB: return static_cast<uint8_t>(hr::modifier::left_command);
  case 0xEFE8: case 0xEFEC: return static_cast<uint8_t>(hr::modifier::right_command);
  default: return 0;
  }
}

uint8_t mask_to_modifier_bits(uint32_t mask) {
  uint8_t bits = 0;
  if (mask & proto::kMaskShift) bits |= static_cast<uint8_t>(hr::modifier::left_shift);
  if (mask & proto::kMaskControl) bits |= static_cast<uint8_t>(hr::modifier::left_control);
  if (mask & proto::kMaskAlt) bits |= static_cast<uint8_t>(hr::modifier::left_option);
  if (mask & (proto::kMaskMeta | proto::kMaskSuper))
    bits |= static_cast<uint8_t>(hr::modifier::left_command);
  return bits;
}

// ---------------------------------------------------------------------------
// Bridge — owns input state and translates one host connection.
// ---------------------------------------------------------------------------
class Bridge {
public:
  Bridge(VirtualHidSink &sink, std::string client_name, int16_t fallback_w, int16_t fallback_h,
         double scale_factor)
      : sink_(sink), client_name_(std::move(client_name)),
        screen_w_(fallback_w), screen_h_(fallback_h), scale_factor_(scale_factor) {
    // query_main_display overwrites these if the live display is readable; if it
    // isn't (can happen at the login window), the caller-supplied fallback — the
    // machine's real size from config — is kept instead of a wrong hardcoded guess.
    // It also reports the display's backing scale, from which we derive the motion
    // scale so the bridge auto-adapts to ANY display (this machine's, whichever is
    // at the login screen) rather than a hardcoded value — 1x->4, 2x->8, 3x->12.
    double backing_scale = 2.0;
    query_main_display(screen_w_, screen_h_, backing_scale);
    motion_scale_ = backing_scale * scale_factor_;
    log_line("motion scale " + std::to_string(motion_scale_) + " (backing " +
             std::to_string(backing_scale) + " x factor " + std::to_string(scale_factor_) + ")");
  }

  // Runs one connection to completion (returns on disconnect/error/close).
  void run(FramedSocket &socket) {
    if (!handshake(socket)) return;
    release_all();
    while (!g_stop.load()) {
      std::optional<std::vector<uint8_t>> message = socket.read_message();
      if (!message) break;
      if (!dispatch(socket, *message)) break;
    }
    release_all();
  }

private:
  // One held key/modifier, keyed by the Deskflow physical button id so that
  // key-up matches key-down even if the reported KeyID changed meanwhile.
  struct HeldKey {
    std::optional<uint16_t> usage; // none for pure modifier keys
    uint8_t modifier_bits = 0;
  };

  bool handshake(FramedSocket &socket) {
    std::optional<std::vector<uint8_t>> hello = socket.read_message();
    if (!hello || hello->size() < 11 ||
        std::memcmp(hello->data(), proto::kGreeting, sizeof(proto::kGreeting)) != 0) {
      log_line("bad or missing server hello");
      return false;
    }
    ByteReader reader(*hello);
    reader.skip(sizeof(proto::kGreeting));
    std::optional<int16_t> major = reader.i16();
    std::optional<int16_t> minor = reader.i16();
    if (!reader.ok() || !major || !minor) {
      log_line("malformed server hello");
      return false;
    }
    if (*major != proto::kMajorVersion) {
      log_line("incompatible server major version " + std::to_string(*major));
      return false;
    }
    std::vector<uint8_t> reply;
    append_bytes(reply, proto::kGreeting, sizeof(proto::kGreeting));
    append_be16(reply, proto::kMajorVersion);
    append_be16(reply, proto::kMinorVersion);
    append_be32(reply, static_cast<uint32_t>(client_name_.size()));
    append_bytes(reply, client_name_.data(), client_name_.size());
    if (!socket.write_message(reply)) {
      log_line("failed to send hello-back");
      return false;
    }
    log_line("handshake complete as \"" + client_name_ + "\" (server v" +
             std::to_string(*major) + "." + std::to_string(*minor) + ")");
    return true;
  }

  // Returns false to terminate the connection.
  bool dispatch(FramedSocket &socket, const std::vector<uint8_t> &body) {
    if (body_has_code(body, proto::kKeepAlive)) return socket.write_message(body); // echo CALV
    if (body_has_code(body, proto::kNoop)) return true;
    if (body_has_code(body, proto::kClose)) return false;
    if (body_has_code(body, proto::kQueryInfo)) return send_screen_info(socket);
    if (body_has_code(body, proto::kEnter)) return on_enter(body);
    if (body_has_code(body, proto::kLeave)) { release_all(); return true; }
    if (body_has_code(body, proto::kMouseMove)) return on_mouse_abs(body);
    if (body_has_code(body, proto::kMouseRelMove)) return on_mouse_rel(body);
    if (body_has_code(body, proto::kMouseDown)) return on_mouse_button(body, true);
    if (body_has_code(body, proto::kMouseUp)) return on_mouse_button(body, false);
    if (body_has_code(body, proto::kMouseWheel)) return on_mouse_wheel(body);
    if (body_has_code(body, proto::kKeyDown)) return on_key_down(body);
    if (body_has_code(body, proto::kKeyDownLang)) return on_key_down(body); // v1.8 sends DKDL
    if (body_has_code(body, proto::kKeyUp)) return on_key_up(body);
    if (body_has_code(body, proto::kKeyRepeat)) return on_key_repeat(body);
    // CIAK / CROP / DSOP / CSEC / CCLP and anything else: no input action.
    return true;
  }

  // width/height come in pre-set to the fallback. At the login window the display
  // can be momentarily unreported (CG returns 0) right at boot, which previously
  // pinned the screen to the 1920x1080 fallback and confined the cursor to a
  // sub-rectangle of the real screen — the "invisible barrier". Retry briefly so a
  // not-yet-ready display is detected once WindowServer comes up, and log the
  // outcome so a genuine miss is diagnosable rather than silent.
  static void query_main_display(int16_t &width, int16_t &height, double &backing_scale) {
    backing_scale = 2.0; // sane default (these Macs are 2x Retina) if the query fails
    for (int attempt = 0; attempt < 15; ++attempt) { // ~3s
      CGDirectDisplayID display = CGMainDisplayID();
      size_t w = CGDisplayPixelsWide(display);
      size_t h = CGDisplayPixelsHigh(display);
      if (w >= 16 && w <= 32767 && h >= 16 && h <= 32767) {
        width = static_cast<int16_t>(w);
        height = static_cast<int16_t>(h);
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display);
        size_t pw = mode ? CGDisplayModeGetPixelWidth(mode) : w;
        if (mode) CGDisplayModeRelease(mode);
        if (w && pw) backing_scale = static_cast<double>(pw) / static_cast<double>(w);
        log_line("display detected " + std::to_string(w) + "x" + std::to_string(h) +
                 " (native px width " + std::to_string(pw) + ", backing scale " +
                 std::to_string(backing_scale) + ")");
        return;
      }
      std::this_thread::sleep_for(milliseconds(200));
    }
    log_line("display NOT detected (CG returned 0) -> fallback " +
             std::to_string(width) + "x" + std::to_string(height) +
             " (cursor will be confined if this is wrong)");
  }

  bool send_screen_info(FramedSocket &socket) {
    std::vector<uint8_t> info;
    append_bytes(info, proto::kInfo, sizeof(proto::kInfo));
    append_be16(info, 0);                                    // screen origin x
    append_be16(info, 0);                                    // screen origin y
    append_be16(info, screen_w_);                            // width
    append_be16(info, screen_h_);                            // height
    append_be16(info, 0);                                    // obsolete warp-zone
    append_be16(info, static_cast<int16_t>(screen_w_ / 2));  // initial cursor x
    append_be16(info, static_cast<int16_t>(screen_h_ / 2));  // initial cursor y
    return socket.write_message(info);
  }

  // A relative HID device has no absolute-position command, so establish a known
  // origin by slamming to the screen corner *nearest* the target (the OS clamps
  // the cursor there), then move to the target. Slamming to the nearest corner
  // minimizes visible travel, so the cursor lands cleanly at the host's reported
  // crossing point regardless of which edge the host sits on.
  void warp_to(int x, int y) {
    constexpr int kSlamDistance = 1 << 15; // exceeds any display dimension
    int corner_x = (2 * x < screen_w_) ? 0 : screen_w_;
    int corner_y = (2 * y < screen_h_) ? 0 : screen_h_;
    emit_relative_raw(corner_x == 0 ? -kSlamDistance : kSlamDistance,
                      corner_y == 0 ? -kSlamDistance : kSlamDistance);
    emit_relative(x - corner_x, y - corner_y);
    last_abs_x_ = x;
    last_abs_y_ = y;
    have_last_abs_ = true;
  }

  bool on_enter(const std::vector<uint8_t> &body) {
    ByteReader r(body);
    r.skip(4);
    std::optional<int16_t> x = r.i16();
    std::optional<int16_t> y = r.i16();
    if (!r.ok() || !x || !y) return true;
    // Logs the host's crossing point against our reported size: if the host ever
    // drives near a boundary the bridge can't reach, the mismatch shows up here.
    log_line("enter " + std::to_string(*x) + "," + std::to_string(*y) +
             " of " + std::to_string(screen_w_) + "x" + std::to_string(screen_h_));
    warp_to(*x, *y);
    return true;
  }

  bool on_mouse_abs(const std::vector<uint8_t> &body) {
    ByteReader r(body);
    r.skip(4);
    std::optional<int16_t> x = r.i16();
    std::optional<int16_t> y = r.i16();
    if (!r.ok() || !x || !y) return true;
    if (have_last_abs_)
      emit_relative(static_cast<int>(*x) - last_abs_x_, static_cast<int>(*y) - last_abs_y_);
    last_abs_x_ = *x;
    last_abs_y_ = *y;
    have_last_abs_ = true;
    return true;
  }

  bool on_mouse_rel(const std::vector<uint8_t> &body) {
    ByteReader r(body);
    r.skip(4);
    std::optional<int16_t> dx = r.i16();
    std::optional<int16_t> dy = r.i16();
    if (!r.ok() || !dx || !dy) return true;
    emit_relative(*dx, *dy);
    return true;
  }

  bool on_mouse_button(const std::vector<uint8_t> &body, bool down) {
    ByteReader r(body);
    r.skip(4);
    std::optional<uint8_t> synergy_button = r.u8();
    if (!r.ok() || !synergy_button) return true;
    uint8_t hid_button = synergy_button_to_hid(*synergy_button);
    if (hid_button == 0) return true;
    if (down) mouse_buttons_.insert(hid_button);
    else mouse_buttons_.erase(hid_button);
    sink_.post_pointing(mouse_buttons_, 0, 0, 0, 0);
    return true;
  }

  bool on_mouse_wheel(const std::vector<uint8_t> &body) {
    ByteReader r(body);
    r.skip(4);
    std::optional<int16_t> x = r.i16();
    std::optional<int16_t> y = r.i16();
    if (!r.ok() || !x || !y) return true;
    int8_t vertical = clamp_to_i8(*y / 120);
    int8_t horizontal = clamp_to_i8(*x / 120);
    if (vertical == 0 && *y != 0) vertical = (*y > 0) ? 1 : -1;
    if (horizontal == 0 && *x != 0) horizontal = (*x > 0) ? 1 : -1;
    sink_.post_pointing(mouse_buttons_, 0, 0, vertical, horizontal);
    return true;
  }

  bool on_key_down(const std::vector<uint8_t> &body) {
    int16_t key_id = 0, mask = 0, button = 0;
    if (!parse_key(body, key_id, mask, button)) return true;
    HeldKey entry;
    uint8_t modifier_bit = modifier_keyid_to_bit(static_cast<uint16_t>(key_id));
    if (modifier_bit != 0) {
      entry.modifier_bits = modifier_bit;
    } else {
      std::optional<uint16_t> usage = translate_key(static_cast<uint16_t>(key_id));
      if (!usage) {
        log_line("unmapped key id 0x" + to_hex(static_cast<uint16_t>(key_id)));
        return true;
      }
      entry.usage = usage;
      entry.modifier_bits = mask_to_modifier_bits(static_cast<uint32_t>(static_cast<uint16_t>(mask)));
    }
    held_keys_[button] = entry;
    emit_keyboard();
    return true;
  }

  bool on_key_up(const std::vector<uint8_t> &body) {
    int16_t key_id = 0, mask = 0, button = 0;
    if (!parse_key(body, key_id, mask, button)) return true;
    held_keys_.erase(button);
    emit_keyboard();
    return true;
  }

  // Auto-repeat: the held key is already down, so no report change is required.
  bool on_key_repeat(const std::vector<uint8_t> &) { return true; }

  bool parse_key(const std::vector<uint8_t> &body, int16_t &key_id, int16_t &mask, int16_t &button) {
    ByteReader r(body);
    r.skip(4);
    std::optional<int16_t> id = r.i16();
    std::optional<int16_t> m = r.i16();
    if (!r.ok() || !id || !m) return false;
    std::optional<int16_t> b = r.i16(); // absent in 1.0 variant; default to KeyID
    key_id = *id;
    mask = *m;
    button = b.value_or(*id);
    return true;
  }

  std::optional<uint16_t> translate_key(uint16_t key_id) {
    if (std::optional<uint16_t> special = special_keyid_to_usage(key_id)) return special;
    if (key_id >= 0x20 && key_id <= 0x7e) return ascii_to_physical_usage(static_cast<char>(key_id));
    return std::nullopt;
  }

  void emit_keyboard() {
    uint8_t modifiers = 0;
    std::set<uint16_t> keys;
    for (const auto &[button, held] : held_keys_) {
      modifiers |= held.modifier_bits;
      if (held.usage) keys.insert(*held.usage);
    }
    sink_.post_keyboard(modifiers, keys);
  }

  // Karabiner's pointer is RELATIVE-only (no absolute mode), and with OS pointer
  // acceleration disabled the device's fixed resolution makes one delta cover far
  // less than one screen point (~1/8 on these Retina displays). Scale host-space
  // deltas up by motion_scale_ so they map 1:1 onto the screen. motion_scale_ is
  // derived per-display from the backing scale (see the constructor), so the bridge
  // auto-adapts to whichever machine is at the login screen. emit_relative_raw is
  // the unscaled stepper, used for the corner slam (which only needs to be "big
  // enough" to hit the edge, so it must NOT be scaled again).

  void emit_relative_raw(int dx, int dy) {
    while (dx != 0 || dy != 0) {
      int8_t step_x = clamp_to_i8(dx);
      int8_t step_y = clamp_to_i8(dy);
      sink_.post_pointing(mouse_buttons_, step_x, step_y, 0, 0);
      dx -= step_x;
      dy -= step_y;
    }
  }

  void emit_relative(int dx, int dy) {
    emit_relative_raw(static_cast<int>(dx * motion_scale_), static_cast<int>(dy * motion_scale_));
  }

  void release_all() {
    held_keys_.clear();
    mouse_buttons_.clear();
    have_last_abs_ = false;
    sink_.post_keyboard(0, {});
    sink_.post_pointing({}, 0, 0, 0, 0);
  }

  static int8_t clamp_to_i8(int v) {
    return static_cast<int8_t>(std::clamp(v, -127, 127));
  }
  static uint8_t synergy_button_to_hid(uint8_t synergy_button) {
    switch (synergy_button) {
    case 1: return 1; // left
    case 2: return 3; // middle
    case 3: return 2; // right
    default: return (synergy_button >= 1 && synergy_button <= 32) ? synergy_button : 0;
    }
  }
  static std::string to_hex(uint16_t v) {
    static const char *digits = "0123456789abcdef";
    std::string s(4, '0');
    for (int i = 3; i >= 0; --i) { s[i] = digits[v & 0xf]; v >>= 4; }
    return s;
  }

  VirtualHidSink &sink_;
  std::string client_name_;
  std::map<int16_t, HeldKey> held_keys_;
  std::set<uint8_t> mouse_buttons_;
  int last_abs_x_ = 0;
  int last_abs_y_ = 0;
  bool have_last_abs_ = false;
  int16_t screen_w_ = 1920;
  int16_t screen_h_ = 1080;
  double scale_factor_ = 4.0;  // sensitivity knob (counts/point = backing x this); from arg
  double motion_scale_ = 8.0;  // host-point -> HID-count scale; set per-display in ctor

public:
  static std::atomic<bool> g_stop;
};

std::atomic<bool> Bridge::g_stop{false};

} // namespace

int main(int argc, char **argv) {
  // Split flag arguments (--size=WxH, --scale=S) from positionals so the
  // launchd plist generated by the GUI can pass options without having to
  // fill every preceding positional slot. Legacy positional forms still work.
  std::vector<std::string> positional;
  std::optional<int16_t> flag_w, flag_h;
  std::optional<double> flag_scale;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--size=", 0) == 0) {
      int w = 0, h = 0;
      if (std::sscanf(arg.c_str() + 7, "%dx%d", &w, &h) == 2 && w >= 16 && w <= 32767 && h >= 16 && h <= 32767) {
        flag_w = static_cast<int16_t>(w);
        flag_h = static_cast<int16_t>(h);
      } else {
        log_line("invalid --size (expected WxH): " + arg);
        return 2;
      }
    } else if (arg.rfind("--scale=", 0) == 0) {
      double s = std::strtod(arg.c_str() + 8, nullptr);
      if (s > 0.1 && s < 100.0) {
        flag_scale = s;
      } else {
        log_line("invalid --scale: " + arg);
        return 2;
      }
    } else {
      positional.push_back(arg);
    }
  }

  if (positional.size() < 2) {
    log_line("usage: deskflow-vhid-bridge <server_hosts> <client_screen_name> "
             "[port [width height [scale_factor]]] [--size=WxH] [--scale=S]");
    return 2;
  }

  // Comma-separated server candidates: in auto-switch mode only the elected
  // server listens, so the bridge cycles the list until one accepts.
  std::vector<std::string> server_hosts;
  {
    const std::string &list = positional[0];
    size_t start = 0;
    while (start <= list.size()) {
      size_t comma = list.find(',', start);
      std::string host = list.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
      while (!host.empty() && host.front() == ' ')
        host.erase(host.begin());
      while (!host.empty() && host.back() == ' ')
        host.pop_back();
      if (!host.empty())
        server_hosts.push_back(host);
      if (comma == std::string::npos)
        break;
      start = comma + 1;
    }
  }
  if (server_hosts.empty()) {
    log_line("no server hosts given");
    return 2;
  }
  const std::string client_name = positional[1];
  uint16_t port = proto::kDefaultPort;
  if (positional.size() >= 3) {
    long parsed = std::strtol(positional[2].c_str(), nullptr, 10);
    if (parsed <= 0 || parsed > 65535) {
      log_line("invalid port");
      return 2;
    }
    port = static_cast<uint16_t>(parsed);
  }
  // Optional authoritative screen size (from config) used as the fallback when the
  // live display query is empty — fixes the login-window cursor "barrier" where a
  // wrong size confined the cursor to a sub-rectangle of the real screen.
  int16_t fallback_w = 1920, fallback_h = 1080;
  if (positional.size() >= 5) {
    long pw = std::strtol(positional[3].c_str(), nullptr, 10);
    long ph = std::strtol(positional[4].c_str(), nullptr, 10);
    if (pw >= 16 && pw <= 32767) fallback_w = static_cast<int16_t>(pw);
    if (ph >= 16 && ph <= 32767) fallback_h = static_cast<int16_t>(ph);
  }
  if (flag_w && flag_h) {
    fallback_w = *flag_w;
    fallback_h = *flag_h;
  }
  // Optional sensitivity knob: counts-per-point = backing_scale * scale_factor.
  // 4.0 gives the calibrated full-reach 1:1 on 2x Retina (x8); lower = less sensitive
  // (gentler motion, but the cursor reaches less of the screen). Tunable from config.
  double scale_factor = 4.0;
  if (positional.size() >= 6) {
    double s = std::strtod(positional[5].c_str(), nullptr);
    if (s > 0.1 && s < 100.0) scale_factor = s;
  }
  if (flag_scale)
    scale_factor = *flag_scale;

  struct sigaction sa{};
  sa.sa_handler = [](int) { Bridge::g_stop.store(true); };
  sigaction(SIGTERM, &sa, nullptr);
  sigaction(SIGINT, &sa, nullptr);
  signal(SIGPIPE, SIG_IGN);

  // Disable acceleration so motion is LINEAR (no drift); the resulting fixed
  // count->point scale is then corrected by kMotionScale below. Detached so a hung
  // legacy-HID call can never block the bridge.
  std::thread(disable_pointer_acceleration).detach();

  VirtualHidSink sink;
  sink.start();
  if (!sink.wait_ready(milliseconds(10000))) {
    log_line("virtual HID device not ready (is the Karabiner daemon running?)");
    return 1;
  }
  {
    std::string hosts;
    for (const auto &h : server_hosts)
      hosts += (hosts.empty() ? "" : ", ") + h;
    log_line("virtual HID ready; server candidates: " + hosts + " port " + std::to_string(port));
  }

  Bridge bridge(sink, client_name, fallback_w, fallback_h, scale_factor);
  // Cycle the candidate list; back off only after a full pass with no server
  // accepting, so a role flip to any peer is picked up within one pass.
  int backoff_ms = 500;
  size_t host_idx = 0;
  while (!Bridge::g_stop.load()) {
    const std::string &host = server_hosts[host_idx];
    int fd = connect_tcp(host, port);
    if (fd < 0) {
      host_idx = (host_idx + 1) % server_hosts.size();
      if (host_idx == 0) {
        std::this_thread::sleep_for(milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, 5000);
      }
      continue;
    }
    backoff_ms = 500;
    log_line("connected to host " + host);
    FramedSocket socket(fd);
    bridge.run(socket);
    log_line("host connection closed");
  }
  return 0;
}
